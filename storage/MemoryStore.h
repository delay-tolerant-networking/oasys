/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2005 Intel Corporation. All rights reserved. 
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

#ifndef __MEMORY_STORE_H__
#define __MEMORY_STORE_H__

#include "config.h"
#include <map>

#include "../debug/Logger.h"
#include "../thread/SpinLock.h"
#include "../util/ScratchBuffer.h"
#include "../util/StringUtils.h"

#include "DurableStore.h"

namespace oasys {

// forward decls
class MemoryStore;
class MemoryTable;
class MemoryIterator;
class StorageConfig;

/**
 * Object that encapsulates a single table. Multiple instances of
 * this object represent multiple uses of the same table.
 */
class MemoryTable : public DurableTableImpl, public Logger {
    friend class MemoryStore;
    friend class MemoryIterator;

public:
    ~MemoryTable();

    /// @{ virtual from DurableTableInpl
    int get(const SerializableObject& key,
            SerializableObject* data);
    
    int get(const SerializableObject& key,
            SerializableObject** data,
            TypeCollection::Allocator_t allocator);
    
    int put(const SerializableObject& key,
            TypeCollection::TypeCode_t typecode,
            const SerializableObject* data,
            int flags);
    
    int del(const SerializableObject& key);

    size_t size() const;
    
    DurableIterator* iter();
    /// @}

private:
    SpinLock lock_;

    struct Item {
        ScratchBuffer<u_char*>	   key_;
        ScratchBuffer<u_char*>	   data_;
        TypeCollection::TypeCode_t typecode_;
    };

    typedef StringMap<Item*>   ItemMap;
    ItemMap* items_;
    
    oasys::ScratchBuffer<u_char*> scratch_;

    //! Only MemoryStore can create MemoryTables
    MemoryTable(ItemMap* items, const std::string& name, bool multitype);
};

/**
 * Fake durable store that just uses RAM. 
 *
 * N.B: This is not durable unless you have a bunch of NVRAM.
 *
 */
class MemoryStore : public DurableStoreImpl {
    friend class MemoryTable;

public:
    MemoryStore();

    // Can't copy or =, don't implement these
    MemoryStore& operator=(const MemoryStore&);
    MemoryStore(const MemoryStore&);

    ~MemoryStore();

    //! @{ Virtual from DurableStoreImpl
    //! Initialize MemoryStore
    int init(const StorageConfig& cfg);

    int get_table(DurableTableImpl** table,
                  const std::string& name,
                  int                flags,
                  PrototypeVector&   prototypes);

    int del_table(const std::string& name);
    int get_table_names(StringVector* names);
    /// @}

private:
    bool init_;        //!< Initialized?

    typedef StringMap<MemoryTable::ItemMap> TableMap;
    TableMap tables_;
};
 
/**
 * Iterator class for Memory tables.
 */
class MemoryIterator : public DurableIterator, public Logger {
    friend class MemoryTable;

private:
    /**
     * Create an iterator for table t. These should not be called
     * except by MemoryTable.
     */
    MemoryIterator(MemoryTable* t);

public:
    virtual ~MemoryIterator();
    
    /// @{ virtual from DurableIteratorImpl
    int next();
    int get(SerializableObject* key);
    /// @}

protected:
    MemoryTable* table_;
    bool first_;
    MemoryTable::ItemMap::iterator iter_;
};

}; // namespace oasys

#endif //__MEMORY_STORE_H__
