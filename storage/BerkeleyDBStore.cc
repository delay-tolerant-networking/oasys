/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "debug/Debug.h"
#include "util/StringBuffer.h"
#include "serialize/MarshalSerialize.h"

#include "DurableTable.h"
#include "BerkeleyTable.h"

#define NO_TX  0 // for easily going back and changing TX id's later

/// @{ Macros for dealing with Berkeley DB
#define MAKE_DBT(_x, _data, _len)               \
  do {                                          \
    memset(&_x, 0, sizeof(_x));                 \
    _x.data = static_cast<void*>(_data);        \
    _x.size = _len;                             \
  } while(0)
/// @}

namespace oasys {

/******************************************************************************
 *
 * BerkeleyDs
 *
 *****************************************************************************/
BerkeleyStore::BerkeleyStore() 
    : Logger("/berkeleydb/store")
{
    // Real init code in do_init. 
}

BerkeleyStore::~BerkeleyStore()
{
    StringBuffer err_str;

    err_str.append("Tables still open at deletion time: ");
    bool busy = false;

    for(RefCountMap::iterator itr = ref_count_.begin(); 
        itr != ref_count_.end();
        ++itr)
    {
        if(itr->second != 0)
        {
            err_str.appendf("%d ", itr->first);
            busy = true;
        }
    }

    if(busy)
    {
        log_err(err_str.c_str());
    }
    dbenv_->close(dbenv_, 0);
    dbenv_ = 0;

    fclose(err_log_);
}

void
BerkeleyStore::init()
{
    BerkeleyStore* s = new BerkeleyStore();
    DurableTableStore::init(s);
    if(s->do_init() != 0) {
        PANIC("Can't init() BerkeleyDB");
    }
}

int 
BerkeleyStore::do_init()
{
    int err;

    BerkeleyStoreCommand* config = BerkeleyStoreCommand::instance();
    config->ds_ = this;
    
    // cache database name
    db_name_ = config->db_name_;

    // create database directory
    struct stat f_stat;
    const char* config_dir = config->dir_.c_str();
    if(stat(config_dir, &f_stat) == -1)
    {
        if(errno == ENOENT)
        {
            log_info("creating new database directory %s", config_dir);

            if(mkdir(config_dir, 0700) != 0) 
            {
                log_crit("can't create datastore directory %s: %s",
                         config_dir, strerror(errno));
                return DS_ERR;
            }
        }
    }

    // Do real initialization
    db_env_create(&dbenv_, 0);
    if(dbenv_ == 0) {
        log_crit("Can't create db env");
        return DS_ERR;
    }

    err_log_ = ::fopen(config->err_log_.c_str(), "w");

    if(err_log_ == NULL) 
    {
        log_err("Can't create db error log file");
    }
    else 
    {
        dbenv_->set_errfile(dbenv_, err_log_);
    }

    log_info("Using dbdir = %s, errlog = %s", config_dir, config->err_log_.c_str());
    
    if(config->tidy_db_)
    {
        char cmd[256];
        for(int i = config->tidy_wait_; i > 0; --i) {
            log_warn("PRUNING CONTENTS OF %s IN %d SECONDS",
                     config_dir, i);
            sleep(1);
        }
        sprintf(cmd, "/bin/rm -rf %s", config_dir);
        system(cmd);
    }
    if(stat(config_dir, &f_stat) == -1)
    {
        if(errno == ENOENT)
        {
            log_info("creating new database directory %s", config_dir);

            if(mkdir(config_dir, 0700) != 0) {
                log_crit("can't create datastore directory: %s", strerror(errno));
                return DS_ERR;
            }
        }
    }

    err = dbenv_->open(dbenv_, 
                       config_dir,
                       DB_CREATE     | // create new files
                       DB_INIT_MPOOL | // initialize memory pool
                       DB_INIT_LOG   | // use logging
                       DB_INIT_TXN   | // use transactions
                       DB_RECOVER    | // recover from previous crash (if any)
                       DB_PRIVATE,     // only one process can access the db
                       0);             // no flags
    if(err != 0) {
        log_crit("DB: %s, cannot open database", db_strerror(err));
        return DS_ERR;
    }

    err = dbenv_->set_flags(dbenv_,
                            DB_AUTO_COMMIT, // every op is automatically in a tx
                            1);
    if(err != 0) {
        log_crit("DB: %s, cannot set flags", db_strerror(err));
        return DS_ERR;
    }
    
    // Create database file if none exists. Table 0 is used for
    // storing general global metadata.
    std::string dbpath = config_dir;
    dbpath += "/";
    dbpath += config->db_name_;

    if(stat(dbpath.c_str(), &f_stat) == -1) 
    {
        if(errno == ENOENT)
        {
            DB* db;

            db_create(&db, dbenv_, 0);
            log_info("Creating new database file");
            
            err = db->open(db, NO_TX, config->db_name_.c_str(),
                           "0", DB_HASH ,DB_CREATE, 0);
            if(err != 0) {
                log_crit("Can't create database file");
                return DS_ERR;
            }
            
            err = db->close(db, 0);
            if(err != 0) {
                log_crit("Corrupt database file?!");
                return DS_ERR;
            }
        }
        else
        {
            log_err("Unable to stat database file %d", errno);
            return DS_ERR;
        }
    }

    // Initialize max id from meta-database.
    DurableTable* metatable;
    if(get_meta_table(&metatable) != 0) 
    {
        log_crit("Unable to open metatable!");
        return DS_ERR;
    }

    DurableTableItr* itr = new BerkeleyTableItr(metatable);
    int max_id = 0;

    while(itr->next() == 0)
    {
        int id = atoi(static_cast<const char*>(itr->key()));
        max_id = (max_id < id) ? id : max_id;
    }
    next_id_ = max_id + 1;

    delete itr;
    delete metatable;

    return 0;
}

int
BerkeleyStore::new_table(DurableTable** table)
{
    DB* db;
    int err;
    DurableTableId id = next_id_;

    next_id_++;
    err = db_create(&db, dbenv_, 0);
    if(err != 0) {
        log_err("Can't create db pointer");
        return DS_ERR;
    }

    err = db->open(db, NO_TX, db_name_.c_str(), get_name(id).c_str(), 
                   DB_HASH, DB_CREATE, 0);
    if(err != 0) {
        log_err("DB: %s", db_strerror(err));
        return DS_ERR;
    }
    
    log_debug("Creating new table %d", id);

    *table = new BerkeleyTable(id, db);

    return 0;
}


int
BerkeleyStore::del_table(DurableTableId id)
{
    int err;
    
    if(ref_count_[id] != 0)
    {
        log_info("Trying to delete table %d with %d refs still on it",
                 id, ref_count_[id]);

        return DS_BUSY;
    }

    log_info("Deleting table %d", id);
    err = dbenv_->dbremove(dbenv_, NO_TX, db_name_.c_str(), 
                           get_name(id).c_str(), 0);
    if(err != 0) {
        log_err("DB: del_table %s", db_strerror(err));

        if(err == ENOENT) 
        {
            return DS_NOTFOUND;
        }
        else 
        {
            return DS_ERR;
        }
    }
    
    ref_count_.erase(id);

    return 0;
}


int 
BerkeleyStore::get_table(DurableTableId id, DurableTable** table)
{
    DB* db;
    int err;
    
    err = db_create(&db, dbenv_, 0);
    if(err != 0) {
        log_err("Can't create db pointer");
        return DS_ERR;
    }
    
    err = db->open(db, NO_TX, db_name_.c_str(), 
                   get_name(id).c_str(), DB_UNKNOWN, 0, 0);
    if(err != 0) {
        switch(err) 
        {
        case ENOENT:
            return DS_NOTFOUND;
        default:
            log_err("DB: %s", db_strerror(err));
            return DS_ERR;
        }
    }
    
    *table = new BerkeleyTable(id, db);
    
    return 0;
}

int  
BerkeleyStore::get_meta_table(DurableTable** table)
{
    DB* db;
    int err;
    
    err = db_create(&db, dbenv_, 0);
    if(err != 0) {
        log_err("Can't create db pointer");
        return DS_ERR;
    }
    
    err = db->open(db, NO_TX, db_name_.c_str(), 
                   NULL, DB_UNKNOWN, DB_RDONLY, 0);
    if(err != 0) {
        log_err("unable to open metatable - DB: %s", db_strerror(err));
        return DS_ERR;
    }
    
    *table = new BerkeleyTable(META_TABLE_ID, db);
    
    return 0;
}

int
BerkeleyStore::acquire_table(DurableTableId id)
{
    ++ref_count_[id];
    ASSERT(ref_count_[id] >= 0);

    log_debug("table %d, +refcount=%d", id, ref_count_[id]);

    return ref_count_[id];
}

int
BerkeleyStore::release_table(DurableTableId id)
{
    --ref_count_[id];
    ASSERT(ref_count_[id] >= 0);

    log_debug("table %d, -refcount=%d", id, ref_count_[id]);

    return ref_count_[id];
}

std::string 
BerkeleyStore::get_name(int id)
{
    char buf[256];
    sprintf(buf, "%d", id);

    return buf;
}

/******************************************************************************
 *
 * Dstable
 *
 *****************************************************************************/
BerkeleyTable::BerkeleyTable(DurableTableId id, DB* db)
    : DurableTable(id), db_(db)
{
    logpathf("/berkeleydb/table(%d)", id);

    BerkeleyStore* store = 
        static_cast<BerkeleyStore*>(BerkeleyStore::instance()); 
    store->acquire_table(this->id());
}


BerkeleyTable::~BerkeleyTable() 
{
    // Note: If we are to multithread access to the same table, this
    // will have potential concurrency problems, because close can
    // only happen if no other instance of Db is around.
    BerkeleyStore* store = 
        static_cast<BerkeleyStore*>(BerkeleyStore::instance()); 
    
    if(store->release_table(id()) == 0)
    {
        log_debug("closing Db %d", id());
        db_->close(db_, 0);
    }

    delete db_;
}

int 
BerkeleyTable::get(const SerializableObject& key, 
                   SerializableObject*       data)
{
    u_char key_buf[256];
    size_t key_buf_len;

    key_buf_len = flatten_key(key, key_buf, 256);
    if(key_buf_len == 0) 
    {
        log_err("zero or too long key length");
        return DS_ERR;
    }

    DBT k, d;
    MAKE_DBT(k, key_buf, key_buf_len);

    int err = db_->get(db_, NO_TX, &k, &d, 0);
     
    if(err == DB_NOTFOUND) 
    {
        return DS_NOTFOUND;
    }
    else 
    {
        log_err("DB: %s", db_strerror(err));
        return DS_ERR;
    }
    
    Unmarshal unmarshaller(Serialize::CONTEXT_LOCAL, 
                           static_cast<const u_char*>((u_char *)d.data),  d.size);
    data->serialize(&unmarshaller);

    return 0;
}

int 
BerkeleyTable::put(const SerializableObject& key, 
                   const SerializableObject* data)
{
    u_char key_buf[256];
    size_t key_buf_len;

    key_buf_len = flatten_key(key, key_buf, 256);
    if(key_buf_len == 0) 
    {
        log_err("zero or too long key length");
        return DS_ERR;
    }

    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    const_cast<SerializableObject*>(data)->serialize(&sizer);
    
    { // lock
        ScopeLock lock(&scratch_mutex_);
    
        u_char* buf = scratch_.buf(sizer.size());
        int err;
        
        Marshal m(Serialize::CONTEXT_LOCAL, buf, scratch_.size());
        const_cast<SerializableObject*>(data)->serialize(&m);
        
        DBT k, d;
        MAKE_DBT(k, key_buf, key_buf_len);
        MAKE_DBT(d, buf, sizer.size());
        err = db_->put(db_, NO_TX, &k, &d, 0);

        if(err != 0) 
        {
            log_err("DB: %s", db_strerror(err));
            return DS_ERR;
        }
    } // unlock

    return 0;
}

int 
BerkeleyTable::del(const SerializableObject& key)
{
    u_char key_buf[256];
    size_t key_buf_len;

    key_buf_len = flatten_key(key, key_buf, 256);
    if(key_buf_len == 0) 
    {
        log_err("zero or too long key length");
        return DS_ERR;
    }

    DBT k;
    MAKE_DBT(k, key_buf, key_buf_len);
    
    int err = db_->del(db_, NO_TX, &k, 0);
    
    if(err == DB_NOTFOUND) 
    {
        return DS_NOTFOUND;
    } 
    else if(err != 0) 
    {
        log_err("DB: %s", db_strerror(err));
        return DS_ERR;
    }

    return 0;
}

size_t
BerkeleyTable::flatten_key(const SerializableObject& key, 
                           u_char* key_buf, size_t size)
{
    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    const_cast<SerializableObject&>(key).serialize(&sizer);
    if(sizer.size() < size)
    {
        return 0;
    }

    Marshal marshaller(Serialize::CONTEXT_LOCAL, key_buf, 256);
    const_cast<SerializableObject&>(key).serialize(&marshaller);
    
    return sizer.size();
}

/******************************************************************************
 *
 * BerkeleyTableItr
 *
 *****************************************************************************/
BerkeleyTableItr::BerkeleyTableItr(DurableTable* d)
    : cur_(0), valid_(false)
{
    logpathf("/berkeleydb/itr(%d)", d->id());

    BerkeleyTable* t = dynamic_cast<BerkeleyTable*>(d);
    ASSERT(t != 0);

    int err = t->db_->cursor(t->db_, NO_TX, &cur_, 0);
    if(err != 0) {
        log_err("DB: cannot create a DB iterator, err=%s", db_strerror(err));
        cur_ = 0;
    }

    if(cur_)
    {
        valid_ = true;
    }
}

BerkeleyTableItr::~BerkeleyTableItr()
{
    valid_ = false;
    if(cur_) 
    {
        int err = cur_->c_close(cur_);

        if(err != 0) {
            log_err("Unable to close cursor, %s", db_strerror(err));
        }
    }
}

int
BerkeleyTableItr::next()
{
    ASSERT(valid_);

    int err = cur_->c_get(cur_, &key_, &data_, DB_NEXT);
    if(err != 0) {
        log_err("DB: %s", db_strerror(err));
        valid_ = false;
        
        return DS_ERR;
    }
    
    if(err == DB_NOTFOUND)
    {
        valid_ = false;

        return DS_NOTFOUND;
    }

    return 0;
}

int 
BerkeleyTableItr::get(SerializableObject* obj)
{
    return 0; // XXX/bowei
}


/******************************************************************************
 *
 * BerkeleyStoreCommand
 *
 *****************************************************************************/
BerkeleyStoreCommand* BerkeleyStoreCommand::instance_;

BerkeleyStoreCommand::BerkeleyStoreCommand()
    : oasys::AutoTclCommand("ds"), 
      tidy_db_(false),
      tidy_wait_(3),
      dir_("/var/tier/db"),
      err_log_("/var/tier/db/err.log"),
      ds_(NULL)
{
}

void
BerkeleyStoreCommand::bind_vars()
{
    bind_b("tidy",   	&tidy_db_);
    bind_i("tidywait",	&tidy_wait_);
    bind_s("dir",    	&dir_);
    bind_s("errlog", 	&err_log_);
}

int
BerkeleyStoreCommand::exec(int argc, const char** argv, Tcl_Interp* interp)
{
#if 0
    int err;

    switch(argc) 
    {
    case 2: {
        if(strcmp(argv[1], "info") == 0) 
        {
            
            std::string out;
            for(std::map<int,int>::iterator itr = ds_->ref_count_.begin();
                itr != ds_->ref_count_.end();
                ++itr)
            {
                char buf[50];
                sprintf(buf, "(%d, %d) ", (*itr).first, (*itr).second);
                out += buf;
            }
            log_info("%s", out.c_str());
            
            return 0;
        } // if(info)
        break;
    } // case 2

    case 3: {
        if(strcmp(argv[1], "get") == 0) 
        {
            if(strcmp(argv[2], "tables") == 0) 
            {
                BerkeleyTable* m;
                err = ds_->get_meta_table(&m);
                if(err != 0) {
                    resultf("unable to get metatable");
                    return TCL_ERROR;
                }
                
                BerkeleyTableItr* itr = new BerkeleyTableItr(m);
                std::string result;
                
                while(itr->next() == 0) 
                {
                    char buf[200];
                    memcpy(buf, itr->key(), itr->key_len());
                    buf[itr->key_len()] = '\0';
                    result = result + " " + buf;
                }

                log_info("tables %s", result.c_str());
                set_result(result.c_str());

                delete itr;
                delete m;

		return 0;
            }
        } // if(get)
        else if(strcmp(argv[1], "delete") == 0)
        {
            int id = atoi(argv[2]);
            
            if(id <= 0)
            {
                set_result( "Can't delete table <= 0.");
                return -1;
            }

            if(ds_->ref_count_[id] != 0) 
            {
                set_result( "Table still has references.");
                return -1;
            }

            int err;
            if((err = ds_->del_table(id)) != 0)
            {
                if(err == DS_NOTFOUND)
                {
                    set_result("Table not found.");
                    return -1;
                }
                else
                {
                    set_result("Error deleting table.");
                    return -1;
                }
            }
            return 0;
        } // if(delete)

        break;
    } // case 3

    case 4: {
        if(strcmp(argv[1], "get") == 0)
        {
            if(strcmp(argv[2], "table") == 0)
            {
                DurableTable* t;
                int table = atoi(argv[3]);

                err = BerkeleyStore:instance()->get_table(table, &t);
                if(err != 0) {
                    set_result("Couldn't get table");
                    return -1;
                }
                BerkeleyTableItr* itr = new BerkeleyTableItr(t);
             
                oasys::StringBuffer result;
                    
                while(itr->next() == 0) {
                    result.appendf("%.*s { %.*s }",
                                   itr->key_len(),
                                   (char*)itr->key(),
                                   itr->data_len() < 50 ? itr->data_len() : 50,
                                   (char*)itr->data());
                }

                // make unprintable characters '?'
                char* buf = result.data();
                for(size_t i = 0; i < result.length(); i++) {
                    if(buf[i] < 32 || buf[i] > 126)
                        buf[i] = '?';
                }
                
                set_result(result.c_str());
                
                delete itr;
                delete t;

		return 0;
            }
        } // if(get)        

        break;
    } // case 4
    } // switch(argc)

    set_result("Unsupported command");
    return -1;
    #endif // if 0

    return -1;
}

}; // namespace oasys
