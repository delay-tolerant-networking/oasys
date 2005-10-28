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

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include <debug/DebugUtils.h>
#include <util/StringBuffer.h>
#include <util/Pointers.h>
#include <serialize/MarshalSerialize.h>
#include <serialize/TypeShims.h>

#include "MemoryStore.h"
#include "StorageConfig.h"

namespace oasys {
/******************************************************************************
 *
 * MemoryStore
 *
 *****************************************************************************/

MemoryStore::MemoryStore() 
    : DurableStoreImpl("/storage/memory"),
      init_(false)
{}

MemoryStore::~MemoryStore()
{
    log_info("db closed");
}

int 
MemoryStore::init(StorageConfig* cfg)
{
    init_ = true;
    if (cfg->tidy_) {
        tables_.clear();
    }
    return 0;
}

int
MemoryStore::get_table(DurableTableImpl**  table,
                       const std::string&  name,
                       int                 flags,
                       PrototypeVector&    prototypes)
{
    TableMap::iterator iter = tables_.find(name);

    MemoryTable::ItemMap* items;
    
    if (iter == tables_.end()) {
        if (! (flags & DS_CREATE)) {
            return DS_NOTFOUND;
        }
        
        tables_[name] = MemoryTable::ItemMap();
        items = &tables_[name];
    } else {
        if (flags & DS_EXCL) {
            return DS_EXISTS;
        }

        items = &iter->second;
    }

    *table = new MemoryTable(items, name, (flags & DS_MULTITYPE) != 0);

    return DS_OK;
}

int
MemoryStore::del_table(const std::string& name)
{
    log_info("deleting table %s", name.c_str());
    tables_.erase(name);
    return 0;
}


/******************************************************************************
 *
 * MemoryTable
 *
 *****************************************************************************/
MemoryTable::MemoryTable(ItemMap* items,
                         const std::string& name, bool multitype)
    : DurableTableImpl(name, multitype), items_(items)
{
    logpathf("/memory/table(%s)", name.c_str());
}


MemoryTable::~MemoryTable() 
{
}

int 
MemoryTable::get(const SerializableObject& key, 
                 SerializableObject*       data)
{
    ASSERTF(!multitype_, "single-type get called for multi-type table");
    
    StringSerialize serialize(Serialize::CONTEXT_LOCAL,
                              Serialize::DOT_SEPARATED);
    if (serialize.action(&key) != 0) {
        PANIC("error sizing key");
    }
    std::string table_key;
    table_key.assign(serialize.buf().data(), serialize.buf().length());

    ItemMap::iterator iter = items_->find(table_key);
    if (iter == items_->end()) {
        return DS_NOTFOUND;
    }

    Item* item = iter->second;
    Unmarshal unm(Serialize::CONTEXT_LOCAL,
                  item->data_.buf(), item->data_.len());

    if (unm.action(data) != 0) {
        log_err("error unserializing data object");
        return DS_ERR;
    }

    return 0;
}

int
MemoryTable::get(const SerializableObject&   key,
                 SerializableObject**        data,
                 TypeCollection::Allocator_t allocator)
{
    ASSERTF(multitype_, "multi-type get called for single-type table");
    
    StringSerialize serialize(Serialize::CONTEXT_LOCAL,
                              Serialize::DOT_SEPARATED);
    if (serialize.action(&key) != 0) {
        PANIC("error sizing key");
    }
    std::string table_key;
    table_key.assign(serialize.buf().data(), serialize.buf().length());

    ItemMap::iterator iter = items_->find(table_key);
    if (iter == items_->end()) {
        return DS_NOTFOUND;
    }

    Item* item = iter->second;
    
    int err = allocator(item->typecode_, data);
    if (err != 0) {
        return DS_ERR;
    }

    Unmarshal unm(Serialize::CONTEXT_LOCAL,
                  item->data_.buf(), item->data_.len());

    if (unm.action(*data) != 0) {
        log_err("error unserializing data object");
        return DS_ERR;
    }

    return DS_OK;
}

int 
MemoryTable::put(const SerializableObject& key,
                 TypeCollection::TypeCode_t typecode,
                 const SerializableObject* data,
                 int                       flags)
{
    StringSerialize serialize(Serialize::CONTEXT_LOCAL,
                              Serialize::DOT_SEPARATED);
    if (serialize.action(&key) != 0) {
        PANIC("error sizing key");
    }
    std::string table_key;
    table_key.assign(serialize.buf().data(), serialize.buf().length());

    ItemMap::iterator iter = items_->find(table_key);

    Item* item;
    if (iter == items_->end()) {
        if ((flags & DS_CREATE) == 0) {
            return DS_NOTFOUND;
        }

        item = new Item();
        (*items_)[table_key] = item;

    } else {
        if (flags & DS_EXCL) {
            return DS_EXISTS;
        }

        item = iter->second;
    }

    item->typecode_ = typecode;

    { // first the key
        MarshalSize sizer(Serialize::CONTEXT_LOCAL);
        if (sizer.action(&key) != 0) {
            log_err("error sizing key object");
            return DS_ERR;
        }
        size_t sz = sizer.size();
    
        log_debug("put: serializing %u byte key object", (u_int)sz);
    
        u_char* buf = item->key_.buf(sz);
    
        Marshal m(Serialize::CONTEXT_LOCAL, buf, sz);
        if (m.action(&key) != 0) {
            log_err("error serializing key object");
            return DS_ERR;
        }

        item->key_.set_len(sz);
    }

    { // then the data
        MarshalSize sizer(Serialize::CONTEXT_LOCAL);
        if (sizer.action(data) != 0) {
            log_err("error sizing data object");
            return DS_ERR;
        }
        size_t sz = sizer.size();
    
        log_debug("put: serializing %u byte object", (u_int)sz);
    
        u_char* buf = item->data_.buf(sz);
    
        Marshal m(Serialize::CONTEXT_LOCAL, buf, sz);
        if (m.action(data) != 0) {
            log_err("error serializing data object");
            return DS_ERR;
        }

        item->data_.set_len(sz);
    }

    item->typecode_ = typecode;

    return DS_OK;
}

int 
MemoryTable::del(const SerializableObject& key)
{ 
    StringSerialize serialize(Serialize::CONTEXT_LOCAL,
                              Serialize::DOT_SEPARATED);
    if (serialize.action(&key) != 0) {
        PANIC("error sizing key");
    }
    std::string table_key;
    table_key.assign(serialize.buf().data(), serialize.buf().length());

    ItemMap::iterator iter = items_->find(table_key);
    if (iter == items_->end()) {
        return DS_NOTFOUND;
    }

    Item* item = iter->second;
    items_->erase(iter);
    delete item;
    
    return DS_OK;
}

size_t
MemoryTable::size()
{
    return items_->size();
}

DurableIterator*
MemoryTable::iter()
{
    return new MemoryIterator(this);
}


/******************************************************************************
 *
 * MemoryIterator
 *
 *****************************************************************************/
MemoryIterator::MemoryIterator(MemoryTable* t)
{
    logpathf("/berkeleydb/iter(%s)", t->name());
    table_ = t;
    first_ = true;
}

MemoryIterator::~MemoryIterator()
{
}

int
MemoryIterator::next()
{
    if (first_) {
        first_ = false;
        iter_ = table_->items_->begin();
    } else {
        ++iter_;
    }

    if (iter_ == table_->items_->end()) {
        return DS_NOTFOUND;
    }

    return 0;
}

int
MemoryIterator::get(SerializableObject* key)
{
    ASSERT(key != NULL);

    MemoryTable::Item* item = iter_->second;
    
    oasys::Unmarshal un(oasys::Serialize::CONTEXT_LOCAL,
                        item->key_.buf(), item->key_.len());
    
    if (un.action(key) != 0) {
        log_err("error unmarshalling");
        return DS_ERR;
    }
    
    return 0;
}

} // namespace oasys

