/*
 *    Copyright 2004-2006 Intel Corporation
 *    Copyright 2011 Mitre Corporation
 *    Copyright 2011 Trinity College Dublin
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */


#ifndef __ODBC_TABLE_STORE_H__
#define __ODBC_TABLE_STORE_H__

#ifndef OASYS_CONFIG_STATE
#error "MUST INCLUDE oasys-config.h before including this file"
#endif

#if LIBODBC_ENABLED

#include <stdio.h>              //glr
#include <stdlib.h>             //glr
#include <string.h>             //glr
#include <sys/time.h>           //glr

#include <map>

//glr includes for standard ODBC headers typically in /usr/include
#include <sql.h>                //glr
#include <sqlext.h>             //glr
#include <sqltypes.h>           //glr
#include <sqlucode.h>           //glr

#include "../debug/Logger.h"
#include "../thread/Mutex.h"
#include "../thread/SpinLock.h"
#include "../thread/Timer.h"

#include "DurableStore.h"

#define DATA_MAX_SIZE (1 * 1024 * 1024) //1M - increase this and table create for larger buffers
//#define SQLITE_DB_CONSTRAINT  19        // Abort due to contraint violation

struct ODBC_dbenv               //glr
{
//glr standard ODBC DB handles (environment, database connection, statement)
    SQLHENV m_henv;
    SQLHDBC m_hdbc;
    SQLHSTMT hstmt;
    SQLHSTMT trans_hstmt;
    //SQLHSTMT iterator_hstmt;
    char table_name[128];
};

struct __my_dbt
{                               //glr struct borrowed from Berkeley DB's /usr/include/db4/db.h
    void *data;                 /* Key/data */
    u_int32_t size;             /* key/data length */
#ifndef DB_DBT_MALLOC
#define DB_DBT_MALLOC           0x004   /* Return in malloc'd memory. */
#endif
#ifndef DB_DBT_REALLOC
#define DB_DBT_REALLOC          0x010   /* Return in realloc'd memory. */
#endif
#ifndef DB_DBT_USERMEM
#define DB_DBT_USERMEM          0x020   /* Return in user's memory. */
#endif
    u_int32_t flags;
};

/// List of names of 'DTN standard' tables in ODC databases
/// which have two columns (the_key, the_data).
typedef char* odbc_table_name_t;
#if 0
static odbc_table_name_t odbc_table_name_list[] =
    { "bundles", "prophet", "links", "registrations", NULL };
#endif
namespace oasys
{

// forward decls
    class ODBCDBStore;
    class ODBCDBTable;
    class ODBCDBIteratorDBIterator;
    class StorageConfig;

/**
 * Interface for the generic ODBC datastore
 * Caters for any number of different types of database engine using
 * a ODBC driver manager, the relevant ODBC driver for the selected
 * database engine.
 */
class ODBCDBStore:public DurableStoreImpl
{
    friend class ODBCDBTable;
    friend class ODBCDBIterator;

    public:
        ODBCDBStore (const char* derived_classname, const char *logpath);

        // Can't copy or =, don't implement these
        ODBCDBStore & operator= (const ODBCDBStore &);
        ODBCDBStore (const ODBCDBStore &);

        ~ODBCDBStore ();

        //! @{ Virtual from DurableStoreImpl
        //! Initialize derivative class - must be implemented in derived
        //! classes for dpecific database engines.
        // int init (const StorageConfig & cfg);

        int get_table (DurableTableImpl ** table,
                       const std::string & name,
                       int flags, PrototypeVector & prototypes);

        int del_table (const std::string & name);
        int get_table_names (StringVector * names);
        std::string get_info () const;

        int beginTransaction (void **txid);
        int endTransaction (void *txid, bool be_durable);
        void *getUnderlying ();

        /// @}

    protected:
        //! @{ Common pieces of initialization code.
        //! Factored out of specific driver classes.
        DurableStoreResult_t connect_to_database(const StorageConfig & cfg);
        DurableStoreResult_t create_tables();

        /// @}

        bool init_;             ///< Initialized?
        std::string db_name_;   ///< Data source name (overload purpose of dbname in StorageConfig)
        ODBC_dbenv *dbenv_;     ///< database environment for all tables
        ODBC_dbenv base_dbenv_; //glr
        bool sharefile_;        ///< share a single db file
        bool auto_commit_;       /// True if auto-commit is on
        SQLRETURN sqlRC;        //glr
        RefCountMap ref_count_; ///< Ref. count for open tables.

        /// Id that represents the metatable of tables
        static const std::string META_TABLE_NAME;

        /// List of names of 'DTN standard' tables in ODC databases
        /// which have two columns (the_key, the_data).
        static const odbc_table_name_t odbc_table_name_list[];

        /**
         * Timer class used to periodically check for deadlocks.
         */
        class DeadlockTimer:public oasys::Timer, public oasys::Logger
        {
            public:
                DeadlockTimer (const char *logbase, ODBC_dbenv * dbenv,
                               int
                               frequency):Logger ("ODBCDBStore::DeadlockTimer",
                                                  "%s/%s", logbase,
                                                  "deadlock_timer"), dbenv_ (dbenv),
                frequency_ (frequency) { }

                void reschedule ();
                virtual void timeout (const struct timeval &now);

            protected:
                ODBC_dbenv * dbenv_;
                int frequency_;
        };

        DeadlockTimer *deadlock_timer_;
    private:

       bool serializeAll;            // Serialize all access across all tables
       SpinLock serialization_lock_; // For serializing all access to all tables


       // int parseOdbcIni(const char *dbName, char *fullPath, char *schemaPath);


       SpinLock ref_count_lock_;

       /// Get meta-table
       int get_meta_table (ODBCDBTable ** table);

       /// @{ Changes the ref count on the tables, used by
       /// ODBCDBTable
       int acquire_table (const std::string & table);
       int release_table (const std::string & table);
       /// @}

};

/**
 * Object that encapsulates a single table. Multiple instances of
 * this object represent multiple uses of the same table.
 */
class ODBCDBTable:public DurableTableImpl, public Logger
{
    friend class ODBCDBStore;
    friend class ODBCDBIterator;

    public:
        ~ODBCDBTable ();
        /// @{ virtual from DurableTableImpl
        int get (const SerializableObject & key, SerializableObject * data);
        int get (const SerializableObject & key,
                 SerializableObject ** data,
                 TypeCollection::Allocator_t allocator);

        int realKey(void *k);

        int put (const SerializableObject & key,
                 TypeCollection::TypeCode_t typecode,
                 const SerializableObject * data, int flags);

        int del (const SerializableObject & key);

        size_t size () const;

        int print_error (SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt);

        DurableIterator *itr ();
        /// @}

    private:
        ODBC_dbenv * db_;       //glr
        int db_type_;           //glr
        ODBCDBStore *store_;

        // Lock to ensure that only one thread accessed a particular
        // table at a time.  There is a single SQLHSTMT per table
        // that stores the current command; multiple threads concurrently
        // messing with that is bad.
        SpinLock lock_;

        // Lock to ensure that only one iterator is active at a given
        // time for a given table.  this is not necessarily a
        // constraint of the database but as with the lock above,
        // there's only one SQLHSTMT per table to be used for iterators.
        SpinLock iterator_lock_;

        SQLHSTMT hstmt;
        SQLHSTMT iterator_hstmt;
        
        //! Only ODBCDBStore can create ODBCDBTables
        ODBCDBTable (const char *logpath,
                       ODBCDBStore * store,
                       const std::string & table_name,
                       bool multitype, ODBC_dbenv * db, int type);

        /// Whether a specific key exists in the table.
        int key_exists (const void *key, size_t key_len);
        /**
         * Create an iterator for table t. These should not be called
         * except by ODBCDBTable.
         */
};

/**
 * Iterator class for ODBC DB tables.
 */
class ODBCDBIterator:public DurableIterator, public Logger
{
    friend class ODBCDBTable;

    private:

        ODBCDBIterator (ODBCDBTable * t);

        ODBCDBTable* myTable;

    public:
        virtual ~ ODBCDBIterator ();

        /// @{ Obtain the raw byte representations of the key and data.
        // Buffers are only valid until the next invocation of the
        // iterator.
        int raw_key (void **key, size_t * len);
        int raw_data (void **data, size_t * len);
        /// @}

        /// @{ virtual from DurableIteratorImpl
        int next ();
        int get_key (SerializableObject * key);
        /// @}

    protected:
        SQLHSTMT cur_;        // current stmt handle which controls current cursor
        char cur_table_name[128];
        bool valid_;            ///< Status of the iterator

        __my_dbt key_;          ///< Current element key
        __my_dbt data_;         ///< Current element data
#define MAX_GLOBALS_KEY_LEN 11  //for "global_key" string
        char bound_key_string[MAX_GLOBALS_KEY_LEN];
        SQLINTEGER bound_key_int;       //long int
};

};                              // namespace oasys

#endif // LIBODBC_ENABLED

#endif //__ODBC_TABLE_STORE_H__
