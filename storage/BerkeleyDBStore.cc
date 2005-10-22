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
#include <errno.h>
#include <unistd.h>

#include <debug/DebugUtils.h>
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
  } while (0)
/// @}

namespace oasys {
/******************************************************************************
 *
 * BerkeleyDBStore
 *
 *****************************************************************************/
const std::string BerkeleyDBStore::META_TABLE_NAME("___META_TABLE___");

BerkeleyDBStore::BerkeleyDBStore() 
    : DurableStoreImpl("/storage/berkeleydb"),
      init_(false)
{}

BerkeleyDBStore::~BerkeleyDBStore()
{
    StringBuffer err_str;

    err_str.append("Tables still open at deletion time: ");
    bool busy = false;

    for (RefCountMap::iterator iter = ref_count_.begin(); 
         iter != ref_count_.end(); ++iter)
    {
        if (iter->second != 0)
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
    log_info("db closed");
}

int 
BerkeleyDBStore::init(StorageConfig* cfg)
{
    db_name_ = cfg->dbname_;
    sharefile_ = cfg->dbsharefile_;

    // XXX/bowei need to expose options if needed later
    if (cfg->tidy_) {
        prune_db_dir(cfg->dbdir_.c_str(), cfg->tidy_wait_);
    }

    bool db_dir_exists;
    int  err = check_db_dir(cfg->dbdir_.c_str(), &db_dir_exists);
    if (err != 0)
    {
        return DS_ERR;
    }
    if (!db_dir_exists) 
    {
        if (cfg->init_) {
            if (create_db_dir(cfg->dbdir_.c_str()) != 0) {
                return DS_ERR;
            }
        } else {
            log_crit("DB dir %s does not exist and not told to create!",
                     cfg->dbdir_.c_str());
            return DS_ERR;
        }
    }

    db_env_create(&dbenv_, 0);
    if (dbenv_ == 0) 
    {
        log_crit("Can't create db env");
        return DS_ERR;
    }

    dbenv_->set_errcall(dbenv_, BerkeleyDBStore::db_errcall);

    log_info("initializing db name=%s (%s), dir=%s",
             db_name_.c_str(), sharefile_ ? "shared" : "not shared",
             cfg->dbdir_.c_str());

    err = dbenv_->open(
        dbenv_, 
        cfg->dbdir_.c_str(),
        DB_CREATE     |         // create new files
        DB_INIT_MPOOL |         // initialize memory pool
        DB_INIT_LOG   |         // use logging
        DB_INIT_TXN   |         // use transactions
        DB_RECOVER    |         // recover from previous crash (if any)
        DB_PRIVATE,             // only one process can access the db
        0                       // no flags
    );                     

    if (err != 0) 
    {
        log_crit("DB: %s, cannot open database", db_strerror(err));
        return DS_ERR;
    }

    err = dbenv_->set_flags(dbenv_,
                            DB_AUTO_COMMIT |
                            DB_LOG_AUTOREMOVE, // every operation is a tx
                            1);
    if (err != 0) 
    {
        log_crit("DB: %s, cannot set flags", db_strerror(err));
        return DS_ERR;
    }

    err = dbenv_->set_paniccall(dbenv_, BerkeleyDBStore::db_panic);
    
    if (err != 0) 
    {
        log_crit("DB: %s, cannot set panic call", db_strerror(err));
        return DS_ERR;
    }
    
    init_ = true;

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
                               
    ASSERT(init_);

    // grab a new database handle
    err = db_create(&db, dbenv_, 0);
    if (err != 0) {
        log_err("error creating database handle: %s", db_strerror(err));
        return DS_ERR;
    }
    
    // calculate the db type and creation flags
    db_flags = 0;
    
    if (flags & DS_CREATE) {
        db_flags |= DB_CREATE;

        if (flags & DS_EXCL) {
            db_flags |= DB_EXCL;
        }

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

    } else {
        db_type = DB_UNKNOWN;
    }

    if (sharefile_) {
        oasys::StaticStringBuffer<128> dbfile("%s.db", db_name_.c_str());
        err = db->open(db, NO_TX, dbfile.c_str(), name.c_str(),
                       db_type, db_flags, 0);
    } else {
        oasys::StaticStringBuffer<128> dbname("%s-%s.db",
                                              db_name_.c_str(), name.c_str());
        err = db->open(db, NO_TX, dbname.c_str(), NULL,
                       db_type, db_flags, 0);
    }
        
    if (err == ENOENT)
    {
        log_debug("get_table -- notfound database %s", name.c_str());
        db->close(db, 0);
        return DS_NOTFOUND;
    }
    else if (err == EEXIST)
    {
        log_debug("get_table -- already existing database %s", name.c_str());
        db->close(db, 0);
        return DS_EXISTS;
    }
    else if (err != 0)
    {
        log_err("DB internal error in get_table: %s", db_strerror(err));
        db->close(db, 0);
        return DS_ERR;
    }

    if (db_type == DB_UNKNOWN) {
        err = db->get_type(db, &db_type);
        if (err != 0) {
            log_err("DB internal error in get_type: %s", db_strerror(err));
            db->close(db, 0);
            return DS_ERR;
        }
    }
    
    log_debug("get_table -- opened table %s type %d", name.c_str(), db_type);

    *table = new BerkeleyDBTable(this, name, (flags & DS_MULTITYPE), db, db_type);

    return 0;
}


int
BerkeleyDBStore::del_table(const std::string& name)
{
    int err;
    
    ASSERT(init_);

    if (ref_count_[name] != 0)
    {
        log_info("Trying to delete table %s with %d refs still on it",
                 name.c_str(), ref_count_[name]);
        
        return DS_BUSY;
    }

    log_info("deleting table %s", name.c_str());

    if (sharefile_) {
        oasys::StaticStringBuffer<128> dbfile("%s.db", db_name_.c_str());
        err = dbenv_->dbremove(dbenv_, NO_TX, dbfile.c_str(), name.c_str(), 0);
    } else {
        oasys::StaticStringBuffer<128> dbfile("%s-%s.db",
                                              db_name_.c_str(), name.c_str());
        err = dbenv_->dbremove(dbenv_, NO_TX, dbfile.c_str(), NULL, 0);
    }

    if (err != 0) {
        log_err("DB: del_table %s", db_strerror(err));

        if (err == ENOENT) 
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
    
    ASSERT(init_);

    err = db_create(&db, dbenv_, 0);
    if (err != 0) {
        log_err("Can't create db pointer");
        return DS_ERR;
    }
    
    err = db->open(db, NO_TX, db_name_.c_str(), 
                   NULL, DB_UNKNOWN, DB_RDONLY, 0);
    if (err != 0) {
        log_err("unable to open metatable - DB: %s", db_strerror(err));
        return DS_ERR;
    }

    DBTYPE type;
    err = db->get_type(db, &type);
    if (err != 0) {
        log_err("unable to get metatable type - DB: %s", db_strerror(err));
        return DS_ERR;
    }
    
    *table = new BerkeleyDBTable(this, META_TABLE_NAME, false, db, type);
    
    return 0;
}

int
BerkeleyDBStore::acquire_table(const std::string& table)
{
    ASSERT(init_);

    ++ref_count_[table];
    ASSERT(ref_count_[table] >= 0);

    log_debug("table %s, +refcount=%d", table.c_str(), ref_count_[table]);

    return ref_count_[table];
}

int
BerkeleyDBStore::release_table(const std::string& table)
{
    ASSERT(init_);

    --ref_count_[table];
    ASSERT(ref_count_[table] >= 0);

    log_debug("table %s, -refcount=%d", table.c_str(), ref_count_[table]);

    return ref_count_[table];
}

#if DB_VERSION_MINOR >= 3
void
BerkeleyDBStore::db_errcall(const DB_ENV* dbenv,
                            const char* errpfx,
                            const char* msg)
{
    __log_err("/storage/berkeleydb", "DB internal error: %s", msg);
}

#else

void
BerkeleyDBStore::db_errcall(const char* errpfx, char* msg)
{
    __log_err("/storage/berkeleydb", "DB internal error: %s", msg);
}

#endif

void
BerkeleyDBStore::db_panic(DB_ENV* dbenv, int errval)
{
    PANIC("fatal berkeley DB internal error: %s", db_strerror(errval));
}

/******************************************************************************
 *
 * BerkeleyDBTable
 *
 *****************************************************************************/
BerkeleyDBTable::BerkeleyDBTable(BerkeleyDBStore* store,
                                 std::string name, bool multitype,
                                 DB* db, DBTYPE db_type)
    : DurableTableImpl(name, multitype),
      db_(db), db_type_(db_type), store_(store)
{
    logpathf("/berkeleydb/table(%s)", name.c_str());
    store_->acquire_table(name);
}


BerkeleyDBTable::~BerkeleyDBTable() 
{
    // Note: If we are to multithread access to the same table, this
    // will have potential concurrency problems, because close can
    // only happen if no other instance of Db is around.
    store_->release_table(name());

    log_debug("closing db %s", name());
    db_->close(db_, 0); // XXX/bowei - not sure about comment above
    db_ = NULL;
}

int 
BerkeleyDBTable::get(const SerializableObject& key, 
                     SerializableObject*       data)
{
    ScratchBuffer<u_char*, 256> key_buf;
    size_t key_buf_len = flatten(key, &key_buf);
    ASSERT(key_buf_len != 0);

    DBT k, d;
    bzero(&d, sizeof(d));

    MAKE_DBT(k, key_buf.buf(), key_buf_len);

    int err = db_->get(db_, NO_TX, &k, &d, 0);
     
    if (err == DB_NOTFOUND) 
    {
        return DS_NOTFOUND;
    }
    else if (err != 0)
    {
        log_err("DB: %s", db_strerror(err));
        return DS_ERR;
    }

    // if this is a multitype table we need to skip over the typecode
    // in the flattened buffer
    size_t typecode_sz = 0;
    if (multitype_) {
        TypeCollection::TypeCode_t typecode;
        typecode_sz = MarshalSize::get_size(&typecode);
    }

    u_char* bp = (u_char*)d.data + typecode_sz;
    size_t  sz = d.size - typecode_sz;
    
    Unmarshal unmarshaller(Serialize::CONTEXT_LOCAL, bp, sz);
    
    if (unmarshaller.action(data) != 0) {
        log_err("DB: error unserializing data object");
        return DS_ERR;
    }

    return 0;
}

int
BerkeleyDBTable::get_typecode(const SerializableObject& key,
                              TypeCollection::TypeCode_t* typecode)
{
    ScratchBuffer<u_char*, 256> key_buf;
    size_t key_buf_len = flatten(key, &key_buf);
    if (key_buf_len == 0) 
    {
        log_err("zero or too long key length");
        return DS_ERR;
    }

    DBT k, d;
    bzero(&d, sizeof(d));

    MAKE_DBT(k, key_buf.buf(), key_buf_len);

    int err = db_->get(db_, NO_TX, &k, &d, 0);
     
    if (err == DB_NOTFOUND) 
    {
        return DS_NOTFOUND;
    }
    else if (err != 0)
    {
        log_err("DB: %s", db_strerror(err));
        return DS_ERR;
    }

    u_char* bp = (u_char*)d.data;
    size_t  sz = d.size;

    Builder b;
    UIntShim type_shim(b);

    Unmarshal unmarshaller(Serialize::CONTEXT_LOCAL, bp, sz);

    if (unmarshaller.action(&type_shim) != 0) {
        log_err("DB: error unserializing type code");
        return DS_ERR;
    }

    *typecode = type_shim.value();

    return DS_OK;
}


int 
BerkeleyDBTable::put(const SerializableObject& key,
                     TypeCollection::TypeCode_t typecode,
                     const SerializableObject* data,
                     int                       flags)
{
    ScratchBuffer<u_char*, 256> key_buf;
    size_t key_buf_len = flatten(key, &key_buf);
    DBT k, d;
    int err;

    // flatten and fill in the key
    MAKE_DBT(k, key_buf.buf(), key_buf_len);

    // if the caller does not want to create new entries, first do a
    // db get to see if the key already exists
    if ((flags & DS_CREATE) == 0) {
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
    size_t object_sz = sizer.size();

    // and the size of the type code (if multitype)
    size_t typecode_sz = 0;
    if (multitype_) {
        typecode_sz = MarshalSize::get_size(&typecode);
    }
    
    log_debug("put: serializing %u byte object (plus %u byte typecode)",
              (u_int)object_sz, typecode_sz);
    
    { // lock
        ScopeLock lock(&scratch_mutex_, "BerkeleyDBStore::put");
    
        u_char* buf = scratch_.buf(typecode_sz + object_sz);
        
        // if we're a multitype table, marshal the type code
        if (multitype_) 
        {
            Marshal typemarshal(Serialize::CONTEXT_LOCAL, buf, typecode_sz);
            UIntShim type_shim(typecode);
            
            if (typemarshal.action(&type_shim) != 0) {
                log_err("error serializing type code");
                return DS_ERR;
            }
        }
        
        Marshal m(Serialize::CONTEXT_LOCAL, buf + typecode_sz, object_sz);
	if (m.action(data) != 0) {
            log_err("error serializing data object");
            return DS_ERR;
        }

        MAKE_DBT(d, buf, typecode_sz + object_sz);

        int db_flags = 0;
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
    if (key_buf_len == 0) 
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

size_t
BerkeleyDBTable::size()
{
    int err;
    int flags = 0;

    union {
        void* ptr;
        struct __db_bt_stat* btree_stats;
        struct __db_h_stat*  hash_stats;
    } stats;

    stats.ptr = 0;
    
#if ((DB_VERSION_MAJOR == 4) && (DB_VERSION_MINOR == 2))
    err = db_->stat(db_, &stats.ptr, flags);
#else
    err = db_->stat(db_, NO_TX, &stats.ptr, flags);
#endif
    if (err != 0) {
        log_crit("error in DB::stat: %d", errno);
        ASSERT(stats.ptr == 0);
        return 0;
    }
    
    ASSERT(stats.ptr != 0);

    size_t ret;
    
    switch(db_type_) {
    case DB_BTREE:
        ret = stats.btree_stats->bt_nkeys;
        break;
    case DB_HASH:
        ret = stats.hash_stats->hash_nkeys;
        break;
    default:
        PANIC("illegal value for db_type %d", db_type_);
    }

    free(stats.ptr);

    return ret;
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
    if (err == DB_NOTFOUND) 
    {
        return DS_NOTFOUND;
    }
    else if (err != 0)
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
    if (err != 0) {
        log_err("DB: cannot create a DB iterator, err=%s", db_strerror(err));
        cur_ = 0;
    }

    if (cur_)
    {
        valid_ = true;
    }
}

BerkeleyDBIterator::~BerkeleyDBIterator()
{
    valid_ = false;
    if (cur_) 
    {
        int err = cur_->c_close(cur_);

        if (err != 0) {
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
    if (!valid_) return DS_ERR;

    *key = key_.data;
    *len = key_.size;

    return 0;
}

int 
BerkeleyDBIterator::raw_data(void** data, size_t* len)
{
    if (!valid_) return DS_ERR;

    *data = data_.data;
    *len  = data_.size;

    return 0;
}

} // namespace oasys

