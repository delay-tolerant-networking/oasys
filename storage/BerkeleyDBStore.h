#ifndef __BERKELEY_TABLE_STORE_H__
#define __BERKELEY_TABLE_STORE_H__

#include <map>
#include <db.h>

#include "../tclcmd/TclCommand.h"
#include "../debug/Logger.h"
#include "../thread/Mutex.h"
#include "../thread/SpinLock.h"
#include "../util/ScratchBuffer.h"

#include "DurableStore.h"

namespace oasys {

// forward decls
class BerkeleyDBStore;
class BerkeleyDBTable;
class BerkeleyDBIterator;

/**
 * Interface for the generic datastore
 */
class BerkeleyDBStore : public DurableStore, public Logger {
    friend class BerkeleyDBTable;

public:
    ~BerkeleyDBStore();

    /**
     * Do initialization. Must do this once before you try to obtain
     * an instance of this object.
     */
    static void init(const std::string& db_name,
                     const char*        db_dir,
                     int                db_flags,
                     bool               tidy_db,
                     int                tidy_wait);

    /**
     * Shadow the instance method of DurableStore so callees do not
     * need to do the downcast.
     */
    BerkeleyDBStore* instance()
    {
        return static_cast<BerkeleyDBStore*>(DurableStore::instance());
    }

    /**
     * Shutdown BerkeleyDB and free associated resources.
     */
    static void shutdown_for_debug();

    /// @{ Virtual from DurableStore
    virtual int get_table(DurableTable**      table,
                          int                 flags,
                          const std::string&  name,
                          SerializableObject* prototype);
    
    virtual int del_table(std::string& name);
    /// @}

private:
    FILE*       err_log_;     ///< db err log file
    std::string db_name_;     ///< Name of the database file
    DB_ENV*     dbenv_;       ///< database environment for all tables

    SpinLock next_id_lock_;
    DurableTableID next_id_;  ///< next table id to hand out

    SpinLock ref_count_lock_;
    typedef std::map<int, int> RefCountMap;
    RefCountMap ref_count_;   ///< Ref. count for open tables.

    /// Id that represents the metatable of tables
    static const DurableTableID META_TABLE_ID = 1;

    /// Handle on the metatable
    BerkeleyDBTable* metatable_;

    /// Constructor - protected for singleton
    BerkeleyDBStore();
    BerkeleyDBStore(const BerkeleyDBStore&);

    /// Get meta-table
    int get_meta_table(BerkeleyDBTable** table);
    
    /// @{ Changes the ref count on the tables, used by BerkeleyDBTable 
    int acquire_table(DurableTableID id);
    int release_table(DurableTableID id);
    /// @}

    /// Get the string name of a table
    std::string get_name(int id);

    /**
     * real initialization code
     * @return 0 if no error
     */
    int do_init(const std::string& db_name,
                const char*        db_dir,
                int                db_flags,
                bool               tidy_db,
                int                tidy_wait);
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
    
    int put(const SerializableObject& key,
            const SerializableObject* data,
            int flags);
    
    int del(const SerializableObject& key);
    
    DurableIteratorImpl* iter();
    /// @}

private:
    /**
     * Only BerkeleyDBStore can create BerkeleyDBTables
     */
    BerkeleyDBTable(DurableTableID id, DB* db);

    /// Whether a specific key exists in the table.
    int key_exists(const void* key, size_t key_len);

    DB* db_;

    Mutex scratch_mutex_;   ///< This may cause performance problems.
    ScratchBuffer scratch_;
};

/**
 * Iterator class for Berkeley DB tables.
 */
class BerkeleyDBIterator : public DurableIteratorImpl, public Logger {
    friend class BerkeleyDBTable;

private:
    /**
     * Create an iterator for table t. These should not be called
     * except by BerkeleyDBTable.
     */
    BerkeleyDBIterator(BerkeleyDBTable* t);

public:
    ~BerkeleyDBIterator();
    
    /// @{ Obtain the raw byte representations of the key and data.
    // Buffers are only valid until the next invocation of the
    // iterator.
    int raw_key(void** key, size_t* len);
    int raw_data(void** data, size_t* len);
    /// @}
    
    /// @{ virtual from DurableIteratorImpl
    int next(SerializableObject* key);
    /// @}

protected:
    DBC* cur_;          ///< Current database cursor
    bool valid_;        ///< Status of the iterator

    DBT key_;		///< Current element key
    DBT data_;		///< Current element data
};

}; // namespace oasys

#endif //__BERKELEY_TABLE_STORE_H__
