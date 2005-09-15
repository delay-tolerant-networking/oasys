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

#ifndef __OASYS_DURABLE_STORE_H__
#define __OASYS_DURABLE_STORE_H__

#include <list>
#include <string>
#include "../debug/Log.h"
#include "../debug/DebugUtils.h"
#include "../serialize/Serialize.h"
#include "../serialize/StringSerialize.h"
#include "../serialize/TypeCollection.h"
#include "../thread/SpinLock.h"
#include "../util/LRUList.h"
#include "../util/StringUtils.h"

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
    DS_NOTFOUND  = 1,           ///< Database element not found.
    DS_BUFSIZE   = 2,           ///< Buffer too small.
    DS_BUSY      = 3,           ///< Table is still open, can't delete.
    DS_EXISTS    = 4,           ///< Key already exists
    DS_ERR       = 1000,        ///< XXX/bowei placeholder for now
};

/*!
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
class DurableStore {
    friend class BerkeleyDBStore;

public:
    /*!
     * Create a DurableStore, which needs to be backed by an
     * impl. DurableStore will assume ownership of the impl.
     */
    DurableStore(DurableStoreImpl* impl) : impl_(impl) 
    { 
        ASSERT(impl_ != 0); 
    }

    ~DurableStore() 
    { 
        delete impl_; 
        impl_ = 0; 
    }

    //! Return the implementation pointer.
    DurableStoreImpl* impl() { return impl_; }

    /**
     * Get a new handle on a single-type table.
     *
     * @param flags options for creating the table
     * @param id what the id of the table should be if specified
     * @return DS_OK, DS_NOTFOUND, DS_EXISTS, DS_ERR
     */
    template <typename _DataType>
    int get_table(SingleTypeDurableTable<_DataType>** table,
                  std::string         table_name,
                  int                 flags,
                  DurableObjectCache<_DataType>* cache = NULL);

    /*!
     * Get a new handle on a multi-type table.
     *
     * @param flags options for creating the table
     * @param id what the id of the table should be if specified
     * @return DS_OK, DS_NOTFOUND, DS_EXISTS, DS_ERR
     */
    template <typename _BaseType, typename _Collection>
    int get_table(MultiTypeDurableTable<_BaseType, _Collection>** table,
                  std::string         table_name,
                  int                 flags,
                  DurableObjectCache<_BaseType>* cache = NULL);

    /*!
     * Delete the table (by name) from the datastore.
     */
    int del_table(std::string table_name);

private:
    /**
     * Typedef for the list of objects passed to the implementation to
     * initialize the table.
     */
    typedef DurableStoreImpl::PrototypeVector PrototypeVector;
    
    //! The copy constructor should never be called.
    DurableStore(const DurableStore& other);

    DurableStoreImpl*    impl_;		///< the storage implementation
};

#include "DurableStore.tcc"

} // namespace oasys

#endif // __OASYS_DURABLE_STORE_H__
