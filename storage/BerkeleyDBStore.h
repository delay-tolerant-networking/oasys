#ifndef __BERKELEY_TABLE_STORE_H__
#define __BERKELEY_TABLE_STORE_H__

#include <map>
#include <db.h>

#include "../tclcmd/TclCommand.h"
#include "../debug/Logger.h"
#include "../thread/Mutex.h"
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
     * an instance of this object. This is separated out from the
     * instance() call because we want to have control when the
     * database is initialized.
     */
    static void init(const std::string& db_name,
                     const char*        db_dir,
                     bool               tidy_db,
                     int                tidy_wait);
    /**
     * Shutdown BerkeleyDB and free associated resources.
     */
    static void shutdown_for_debug();

    /// @{ Virtual from DurableStore
    virtual int new_table(DurableTable** table, DurableTableID new_id = -1);
    virtual int del_table(DurableTableID id);
    virtual int get_table(DurableTableID id, DurableTable** table);
    /// @}

private:
    FILE*       err_log_;     ///< db err log file
    std::string db_name_;     ///< Name of the database file
    DB_ENV*     dbenv_;       ///< database environment for all tables

    Mutex next_id_mutex_;
    DurableTableID next_id_;  ///< next table id to hand out

    Mutex ref_count_mutex_;
    typedef std::map<int, int> RefCountMap;
    RefCountMap ref_count_;   ///< Ref. count for open tables.

    /// Id the represents the metatable of tables
    static const DurableTableID META_TABLE_ID = -1;

    /// Constructor - protected for singleton
    BerkeleyDBStore();

    /// Get meta-table
    virtual int get_meta_table(BerkeleyDBTable** table);
    
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
                bool               tidy_db,
                int                tidy_wait);
};

/**
 * Object that encapsulates a single table. Multiple instances of
 * this object represent multiple uses of the same table.
 */
class BerkeleyDBTable : public DurableTable, public Logger {
    friend class BerkeleyDBStore;
    friend class BerkeleyDBIterator;

public:
    ~BerkeleyDBTable();

    /// @{ virtual from DurableTable 
    int get(const SerializableObject& key, SerializableObject* data);
    int put(const SerializableObject& key, const SerializableObject& data);
    int del(const SerializableObject& key);
    int itr(DurableIterator** itr);
    /// @}

private:
    /**
     * Only DataStore can create DsTables
     */
    BerkeleyDBTable(DurableTableID id, DB* db);

    /// Whether a specific key exists in the table.
    int key_exists(const void* key, size_t key_len);

    /**
     * Helper method for flattening keys from the key objects.
     */
    size_t flatten_key(const SerializableObject& key, 
                       u_char* key_buf, size_t size);

    DB* db_;

    Mutex scratch_mutex_;   ///< This may cause performance problems.
    ScratchBuffer scratch_;
};

class BerkeleyDBIterator : public DurableIterator, public Logger {
    friend class BerkeleyDBTable;
private:
    /** Create an iterator for table t. These should not be called
     * except by BerkeleyDBTable. */
    BerkeleyDBIterator(DurableTable* t);
    BerkeleyDBIterator(DurableTable* t, void* k, int len); // UNIMPLEMENTED for now

public:
    ~BerkeleyDBIterator();
    
    /// @{ Obtain the raw byte representations of the key and data.
    // Buffers are only valid until the next invocation of the
    // iterator.
    int raw_key(void** key, size_t* len);
    int raw_data(void** data, size_t* len);
    /// @}
    
    /// @{ virtual from DurableIterator
    int next();
    int get(SerializableObject* key, SerializableObject* data);
    /// @}


protected:
    DBC* cur_;                  ///< Current database cursor.
    bool valid_;                ///< Status of the iterator.

    DBT key_;
    DBT data_;
};

}; // namespace oasys

#endif //__BERKELEY_TABLE_STORE_H__
