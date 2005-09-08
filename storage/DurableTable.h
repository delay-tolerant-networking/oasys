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


#ifndef __OASYS_DURABLE_STORE_INTERNAL_HEADER__
#error DurableTable.h must only be included from within DurableStore.h
#endif

/**
 * Object that encapsulates a single durable table. The class has no
 * derivatives, but rather is responsible for managing the translation
 * of memory <-> datastore. The interactioon with the underlying
 * storage implementation is handled by the DurableTableImpl class.
 */
class DurableTable {
public:
    DurableTable(DurableTableImpl*   impl,
                 const std::string&  name,
                 DurableObjectCache* cache)
        : impl_(impl), name_(name), cache_(cache) {}
    
    ~DurableTable()
    {
        delete impl_;
    }

    
    /** 
     * Update the value of the key, data pair in the database. It
     * should already exist.
     *
     * @param key   Key object
     * @param data  Data object
     * @param flags Bit vector of DurableStoreFlags_t values.
     * @return DS_OK, DS_NOTFOUND, DS_ERR
     */
    int put(const SerializableObject& key,
            const SerializableObject* data,
            int flags);
    
    /**
     * Delete a (key,data) pair from the database
     *
     * @return DS_OK, DS_NOTFOUND if key is not found
     */
    int del(const SerializableObject& key);

    /**
     * Return a newly allocated iterator for the table. Caller has the
     * responsibility to delete it once finished.
     */
    DurableIterator* iter() { return impl_->iter(); }

    /**
     * Return the underlying table implementation.
     */
    DurableTableImpl* impl() { return impl_; }

    /**
     * Return table name.
     */
    std::string name() { return name_; }

protected:
    DurableTableImpl*   impl_;
    std::string         name_;
    DurableObjectCache* cache_;

private:
    DurableTable();
    DurableTable(const DurableTable&);
};

/**
 * Class for a durable table that only stores one type of object,
 * represented by the template parameter _DataType.
 */
template <typename _DataType>
class SingleTypeDurableTable : public DurableTable {
public:
    /**
     * Constructor
     */
    SingleTypeDurableTable(DurableTableImpl*   impl,
                           const std::string&  name,
                           DurableObjectCache* cache)
        : DurableTable(impl, name, cache) {}
    
    /**
     * Get the data for key, possibly creating a new object of the
     * template type _DataType. Note that the given type must match
     * the actual type that was stored in the database, or this will
     * return undefined garbage.
     *
     * @param key  Key object
     * @param data Data object
     *
     * @return DS_OK, DS_NOTFOUND if key is not found
     */
    int get(const SerializableObject& key, _DataType** data);

private:
    SingleTypeDurableTable();
    SingleTypeDurableTable(const SingleTypeDurableTable&);
};

/**
 * Class for a durable table that can store various objects, each a
 * subclass of _BaseType which must in turn be or be a subclass of
 * TypedSerializableObject, and that has a type code defined in the
 * template parameter _Collection.
 */
template <typename _BaseType, typename _Collection>
class MultiTypeDurableTable : public DurableTable {
public:
    /**
     * Constructor
     */
    MultiTypeDurableTable(DurableTableImpl*   impl,
                          const std::string&  name,
                          DurableObjectCache* cache)
        : DurableTable(impl, name, cache) {}
    
    /**
     * Get the data for key, possibly creating a new object of the
     * given template type _Type (or some derivative), using the
     * multitype collection specified by _Collection. _Type therefore
     * must be a valid superclass for the object identified by the
     * type code in the database.
     *
     * @param key  Key object
     * @param data Data object
     *
     * @return DS_OK, DS_NOTFOUND if key is not found
     */
    int get(const SerializableObject& key, _BaseType** data);

private:
    MultiTypeDurableTable();
    MultiTypeDurableTable(const MultiTypeDurableTable&);
};


#include "DurableTable.tcc"
