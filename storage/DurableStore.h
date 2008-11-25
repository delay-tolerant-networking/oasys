/*
 *    Copyright 2004-2006 Intel Corporation
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


#ifndef __OASYS_DURABLE_STORE_H__
#define __OASYS_DURABLE_STORE_H__

#include <list>
#include <string>

#include "../debug/Log.h"
#include "../debug/DebugUtils.h"

#include "../serialize/Serialize.h"
#include "../serialize/MarshalSerialize.h"
#include "../serialize/StringSerialize.h"
#include "../serialize/TypeCollection.h"

#include "../thread/SpinLock.h"

#include "../util/LRUList.h"
#include "../util/StringUtils.h"
#include "../util/ScratchBuffer.h"

#include "DurableStoreKey.h"

namespace oasys {

// forward decls
class DurableStore;
class DurableStoreImpl;
template <typename _Type> class DurableTable;
template <typename _Type> class SingleTypeDurableTable;
template <typename _Type, typename _Collection> class MultiTypeDurableTable;
template <typename _Type> class DurableObjectCache;
class DurableTableImpl;
class DurableIterator;

/*!
 * Enumeration for error return codes from the datastore functions
 */
enum DurableStoreResult_t {
    DS_OK        = 0,           ///< Success
    DS_NOTFOUND  = -1,          ///< Database element not found.
    DS_BUFSIZE   = -2,          ///< Buffer too small.
    DS_BUSY      = -3,          ///< Table is still open, can't delete.
    DS_EXISTS    = -4,          ///< Key already exists
    DS_BADTYPE   = -5,          ///< Error in type collection
    DS_ERR       = -1000,       ///< XXX/bowei placeholder for now
};

/**
 * Pretty print for durable store errors
 */
const char* durable_strerror(int result);

/*!
 * Enumeration for flags to the datastore functions.
 */
enum DurableStoreFlags_t {
    DS_CREATE    = 1 << 0,
    DS_EXCL      = 1 << 1,
    DS_MULTITYPE = 1 << 2,

    // Berkeley DB Specific flags
    DS_HASH      = 1 << 10,
    DS_BTREE     = 1 << 11,
};

// Pull in the various related class definitions (and template class
// implementations) after the above declarations
#define  __OASYS_DURABLE_STORE_INTERNAL_HEADER__
#include "DurableStoreImpl.h"
#include "DurableIterator.h"
#include "DurableTable.h"
#include "DurableObjectCache.h"
#include "DurableTable.tcc"
#include "DurableObjectCache.tcc"
#undef   __OASYS_DURABLE_STORE_INTERNAL_HEADER__

/**
 * Interface for the generic datastore.
 */
class DurableStore : public Logger {
public:
    /*!
     * Initialize the DurableStore object. Note that create_store()
     * must be called before it can be used.
     */
    DurableStore(const char* logpath)
        : Logger("DurableStore", "%s", logpath), impl_(0)
    { 
    }

    /*!
     * Shut down the durable store.
     */
    ~DurableStore();

    /*!
     * Creation function for creating the right kind of database
     * backend implementation for a given StorageConfig.
     *
     * Also initializes the store and returns whether or not the store
     * was cleanly shut down in the previous run.
     */
    int create_store(const StorageConfig& config, 
                     bool* clean_shutdown = NULL);

    //! Return the implementation pointer.
    DurableStoreImpl* impl() { return impl_; }

    /**
     * Get a new handle on a single-type table.
     *
     * @param table      Pointer to the table to be created
     * @param table_name Name of the table
     * @param flags      Options for creating the table
     * @param cache      Optional cache for the table
     *
     * @return DS_OK, DS_NOTFOUND, DS_EXISTS, DS_ERR
     */
    template <typename _DataType>
    int get_table(SingleTypeDurableTable<_DataType>** table,
                  std::string         table_name,
                  int                 flags,
                  DurableObjectCache<_DataType>* cache = NULL);

    /**
     * Get a new handle on a table.
     *
     * @param table      Pointer to the table to be created
     * @param table_name Name of the table
     * @param flags      Options for creating the table
     * @param cache      Optional cache for the table
     * @return DS_OK, DS_NOTFOUND, DS_EXISTS, DS_ERR
     */
    template <typename _BaseType, typename _Collection>
    int get_table(MultiTypeDurableTable<_BaseType, _Collection>** table,
                  std::string         table_name,
                  int                 flags,
                  DurableObjectCache<_BaseType>* cache = NULL);

    /**
     * Get a new handle on a table.
     *
     * @param table      Pointer to the table to be created
     * @param flags      Options for creating the table
     * @param table_name Name of the table
     * @param cache      Optional cache for the table
     * @return DS_OK, DS_NOTFOUND, DS_EXISTS, DS_ERR
     */
    int get_table(StaticTypedDurableTable** table, 
                  std::string               table_name,
                  int                       flags,
                  DurableObjectCache< SerializableObject >* cache = NULL);

    /*!
     * Delete the table (by name) from the datastore.
     */
    int del_table(std::string table_name);

    /*!
     * Retrieve a list of all of the tables in the database. 
     *
     * @param table_list Vector will be filled with list of all of the
     *     table names.
     * @return DS_OK, DS_ERR
     */
    int get_table_names(StringVector* table_names);

    /*!
     * Get descriptive information about the store.
     *
     * @return Text string describing storage configuration.
     */
    std::string get_info() const;

private:
    /**
     * Typedef for the list of objects passed to the implementation to
     * initialize the table.
     */
    typedef DurableStoreImpl::PrototypeVector PrototypeVector;
    
    //! The copy constructor should never be called.
    DurableStore(const DurableStore& other);

    DurableStoreImpl*    impl_;		///< the storage implementation

    std::string clean_shutdown_file_;	///< path to the special shutdown file
};

#include "DurableStore.tcc"

} // namespace oasys

#endif // __OASYS_DURABLE_STORE_H__
