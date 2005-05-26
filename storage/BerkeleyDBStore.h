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
class BerkeleyDBStore : public DurableStoreImpl, public Logger {
    friend class BerkeleyDBTable;

public:
    virtual ~BerkeleyDBStore();

    /**
     * Do initialization. Must do this once before you try to obtain
     * an instance of this object.
     */
    static void init();

    /**
     * Shadow the instance method of DurableStore so callees do not
     * need to do the downcast.
     */
    static BerkeleyDBStore* instance()
    {
        return static_cast<BerkeleyDBStore*>(DurableStore::instance()->impl());
    }

    /**
     * Shutdown BerkeleyDB and free associated resources.
     */
    static void shutdown_for_debug();

    /// @{ Virtual from DurableStore
    int get_table(DurableTableImpl**  table,
                  const std::string&  name,
                  int                 flags,
                  PrototypeVector&    prototypes);
    
    int del_table(const std::string& name);
    /// @}

private:
    FILE*       err_log_;     ///< db err log file
    std::string db_name_;     ///< Name of the database file
    DB_ENV*     dbenv_;       ///< database environment for all tables

    SpinLock ref_count_lock_;
    typedef std::map<std::string, int> RefCountMap;
    RefCountMap ref_count_;   ///< Ref. count for open tables.

    /// Id that represents the metatable of tables
    static const std::string META_TABLE_NAME;

    /// Constructor - protected for singleton
    BerkeleyDBStore();
    BerkeleyDBStore(const BerkeleyDBStore&);

    /// Get meta-table
    int get_meta_table(BerkeleyDBTable** table);
    
    /// @{ Changes the ref count on the tables, used by BerkeleyDBTable 
    int acquire_table(const std::string& table);
    int release_table(const std::string& table);
    /// @}

    /**
     * real initialization code
     * @return 0 if no error
     */
    int do_init();
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
    
    DurableIterator* iter();
    /// @}

private:
    /**
     * Only BerkeleyDBStore can create BerkeleyDBTables
     */
    BerkeleyDBTable(std::string name, DB* db);

    /// Whether a specific key exists in the table.
    int key_exists(const void* key, size_t key_len);

    DB* db_;

    Mutex scratch_mutex_;   ///< This may cause performance problems.
    ScratchBuffer scratch_;
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

    DBT key_;		///< Current element key
    DBT data_;		///< Current element data
};

}; // namespace oasys

#endif //__BERKELEY_TABLE_STORE_H__
