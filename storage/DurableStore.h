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

#ifndef __DURABLE_TABLE_H__
#define __DURABLE_TABLE_H__

#include <new>
#include <string>

#include "debug/Debug.h"
#include "debug/Log.h"
#include "serialize/Serialize.h"

namespace oasys {

// forward decls
class DurableTableStore;
class DurableTable;
class DurableTableItr;

/**
 * Enumeration for error return codes from the datastore functions
 *
 * XXX/bowei - change these names
 */
enum DurableTableResult_t {
    DS_OK        = 0,           ///< Success
    DS_NOTFOUND  = 1,           ///< Database element not found.
    DS_BUFSIZE   = 2,           ///< Buffer too small.
    DS_BUSY      = 3,           ///< Table is still open, can't delete.
    DS_EXISTS    = 4,           ///< Key already exists
    DS_ERR       = 1000,        ///< xxx/bowei placeholder for now
};

/**
 * Identifier for DurableTables
 */
typedef int DurableTableId;

/**
 * Interface for the generic datastore
 */
class DurableTableStore {
public:
    /** Get singleton manager instance. Call init to create the
     * singleton instance.
     */
    static inline DurableTableStore* instance() 
    {
        ASSERT(instance_);      // use init() please
        return instance_;      
    }

    /**
     * Do initialization. Must do this once before you try to obtain
     * an instance of this object. This is separated out from the
     * instance() call because we want to have control when the
     * database is initialized.
     * 
     * Subclasses should call this to set the instance_ variable in
     * their own init methods.
     */
    static void init(DurableTableStore* instance) {
        ASSERT(instance_ == 0);
        instance_ = instance;
    }

    // XXX/bowei should be virtualized
    static void shutdown() {
        ASSERT(instance_ != 0);
        delete instance_;

        instance_ = 0;
    }

    /**
     * Destructor
     */
    virtual ~DurableTableStore() {}

    /**
     * Create a new table. Caller deletes the pointer.
     * @param id Specifies what the id of the table should be.
     */
    virtual int new_table(DurableTable** table, DurableTableId id = -1) = 0;

    /**
     * Delete (by id) from the datastore
     */
    virtual int del_table(DurableTableId id) = 0;

    /**
     * Get a new table ptr to an id
     */
    virtual int get_table(DurableTableId id, DurableTable** table) = 0;
    
private:
    static DurableTableStore* instance_; //< singleton instance
};

/**
 * Object that encapsulates a single table.
 */
class DurableTable {
public:
    DurableTable(DurableTableId id) : id_(id) {}
    virtual ~DurableTable() {}

    /**
     * Get the data for key.
     *
     * @param key  Key object
     * @param data Data object
     * @return DS_OK, DS_NOTFOUND if key is not found, DS_BUFSIZE if
     * the given buffer is too small to store the data.
     */
    virtual int get(const SerializableObject& key, 
                    SerializableObject* data) = 0;

    /** 
     * Put data for key in the database
     *
     * @param key Key object
     * @param data Data object
     * @return DS_OK, DS_ERR // XXX/bowei
     */
    virtual int put(const SerializableObject& key, 
                    const SerializableObject& data) = 0;

    /**
     * Delete a (key,data) pair from the database
     * @return DS_OK, DS_NOTFOUND if key is not found
     */
    virtual int del(const SerializableObject& key) = 0;

    /**
     * Get an iterator to this table.
     *
     * @param itr Iterator pointer to be set. Caller deletes this
     * pointer.
     */
    virtual int itr(DurableTableItr** itr) = 0;

    /** Return table id. */
    DurableTableId id() { return id_; }

private:
    DurableTableId id_;
};

/**
 * Table iterator object. Just like Java. Note: It is important that
 * iterators do NOT outlive the tables they point into.
 */
class DurableTableItr {
public:
    virtual ~DurableTableItr() {};

    /**
     * Advance the pointer. An initialized iterator will be pointing
     * right before the first element in the list, so iteration code
     * will always be:
     *
     * @code
     * DurableTableItr i(table);
     * while(i.next() == 0) {
     *    // ... do stuff
     * }
     * @endcode
     *
     * @return DS_OK, DS_NOTFOUND if no more elements, DS_ERR if an
     * error occurred while iterating.
     */
    virtual int next() = 0;

    /** Unserialize the data. */
    virtual int get(SerializableObject* key, SerializableObject* data) = 0;
};

}; // namespace oasys

#endif // __DURABLE_TABLE_H__
