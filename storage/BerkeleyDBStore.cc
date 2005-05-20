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

#include "DurableStore.h"
#include "BerkeleyDBStore.h"

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
BerkeleyDBStore::init(const std::string& db_name,
                      const char*        db_dir,
                      bool               tidy_db,
                      int                tidy_wait)
{
    BerkeleyDBStore* s = new BerkeleyDBStore();
    DurableStore::init(s);

    int err;
    err = s->do_init(db_name, 
                     db_dir, 
                     tidy_db, 
                     tidy_wait);
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
BerkeleyDBStore::do_init(const std::string& db_name,
                         const char*        db_dir,
                         bool               tidy_db,
                         int                tidy_wait)
{
    int err;

    db_name_ = db_name;

    // create database directory
    struct stat f_stat;

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

    log_info("Using dbdir = %s, errlog = %s", db_dir, err_filename.c_str());
    if(tidy_db)
    {
        char cmd[256];
        for(int i = tidy_wait; i > 0; --i) {
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
                log_crit("can't create datastore directory: %s", strerror(errno));
                return DS_ERR;
            }
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
                            DB_AUTO_COMMIT, // every op is automatically in a tx
                            1);
    if(err != 0) {
        log_crit("DB: %s, cannot set flags", db_strerror(err));
        return DS_ERR;
    }
    
    // Create database file if none exists. Table 0 is used for
    // storing general global metadata.
    std::string dbpath = db_dir;
    dbpath += "/";
    dbpath += db_name_;

    if(stat(dbpath.c_str(), &f_stat) == -1) 
    {
        if(errno == ENOENT)
        {
            DB* db;

            db_create(&db, dbenv_, 0);
            log_info("Creating new database file");
            
            err = db->open(db, NO_TX, db_name_.c_str(),
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
    BerkeleyDBTable* metatable;
    if(get_meta_table(&metatable) != 0) 
    {
        log_crit("Unable to open metatable!");
        return DS_ERR;
    }

    DurableIterator* dItr;
    err = metatable->itr(&dItr);
    BerkeleyDBIterator* pItr = static_cast<BerkeleyDBIterator*>(dItr);
    
    if(err != 0) {
        PANIC("Unable to create metatable iterator");
    }

    int max_id = 0;
    while(pItr->next() == 0)
    {
	char raw_key_value[256];
	void* raw_key;
	size_t raw_key_len;
	
	err = pItr->raw_key(&raw_key, &raw_key_len);
	if(err != 0) {
	    PANIC("Iterator to metatable is unusable!");
	}

	memcpy(raw_key_value, raw_key, raw_key_len);
	raw_key_value[raw_key_len] = '\0';

	int cur_id = atoi(raw_key_value);
        max_id = (max_id < cur_id) ? cur_id : max_id;
        
        log_debug("metatable has %s", raw_key_value);
    }
    next_id_ = max_id + 1;

    delete pItr;
    delete metatable;

    return 0;
}

int
BerkeleyDBStore::new_table(DurableTable** table, DurableTableID new_id)
{
    DB* db;
    int err;
    DurableTableID id;

    if(new_id != -1) 
    {
        oasys::ScopePtr<BerkeleyDBTable> metatable;
        if(get_meta_table(&(metatable.get())) != 0)
        {
            log_err("Can't open metatable");
            return DS_ERR;
        }

        StaticStringBuffer<256> buf;
        buf.appendf("%d", new_id);

        if(metatable->key_exists(buf.c_str(), buf.size()) == 0) 
        {
            log_err("Table already exists");
            return DS_EXISTS;
        }
        id = new_id;
        if(next_id_ < id) 
        {
            next_id_ = id + 1;
        }
    } 
    else 
    {
        id = next_id_;
        next_id_++;
    }

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

    *table = new BerkeleyDBTable(id, db);

    return 0;
}


int
BerkeleyDBStore::del_table(DurableTableID id)
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
BerkeleyDBStore::get_table(DurableTableID id, DurableTable** table)
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
    
    *table = new BerkeleyDBTable(id, db);
    
    return 0;
}

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
    
    *table = static_cast<BerkeleyDBTable*>
             (new BerkeleyDBTable(META_TABLE_ID, db));
    
    return 0;
}

int
BerkeleyDBStore::acquire_table(DurableTableID id)
{
    ++ref_count_[id];
    ASSERT(ref_count_[id] >= 0);

    log_debug("table %d, +refcount=%d", id, ref_count_[id]);

    return ref_count_[id];
}

int
BerkeleyDBStore::release_table(DurableTableID id)
{
    --ref_count_[id];
    ASSERT(ref_count_[id] >= 0);

    log_debug("table %d, -refcount=%d", id, ref_count_[id]);

    return ref_count_[id];
}

std::string 
BerkeleyDBStore::get_name(int id)
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
BerkeleyDBTable::BerkeleyDBTable(DurableTableID id, DB* db)
    : DurableTable(id), db_(db)
{
    logpathf("/berkeleydb/table(%d)", id);

    BerkeleyDBStore* store = 
        static_cast<BerkeleyDBStore*>(BerkeleyDBStore::instance()); 
    store->acquire_table(this->id());
}


BerkeleyDBTable::~BerkeleyDBTable() 
{
    // Note: If we are to multithread access to the same table, this
    // will have potential concurrency problems, because close can
    // only happen if no other instance of Db is around.
    BerkeleyDBStore* store = 
        static_cast<BerkeleyDBStore*>(BerkeleyDBStore::instance()); 
    
    if(store->release_table(id()) == 0)
    {
        log_debug("closing Db %d", id());
        db_->close(db_, 0);
    }
}

int 
BerkeleyDBTable::get(const SerializableObject& key, 
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
                           static_cast<const u_char*>((u_char *)d.data),  d.size);
    data->serialize(&unmarshaller);

    return 0;
}

int 
BerkeleyDBTable::put(const SerializableObject& key, 
                     const SerializableObject& data)
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
    sizer.action(const_cast<SerializableObject*>(&data));
    
    { // lock
        ScopeLock lock(&scratch_mutex_);
    
        u_char* buf = scratch_.buf(sizer.size());
        int err;
        
        Marshal m(Serialize::CONTEXT_LOCAL, buf, scratch_.size());
	m.action(const_cast<SerializableObject*>(&data));
        
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
BerkeleyDBTable::del(const SerializableObject& key)
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

int 
BerkeleyDBTable::itr(DurableIterator** itr)
{
    *itr = new BerkeleyDBIterator(this);
    ASSERT(*itr);
    
    return 0;
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

size_t
BerkeleyDBTable::flatten_key(const SerializableObject& key, 
                             u_char* key_buf, size_t size)
{
    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    sizer.action(const_cast<SerializableObject*>(&key));

    if(sizer.size() > size)
    {
        return 0;
    }

    Marshal marshaller(Serialize::CONTEXT_LOCAL, key_buf, 256);
    const_cast<SerializableObject&>(key).serialize(&marshaller);
    
    return sizer.size();
}

/******************************************************************************
 *
 * BerkeleyDBIterator
 *
 *****************************************************************************/
BerkeleyDBIterator::BerkeleyDBIterator(DurableTable* d)
    : cur_(0), valid_(false)
{
    logpathf("/berkeleydb/itr(%d)", d->id());

    BerkeleyDBTable* t = dynamic_cast<BerkeleyDBTable*>(d);
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

    bzero(&key_, sizeof(DBT));
    bzero(&data_, sizeof(DBT));

    int err = cur_->c_get(cur_, &key_, &data_, DB_NEXT);
    if(err == DB_NOTFOUND) 
    {
        valid_ = false;

        return DS_NOTFOUND;
    } 
    else if(err != 0) 
    {
        log_err("next() DB: %s", db_strerror(err));
        valid_ = false;
        
        return DS_ERR;
    }

    return 0;
}

int 
BerkeleyDBIterator::get(SerializableObject* keyObj, 
                        SerializableObject* dataObj)
{
    if(!valid_)
        return DS_ERR;

    if(keyObj != 0) {
        oasys::Unmarshal un(oasys::Serialize::CONTEXT_LOCAL,
                            static_cast<u_char*>(key_.data),
                            key_.size, 0);
        if(un.action(keyObj) != 0) {
            log_debug("Can't unmarshal key");
            return DS_ERR;
        }
    }
    
    if(dataObj != 0) {
        oasys::Unmarshal un(oasys::Serialize::CONTEXT_LOCAL,
                            static_cast<u_char*>(data_.data),
                            data_.size, 0);
        if(un.action(dataObj) != 0) {
            log_debug("Can't unmarshal data");
            return DS_ERR;
        }
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
