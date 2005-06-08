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

#include <debug/Debug.h>
#include <util/StringBuffer.h>
#include <util/Pointers.h>
#include <serialize/MarshalSerialize.h>
#include <serialize/TypeShims.h>

#include "BerkeleyDBStore.h"
#include "StorageConfig.h"
#include "util/InitSequencer.h"

#define NO_TX  0 // for easily going back and changing TX id's later

/// @{ Macros for dealing with Berkeley DB
#define MAKE_DBT(_x, _data, _len)               \
  do {                                          \
    bzero(&_x, sizeof(_x));                     \
    _x.data = static_cast<void*>(_data);        \
    _x.size = _len;                             \
  } while(0)
/// @}

namespace oasys {

/*
 * BerkeleyDB depends on StorageConfig
 */

/******************************************************************************
 *
 * BerkeleyDBStore
 *
 *****************************************************************************/
const std::string BerkeleyDBStore::META_TABLE_NAME("___META_TABLE___");

BerkeleyDBStore::BerkeleyDBStore() 
    : Logger("/berkeleydb/store")
{
    // Real init code in do_init.
}

BerkeleyDBStore::~BerkeleyDBStore()
{
    StringBuffer err_str;

    err_str.append("Tables still open at deletion time: ");
    bool busy = false;

    for (RefCountMap::iterator iter = ref_count_.begin(); 
         iter != ref_count_.end(); ++iter)
    {
        if(iter->second != 0)
        {
            err_str.appendf("%s ", iter->first.c_str());
            busy = true;
        }
    }

    if (busy)
    {
        log_err(err_str.c_str());
    }
    dbenv_->close(dbenv_, 0);
    dbenv_ = 0;

    fclose(err_log_);
}

void 
BerkeleyDBStore::init()
{
    BerkeleyDBStore* s = new BerkeleyDBStore();
    DurableStore::init(s);

    int err;
    err = s->do_init();
    
    if(err != 0) {
        PANIC("Can't init() BerkeleyDB");
    }
}

void
BerkeleyDBStore::shutdown_for_debug()
{
    DurableStore::shutdown();
}

int 
BerkeleyDBStore::do_init()
{
    int err;
    StorageConfig* cfg = StorageConfig::instance();

    db_name_ = cfg->dbname_ + ".db";
    
    // XXX/demmer this shouldn't always create the db, rather should
    // look for DS_CREATE and DS_EXCL flags like everything else
    if (cfg->dbflags_ != 0) {
        PANIC("XXX/demmer handle ds_init flags");
    }

    // create database directory
    struct stat f_stat;
    const char* db_dir = cfg->dbdir_.c_str();

    if(stat(db_dir, &f_stat) == -1)
    {
        if(errno == ENOENT)
        {
            log_info("creating new database directory %s", db_dir);

            if(mkdir(db_dir, 0700) != 0) 
            {
                log_crit("can't create datastore directory %s: %s",
                         db_dir, strerror(errno));
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

    std::string err_filename = db_dir;
    err_filename += "/err.log";
    err_log_ = ::fopen(err_filename.c_str(), "w");

    if(err_log_ == NULL) 
    {
        log_err("Can't create db error log file");
    }
    else 
    {
        dbenv_->set_errfile(dbenv_, err_log_);
    }

    log_info("using dbdir = %s, errlog = %s", db_dir, err_filename.c_str());
    if (cfg->tidy_)
    {
        char cmd[256];
        for(int i = cfg->tidy_wait_; i > 0; --i) {
            log_warn("PRUNING CONTENTS OF %s IN %d SECONDS",
                     db_dir, i);
            sleep(1);
        }
        sprintf(cmd, "/bin/rm -rf %s", db_dir);
        system(cmd);
    }

    if(stat(db_dir, &f_stat) == -1)
    {
        if(errno == ENOENT)
        {
            log_info("creating new database directory %s", db_dir);

            if(mkdir(db_dir, 0700) != 0) {
                log_crit("can't create datastore directory: %s",
                         strerror(errno));
                return DS_ERR;
            }
        }
        else
        {
            log_err("error trying to stat database directory %s: %s",
                    db_dir, strerror(errno));
            return DS_ERR;
        }
    }

    err = dbenv_->open(dbenv_, 
                       db_dir,
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
                            DB_AUTO_COMMIT, // op is automatically in a tx
                            1);
    if(err != 0) {
        log_crit("DB: %s, cannot set flags", db_strerror(err));
        return DS_ERR;
    }
    
    return 0;
}

int
BerkeleyDBStore::get_table(DurableTableImpl**  table,
                           const std::string&  name,
                           int                 flags,
                           PrototypeVector&    prototypes)
{
    DB* db;
    int err;
    DBTYPE db_type = DB_BTREE;
    u_int32_t db_flags;

    // grab a new database handle
    err = db_create(&db, dbenv_, 0);
    if (err != 0) {
        log_err("error creating database handle: %s", db_strerror(err));
        return DS_ERR;
    }
    
    // if the user requested that the table be created, add the entry
    // to the meta table, and calculate the db type and creation flags
    db_flags = 0;
    if (flags & DS_CREATE) {
        db_flags |= DB_CREATE;

        if (((flags & DS_BTREE) != 0) && ((flags & DS_HASH) != 0)) {
            PANIC("both DS_HASH and DS_BTREE were specified");
        }
        
        if (flags & DS_HASH)
        {
            db_type = DB_HASH;
        }
        else if (flags & DS_BTREE)
        {
            db_type = DB_BTREE;
        }
        else // XXX/demmer force type to be specified??
        {
            db_type = DB_BTREE;
        }
    }

    if (flags & DS_EXCL) {
        db_flags |= DB_EXCL;
    }
    
    err = db->open(db, NO_TX, db_name_.c_str(), name.c_str(),
                   db_type, db_flags, 0);

    if (err == ENOENT)
    {
        log_debug("get_table -- notfound database %s", name.c_str());
        return DS_NOTFOUND;
    }
    else if (err == EEXIST)
    {
        log_debug("get_table -- already existing database %s", name.c_str());
        return DS_EXISTS;
    }
    else if (err != 0)
    {
        log_err("DB internal error in get_table: %s", db_strerror(err));
        return DS_ERR;
    }
    
    log_debug("get_table -- opened table %s", name.c_str());

    *table = new BerkeleyDBTable(name, db);

    return 0;
}


int
BerkeleyDBStore::del_table(const std::string& name)
{
    int err;
    
    if(ref_count_[name] != 0)
    {
        log_info("Trying to delete table %s with %d refs still on it",
                 name.c_str(), ref_count_[name]);
        
        return DS_BUSY;
    }

    log_info("deleting table %s", name.c_str());
    err = dbenv_->dbremove(dbenv_, NO_TX, db_name_.c_str(), name.c_str(), 0);
    
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
    
    ref_count_.erase(name);

    return 0;
}

/**
 * Get a handle on the internal metatable by passing NULL for the
 * table name to db->open.
 */
int  
BerkeleyDBStore::get_meta_table(BerkeleyDBTable** table)
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
    
    *table = new BerkeleyDBTable(META_TABLE_NAME, db);
    
    return 0;
}

int
BerkeleyDBStore::acquire_table(const std::string& table)
{
    ++ref_count_[table];
    ASSERT(ref_count_[table] >= 0);

    log_debug("table %s, +refcount=%d", table.c_str(), ref_count_[table]);

    return ref_count_[table];
}

int
BerkeleyDBStore::release_table(const std::string& table)
{
    --ref_count_[table];
    ASSERT(ref_count_[table] >= 0);

    log_debug("table %s, -refcount=%d", table.c_str(), ref_count_[table]);

    return ref_count_[table];
}

/******************************************************************************
 *
 * BerkeleyDBTable
 *
 *****************************************************************************/
BerkeleyDBTable::BerkeleyDBTable(std::string name, DB* db)
    : DurableTableImpl(name), db_(db)
{
    logpathf("/berkeleydb/table(%s)", name.c_str());

    BerkeleyDBStore* store = BerkeleyDBStore::instance();
    store->acquire_table(name);
}


BerkeleyDBTable::~BerkeleyDBTable() 
{
    // Note: If we are to multithread access to the same table, this
    // will have potential concurrency problems, because close can
    // only happen if no other instance of Db is around.
    BerkeleyDBStore* store = 
        static_cast<BerkeleyDBStore*>(BerkeleyDBStore::instance()); 
    
    store->release_table(name());

    log_debug("closing db %s", name());
    db_->close(db_, 0);
}

int 
BerkeleyDBTable::get(const SerializableObject& key, 
                     SerializableObject*       data)
{
    u_char key_buf[256];
    size_t key_buf_len;

    key_buf_len = flatten(key, key_buf, sizeof(key_buf));
    if(key_buf_len == 0) 
    {
        log_err("zero or too long key length");
        return DS_ERR;
    }

    DBT k, d;
    bzero(&d, sizeof(d));

    MAKE_DBT(k, key_buf, key_buf_len);

    int err = db_->get(db_, NO_TX, &k, &d, 0);
     
    if(err == DB_NOTFOUND) 
    {
        return DS_NOTFOUND;
    }
    else if(err != 0)
    {
        log_err("DB: %s", db_strerror(err));
        return DS_ERR;
    }

    
    Unmarshal unmarshaller(Serialize::CONTEXT_LOCAL,
                           (u_char*)(d.data), d.size);
    
    if (unmarshaller.action(data) != 0) {
        log_err("DB: error unserializing data object");
        return DS_ERR;
    }

    return 0;
}

int 
BerkeleyDBTable::put(const SerializableObject& key, 
                     const SerializableObject* data,
                     int                       flags)
{
    u_char key_buf[256];
    size_t key_buf_len;
    DBT k, d;
    int db_flags = 0;
    int err;

    // flatten and fill in the key
    key_buf_len = flatten(key, key_buf, 256);
    if(key_buf_len == 0) 
    {
        log_err("zero or too long key length");
        return DS_ERR;
    }
    MAKE_DBT(k, key_buf, key_buf_len);


    // if the caller does not want to create new entries, first do a
    // db get to see if the key already exists
    if ((flags & DB_CREATE) == 0) {
        bzero(&d, sizeof(d));

        err = db_->get(db_, NO_TX, &k, &d, 0);
        if (err == DB_NOTFOUND) {
            return DS_NOTFOUND;

        } else if (err != 0) {
            log_err("put -- DB internal error: %s", db_strerror(err));
            return DS_ERR;
        }
    }

    // figure out the size of the data
    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    if (sizer.action(data) != 0) {
        log_err("error sizing data object");
        return DS_ERR;
    }

    log_debug("put: serializing %d byte object", sizer.size());

    { // lock
        ScopeLock lock(&scratch_mutex_);
    
        u_char* buf = scratch_.buf(sizer.size());
        Marshal m(Serialize::CONTEXT_LOCAL, buf, scratch_.size());
	if (m.action(data) != 0) {
            log_err("error serializing data object");
            return DS_ERR;
        }
        MAKE_DBT(d, buf, sizer.size());

        db_flags = 0;
        if (flags & DS_EXCL) {
            db_flags |= DB_NOOVERWRITE;
        }
        
        err = db_->put(db_, NO_TX, &k, &d, db_flags);

        if (err == DB_KEYEXIST) {
            return DS_EXISTS;

        } else if (err != 0) {
            log_err("DB internal error: %s", db_strerror(err));
            return DS_ERR;
        }
    } // unlock

    return 0;
}

int 
BerkeleyDBTable::del(const SerializableObject& key)
{
    u_char key_buf[256];
    size_t key_buf_len;

    key_buf_len = flatten(key, key_buf, 256);
    if(key_buf_len == 0) 
    {
        log_err("zero or too long key length");
        return DS_ERR;
    }

    DBT k;
    MAKE_DBT(k, key_buf, key_buf_len);
    
    int err = db_->del(db_, NO_TX, &k, 0);
    
    if (err == DB_NOTFOUND) 
    {
        return DS_NOTFOUND;
    } 
    else if (err != 0) 
    {
        log_err("DB internal error: %s", db_strerror(err));
        return DS_ERR;
    }

    return 0;
}

DurableIterator*
BerkeleyDBTable::iter()
{
    return new BerkeleyDBIterator(this);
}

int 
BerkeleyDBTable::key_exists(const void* key, size_t key_len)
{
    DBT k, d;
    bzero(&d, sizeof(d));

    MAKE_DBT(k, const_cast<void*>(key), key_len);

    int err = db_->get(db_, NO_TX, &k, &d, 0);
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

/******************************************************************************
 *
 * BerkeleyDBIterator
 *
 *****************************************************************************/
BerkeleyDBIterator::BerkeleyDBIterator(BerkeleyDBTable* t)
    : cur_(0), valid_(false)
{
    logpathf("/berkeleydb/iter(%s)", t->name());

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

BerkeleyDBIterator::~BerkeleyDBIterator()
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
BerkeleyDBIterator::next()
{
    ASSERT(valid_);

    bzero(&key_,  sizeof(key_));
    bzero(&data_, sizeof(data_));

    int err = cur_->c_get(cur_, &key_, &data_, DB_NEXT);

    if (err == DB_NOTFOUND) {
        valid_ = false;
        return DS_NOTFOUND;
    } 
    else if (err != 0) {
        log_err("next() DB: %s", db_strerror(err));
        valid_ = false;
        return DS_ERR;
    }

    return 0;
}

int
BerkeleyDBIterator::get(SerializableObject* key)
{
    ASSERT(key != NULL);
    oasys::Unmarshal un(oasys::Serialize::CONTEXT_LOCAL,
                        static_cast<u_char*>(key_.data), key_.size);

    if (un.action(key) != 0) {
        log_err("error unmarshalling");
        return DS_ERR;
    }
    
    return 0;
}

int 
BerkeleyDBIterator::raw_key(void** key, size_t* len)
{
    if(!valid_) return DS_ERR;

    *key = key_.data;
    *len = key_.size;

    return 0;
}

int 
BerkeleyDBIterator::raw_data(void** data, size_t* len)
{
    if(!valid_) return DS_ERR;

    *data = data_.data;
    *len  = data_.size;

    return 0;
}

} // namespace oasys

