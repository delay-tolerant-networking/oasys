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

#ifndef __BERKELEY_TABLE_STORE_H__
#define __BERKELEY_TABLE_STORE_H__

#include "config.h"
#include <map>
#include <db.h>

#if DB_VERSION_MAJOR != 4
#error "must use Berkeley DB major version 4"
#endif

#include "../debug/Logger.h"
#include "../thread/Mutex.h"
#include "../thread/SpinLock.h"
#include "../thread/Timer.h"

#include "DurableStore.h"

namespace oasys {

// forward decls
class BerkeleyDBStore;
class BerkeleyDBTable;
class BerkeleyDBIterator;
class StorageConfig;

/**
 * Interface for the generic datastore
 */
class BerkeleyDBStore : public DurableStoreImpl {
    friend class BerkeleyDBTable;

public:
    BerkeleyDBStore();

    // Can't copy or =, don't implement these
    BerkeleyDBStore& operator=(const BerkeleyDBStore&);
    BerkeleyDBStore(const BerkeleyDBStore&);

    ~BerkeleyDBStore();

    //! @{ Virtual from DurableStoreImpl
    //! Initialize BerkeleyDBStore
    int init(StorageConfig* cfg);

    int get_table(DurableTableImpl** table,
                  const std::string& name,
                  int                flags,
                  PrototypeVector&   prototypes);

    int del_table(const std::string& name);
    /// @}

private:
    bool        init_;        //!< Initialized?
    std::string db_name_;     ///< Name of the database file
    DB_ENV*     dbenv_;       ///< database environment for all tables
    bool	sharefile_;   ///< share a single db file

    SpinLock    ref_count_lock_;
    RefCountMap ref_count_;   ///< Ref. count for open tables.

    /// Id that represents the metatable of tables
    static const std::string META_TABLE_NAME;

    /// Get meta-table
    int get_meta_table(BerkeleyDBTable** table);
    
    /// @{ Changes the ref count on the tables, used by
    /// BerkeleyDBTable
    int acquire_table(const std::string& table);
    int release_table(const std::string& table);
    /// @}

    /// DB internal error log callback (unfortunately, the function
    /// signature changed between 4.2 and 4.3)

#if DB_VERSION_MINOR >= 3
    static void db_errcall(const DB_ENV* dbenv,
                           const char* errpfx,
                           const char* msg);
#else
    static void db_errcall(const char* errpfx, char* msg);
#endif
    
    /// DB internal panic callback
    static void db_panic(DB_ENV* dbenv, int errval);

    /**
     * Timer class used to periodically check for deadlocks.
     */
    class DeadlockTimer : public oasys::Timer, public oasys::Logger {
    public:
        DeadlockTimer(const char* logpath, DB_ENV* dbenv, int frequency)
            : Logger(logpath), dbenv_(dbenv), frequency_(frequency) {}

        void reschedule();
        virtual void timeout(struct timeval* now);

    protected:
        DB_ENV* dbenv_;
        int     frequency_;
    };

    DeadlockTimer* deadlock_timer_;
};

/**
 * Object that encapsulates a single table. Multiple instances of
 * this object represent multiple uses of the same table.
 */
class BerkeleyDBTable : public DurableTableImpl, public Logger {
    friend class BerkeleyDBStore;
    friend class BerkeleyDBIterator;

public:
    ~BerkeleyDBTable();

    /// @{ virtual from DurableTableInpl
    int get(const SerializableObject& key,
            SerializableObject* data);
    
    int get(const SerializableObject& key,
            SerializableObject** data,
            TypeCollection::Allocator_t allocator);
    
    int put(const SerializableObject& key,
            TypeCollection::TypeCode_t typecode,
            const SerializableObject* data,
            int flags);
    
    int del(const SerializableObject& key);

    size_t size() const;
    
    DurableIterator* iter();
    /// @}

private:
    DB*              db_;
    DBTYPE	     db_type_;
    BerkeleyDBStore* store_;

    //! Only BerkeleyDBStore can create BerkeleyDBTables
    BerkeleyDBTable(BerkeleyDBStore* store, 
                    std::string name, bool multitype,
                    DB* db, DBTYPE type);

    /// Whether a specific key exists in the table.
    int key_exists(const void* key, size_t key_len);
};

/**
 * Wrapper around a DBT that correctly handles memory management.
 */
class DBTRef {
public:
    /// Initialize an empty key with the DB_DBT_REALLOC flag
    DBTRef()
    {
        bzero(&dbt_, sizeof(dbt_));
        dbt_.flags = DB_DBT_REALLOC;
    }

    /// Initialize a key with the given data/len and the
    /// DB_DBT_USERMEM flag
    DBTRef(void* data, size_t size)
    {
        bzero(&dbt_, sizeof(dbt_));
        dbt_.data  = data;
        dbt_.size  = size;
        dbt_.flags = DB_DBT_USERMEM;
    }

    /// If any data was malloc'd in the key, free it
    ~DBTRef()
    {
        if (dbt_.flags == DB_DBT_MALLOC ||
            dbt_.flags == DB_DBT_REALLOC)
        {
            if (dbt_.data != NULL) {
                free(dbt_.data);
                dbt_.data = NULL;
            }
        }
    }

    /// Return a pointer to the underlying DBT structure
    DBT* dbt() { return &dbt_; }

    /// Convenience operator overload
    DBT* operator->() { return &dbt_; }

protected:
    DBT dbt_;
};

/**
 * Iterator class for Berkeley DB tables.
 */
class BerkeleyDBIterator : public DurableIterator, public Logger {
    friend class BerkeleyDBTable;

private:
    /**
     * Create an iterator for table t. These should not be called
     * except by BerkeleyDBTable.
     */
    BerkeleyDBIterator(BerkeleyDBTable* t);

public:
    virtual ~BerkeleyDBIterator();
    
    /// @{ Obtain the raw byte representations of the key and data.
    // Buffers are only valid until the next invocation of the
    // iterator.
    int raw_key(void** key, size_t* len);
    int raw_data(void** data, size_t* len);
    /// @}
    
    /// @{ virtual from DurableIteratorImpl
    int next();
    int get(SerializableObject* key);
    /// @}

protected:
    DBC* cur_;          ///< Current database cursor
    bool valid_;        ///< Status of the iterator

    DBTRef key_;	///< Current element key
    DBTRef data_;	///< Current element data
};

}; // namespace oasys

#endif //__BERKELEY_TABLE_STORE_H__
