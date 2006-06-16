/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2006 Intel Corporation. All rights reserved. 
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
#ifndef _INTERNAL_KEY_DURABLE_TABLE_H_
#define _INTERNAL_KEY_DURABLE_TABLE_H_

#include "../debug/Logger.h"
#include "../debug/DebugUtils.h"
#include "DurableStore.h"
#include "StorageConfig.h"

namespace oasys {

/**
 * Single type durable table adapter interface used to simplify cases
 * in which the data objects to be stored contain at least one field
 * that is the unique identifier and is wrapped in the table by one of
 * the TypeShims.
 *
 * This interface provides simple hooks for add(), get(), del(), and
 * update() that take only a pointer to the class, not a secondary
 * argument that is the id. The class also implements an alternative
 * iterator interface wherein the iterator stores the current element,
 * rather than forcing the caller to have a local temporary.
 *
 * To fulfill the contract required by the template, the stored class
 * must implement a function called durable_key() that returns the
 * unique key value, suitable to be passed to the _ShimType
 * constructor.
 *
 * Finally, to cover the most common (so far) use cases for this
 * class, it implements logging and assertion handlers to cover
 * unexpected cases in the interface, e.g. logging a warning on a call
 * to get() for an id that's not in the table, PANIC on internal
 * database errors, etc.
 */
template <typename _ShimType, typename _KeyType, typename _DataType>
class InternalKeyDurableTable : public Logger {
public:
    InternalKeyDurableTable(const char* classname,
                            const char* logpath,
                            const char* datatype,
                            const char* table_name);

    virtual ~InternalKeyDurableTable();
    
    /**
     * Real initialization method.
     */
    int do_init(const StorageConfig& cfg,
                DurableStore*        store);

    /**
     * Close and flush the table.
     */
    void close();
    
    bool add(_DataType* data);
    
    _DataType* get(_KeyType id);
    
    bool update(_DataType* data);

    bool del(_KeyType id);

    /**
     * STL-style iterator.
     */
    class iterator {
    public:
        typedef class InternalKeyDurableTable<_ShimType,
                                              _KeyType,
                                              _DataType> table_t;

        virtual ~iterator();

        /**
         * Advances the iterator.
         *
         * @return DS_OK, DS_NOTFOUND if no more elements, DS_ERR if
         * an error occurred while iterating.
         */
        int next();

        /**
         * Alternate hook to next() for starting iterating.
         */
        void begin() { next(); }

        /**
         * Return true if iterating is done.
         */
        bool more() { return !done_; }

        /**
         * Accessor for the value.
         */
        _KeyType cur_val() { return cur_val_.value(); }

    private:
        friend class table_t;

        iterator(table_t* table, DurableIterator* iter);

        table_t*  table_;	///< Pointer to the containing table
        DurableIterator* iter_;	///< The underlying iterator
        _ShimType cur_val_;	///< Current field value
        bool      done_;	///< Flag indicating if at end
    };
    
    /**
     * Return a new iterator. The caller has the responsibility of
     * deleting it once done.
     */
    iterator* new_iterator()
    {
        return new iterator(this, table_->itr());
    }

protected:
    SingleTypeDurableTable<_DataType>* table_;
    const char* datatype_;
    const char* table_name_;
};

#include "InternalKeyDurableTable.tcc"

} // namespace oasys

#endif /* _INTERNAL_KEY_DURABLE_TABLE_H_ */
