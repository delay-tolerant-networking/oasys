#ifndef __BERKELEY_TABLE_STORE_H__
#define __BERKELEY_TABLE_STORE_H__

#include <map>
#include <db.h>

#include "tclcmd/TclCommand.h"
#include "debug/Logger.h"
#include "thread/Mutex.h"
#include "util/ScratchBuffer.h"

#include "DurableTable.h"

namespace oasys {

// forward decls
class BerkeleyTable;

/**
 * Interface for the generic datastore
 */
class BerkeleyStore : public DurableTableStore, public Logger {
    friend class BerkeleyTable;

public:
    /** 
     * Do initialization. Must do this once before you try to obtain
     * an instance of this object. This is separated out from the
     * instance() call because we want to have control when the
     * database is initialized.
     */
    static void init();

    ~BerkeleyStore();

    /// @{ virtual from DataStore
    virtual int new_table(DurableTable** table);
    virtual int del_table(DurableTableId id);
    virtual int get_table(DurableTableId id, DurableTable** table);
    virtual int get_meta_table(DurableTable** table);
    /// @}

private:
    FILE*       err_log_;     ///< db err log file
    std::string db_name_;     ///< Name of the database file
    DB_ENV*     dbenv_;       ///< database environment for all tables

    Mutex next_id_mutex_;
    DurableTableId next_id_;  ///< next table id to hand out

    Mutex ref_count_mutex_;
    typedef std::map<int, int> RefCountMap;
    RefCountMap ref_count_;   ///< Ref. count for open tables.

    /// Id the represents the metatable of tables
    static const DurableTableId META_TABLE_ID = -1;

    /// Constructor - protected for singleton
    BerkeleyStore();
    
    /// @{ Changes the ref count on the tables, used by BerkeleyTable 
    int acquire_table(DurableTableId id);
    int release_table(DurableTableId id);
    /// @}

    /// Get the string name of a table
    std::string get_name(int id);

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
class BerkeleyTable : public DurableTable, public Logger {
    friend class BerkeleyStore;
    friend class BerkeleyTableItr;

public:
    ~BerkeleyTable();

    /// @{ virtual from DurableTable 
    int get(const SerializableObject& key, SerializableObject* data);
    int put(const SerializableObject& key, const SerializableObject* data);
    int del(const SerializableObject& key);
    /// @}

private:
    /**
     * Only DataStore can create DsTables
     */
    BerkeleyTable(DurableTableId id, DB* db);

    /**
     * Helper method for flattening keys from the key objects.
     */
    size_t flatten_key(const SerializableObject& key, 
                    u_char* key_buf, size_t size);

    DB* db_;

    Mutex scratch_mutex_;   ///< This may cause performance problems.
    ScratchBuffer scratch_;
};

class BerkeleyTableItr : public DurableTableItr, public Logger {
public:
    /** Create an iterator for table t */
    BerkeleyTableItr(DurableTable* t);

    /** UNIMPLEMENTED */
    BerkeleyTableItr(DurableTable* t, void* k, int len);

    ~BerkeleyTableItr();
    
    /// @{ virtual from DurableTableItr
    int next();
    int get(SerializableObject* obj);
    /// @}


protected:
    DBC* cur_;                  ///< Current database cursor.
    bool valid_;                ///< Status of the iterator.

    DBT key_;
    DBT data_;
};


/**
 * Data store configuration module.
 */
class BerkeleyStoreCommand : public oasys::AutoTclCommand { 
    friend class BerkeleyStore;

public:
    BerkeleyStoreCommand();

    static BerkeleyStoreCommand* instance() 
    {
	if(instance_ == 0)
	{
	    instance_ = new BerkeleyStoreCommand();
	}

        return instance_;
    }
    
    void bind_vars();

    virtual int exec(int argc, const char** argv, Tcl_Interp* interp);

public:
    bool        tidy_db_;	//!< tidy up the db on init
    int         tidy_wait_;	//!< num seconds to wait before tidying
    std::string dir_;		//!< Database directory
    std::string db_name_;       //!< Database name
    std::string err_log_;	//!< Error log

private:
    static BerkeleyStoreCommand* instance_;

    BerkeleyStore* ds_;      //!< datastore configuration is attached to
};

}; // namespace oasys

#endif //__BERKELEY_TABLE_STORE_H__
