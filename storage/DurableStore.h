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

#include "../debug/Debug.h"
#include "../debug/Log.h"
#include "../serialize/Serialize.h"
#include "../serialize/TypeCollection.h"

namespace oasys {

// forward decls
class DurableStore;
class DurableStoreImpl;
class DurableTable;
template <typename _Type> class SingleTypeDurableTable;
template <typename _Type, typename _Collection> class MultiTypeDurableTable;
class DurableTableImpl;
class DurableIterator;
class DurableObjectCache;

/**
 * Enumeration for error return codes from the datastore functions
 *
 * XXX/bowei - change these names
 */
enum DurableStoreResult_t {
    DS_OK        = 0,           ///< Success
    DS_NOTFOUND  = 1,           ///< Database element not found.
    DS_BUFSIZE   = 2,           ///< Buffer too small.
    DS_BUSY      = 3,           ///< Table is still open, can't delete.
    DS_EXISTS    = 4,           ///< Key already exists
    DS_ERR       = 1000,        ///< xxx/bowei placeholder for now
};

/**
 * Enumeration for flags to the datastore functions.
 */
enum DurableStoreFlags_t {
    DS_CREATE    = 1 << 0,
    DS_EXCL      = 1 << 1,
    DS_HASH      = 1 << 2,
    DS_BTREE     = 1 << 3
};

// Pull in the various related class definitions (and template class
// implementations) after the above declarations
#include "DurableStoreImpl.h"
#include "DurableIterator.h"
#include "DurableTable.h"

/**
 * Interface for the generic datastore.
 */
class DurableStore {
    friend class BerkeleyDBStore;

public:
    /**
     * Get singleton manager instance. Call init to create the
     * singleton instance.
     */
    static inline DurableStore* instance() 
    {
        ASSERT(instance_);      // use init() please
        return instance_;      
    }

    /**
     * Return the implementation pointer.
     */
    DurableStoreImpl* impl()
    {
        return impl_;
    }

    // XXX/bowei should be virtualized
    static void shutdown() {
        delete instance_->impl_;
        delete instance_;
        instance_ = NULL;
    }

    /**
     * Destructor
     */
    virtual ~DurableStore() {}

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
                  DurableObjectCache* cache);

    /**
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
                  DurableObjectCache* cache);

    /**
     * Delete the table (by name) from the datastore.
     */
    int del_table(std::string table_name);

protected:
    /*!
     * Do initialization. This is separated out from the instance()
     * call because we want to have control when the database is
     * initialized.
     * 
     * Subclasses should call this to set the instance_ variable in
     * their own init methods.
     */
    static void init(DurableStoreImpl* impl) {
        ASSERT(instance_ == 0);
        instance_ = new DurableStore(impl);
    }
    
private:
    /**
     * Typedef for the list of objects passed to the implementation to
     * initialize the table.
     */
    typedef DurableStoreImpl::PrototypeVector PrototypeVector;
    
    /**
     * The constructor should only be called by DurableStore::init.
     */
    DurableStore(DurableStoreImpl* impl) : impl_(impl) {}

    /**
     * The copy constructor should never be called.
     */
    DurableStore(const DurableStore& other);

    static DurableStore* instance_; 	///< singleton instance
    DurableStoreImpl*    impl_;		///< the storage implementation
};

#include "DurableStore.tcc"

} // namespace oasys

#endif // __OASYS_DURABLE_STORE_H__
