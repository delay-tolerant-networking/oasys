#ifndef __DURABLE_TABLE_H__
#define __DURABLE_TABLE_H__

#include <new>
#include <string>

#include "db_cxx.h"			// berkeley DB

#include "debug/Debug.h"
#include "debug/Log.h"
#include "tclcmd/TclCommand.h"

namespace oasys {

// forward decls
class DurableTableStore;
class DurableTable;
class DurableTableItr;

/// Enumeration for error return codes from the datastore functions
enum DurableTableResult_t {
    DS_OK        = 0,           ///< Success
    DS_NOTFOUND  = 1,           ///< Database element not found.
    DS_BUFSIZE   = 2,           ///< Buffer too small.
    DS_BUSY      = 3,           ///< Table is still open, can't delete.
    DS_ERR       = 1000,        ///< xxx/bowei placeholder for now
};
typedef int DurableTableId;

/// Interface for the generic datastore
class DurableTableStore {
public:
    /// Get singleton manager instance. Call init to create the
    /// singleton instance.
    static DurableTableStore* instance() 
    {
        ASSERT(instance_);      // use init() please
        return instance_;      
    }

    /// Do initialization. Must do this once before you try to obtain
    /// an instance of this object. This is separated out from the
    /// instance() call because we want to have control when the
    /// database is initialized.
    static void init();

    /// Destructor
    virtual ~DurableTableStore() {}

    /// Create a new table. Caller deletes the pointer.
    virtual int new_table(DurableTable** table) = 0;

    /// Delete (by id) from the datastore
    virtual int del_table(DurableTableId id) = 0;

    /// Get a new table ptr to an id
    virtual int get_table(DurableTableId id, DurableTable** table) = 0;
    
    /// Get meta-table, id = -1.
    virtual int get_meta_table(DurableTable** table) = 0;

protected:
    /// Constructor - protected for singleton
    DurableTableStore();

    static DurableTableStore* instance_; //< singleton instance
};


/// Object that encapsulates a single table.
class DurableTable {
public:
    virtual ~DurableTable() {}

    /// Get the data for key.
    ///
    /// \param key Key to retrieve
    /// \param key_len Key length
    /// \param data Data buffer
    /// \param data_len Length of data buffer. Returned value will be
    /// size of returned data.
    ///
    /// \return DS_OK, DS_NOTFOUND if key is not found, DS_BUFSIZE if
    /// the given buffer is too small to store the data.
    ///
    virtual int get(const void* key, int key_len, void* data, int *data_len) = 0;

    /// Put data for key in the database
    ///
    /// \param key Key to retrieve
    /// \param key_len Key length
    /// \param data Data buffer
    /// \param data_len Length of data buffer
    /// \return DS_OK, DS_ERR // XXX/bowei
    ///
    virtual int put(const void* key, int key_len, const void* data, int data_len) = 0;

    /// Delete a (key,data) pair from the database
    ///
    /// \return DS_OK, DS_NOTFOUND if key is not found
    ///
    virtual int del(const void* key, int key_len) = 0;

    /// Return table id.
    DurableTableId id() { return id_; }

protected:
    DurableTableId id_;
};


/// Table iterator object. Just like Java. Note: It is important that
/// iterators do NOT outlive the tables they point into.
class DurableTableItr {
public:
    /// Destructor.
    virtual ~DurableTableItr() {};

    /// Advance the pointer. An initialized iterator will be pointing
    /// right before the first element in the list, so iteration code
    /// will always be:
    ///
    /// \code
    /// DurableTableItr i(table);
    /// while(i.next() == 0) {
    ///    // ... do stuff
    /// }
    /// \endcode
    ///
    /// \return DS_OK, DS_NOTFOUND if no more elements, DS_ERR if an
    /// error occurred while iterating.
    virtual int next() = 0;

    /// Return a buffer to the key.
    virtual const void* key() = 0;

    /// Return the length of the key buffer.
    virtual int key_len() = 0;
    
    /// Return a pointer to the data buf.
    virtual const void* data() = 0;
    
    /// Return the length fo the data buffer.
    virtual int data_len() = 0;
};

/*
/// data store configuration module.
class DataStoreCommand : public oasys::AutoTclCommand { 
    friend class DataStore;

public:
    DataStoreCommand();

    static DataStoreCommand* instance() {
	if(instance_ == 0)
	{
	    instance_ = new DataStoreCommand();
	}

        return instance_;
    }

    void bind_vars();

    virtual int exec(int argc, const char** argv, Tcl_Interp* interp);
    
protected:
    DataStore* ds_;      //!< datastore configuration is attached to

public:
    bool tidy_db_;		//!< tidy up the db on init
    int tidy_wait_;		//!< num seconds to wait before tidying
    std::string dir_;		//!< Database directory
    std::string errlog_;	//!< Error log

private:
    static DataStoreCommand* instance_;
};
*/

}; // oasys

#endif // __DURABLE_TABLE_H__
