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
#error DurableStoreImpl.h must only be included from within DurableStore.h
#endif

class StorageConfig;

/**
 * Storage implementation specific pieces of the data store.
 */
class DurableStoreImpl : public Logger {
public:
    //! Map used for ref counting tables
    typedef std::map<std::string, int> RefCountMap;

    /**
     * Typedef for the list of objects passed to get_table.
     */
    typedef std::vector<SerializableObject*> PrototypeVector;

    /**
     * Constructor (initializes the log path).
     */
    DurableStoreImpl(const char* logpath) : Logger(logpath) {}
    
    /**
     * Destructor
     */
    virtual ~DurableStoreImpl() {}

    /*!
     * Initialize the storage impl.
     */
    virtual int init(StorageConfig* config) = 0;

    /**
     * Hook to get or create the implementation-specific components of
     * a durable table.
     *
     */
    virtual int get_table(DurableTableImpl** table,
                          const std::string& db_name,
                          int                flags,
                          PrototypeVector&   prototypes) = 0;

    /**
     * Hook to remove a table (by name) from the data store.
     */
    virtual int del_table(const std::string& db_name) = 0;

protected:

    /**
     * Check for the db directory
     * @param db_dir     Directory to check
     * @param dir_exists To be set if directory exists.
     */
    int check_db_dir(const char* db_dir, bool* dir_exists);

    /**
     * Create database directory.
     */
    int create_db_dir(const char* db_dir);
    
    /**
     * Remove the given directory, after waiting the specified
     * amount of time.
     */
    void prune_db_dir(const char* db_dir, int tidy_wait);
};


/**
 * Storage implementation specific piece of a table.
 */
class DurableTableImpl {
public:
    DurableTableImpl(std::string table_name, bool multitype)
        : table_name_(table_name), multitype_(multitype) {}

    virtual ~DurableTableImpl() {}

    /**
     * Get the data for the given key from the datastore and
     * unserialize into the given data object.
     *
     * @param key  Key object
     * @param data Data object
     * @return DS_OK, DS_NOTFOUND if key is not found
     */
    virtual int get(const SerializableObject& key, 
                    SerializableObject*       data) = 0;

    /** 
     * For a multi-type table, get the data for the given key, calling
     * the provided allocator function to create the object.
     *
     * Note that a default implementation (that panics) is provided
     * such that subclasses need not support multi-type tables.
     *
     * @param key Key object
     * @param typecode Typecode pointer
     * @return DS_OK, DS_ERR // XXX/bowei
     */
    virtual int get(const SerializableObject&   key,
                    SerializableObject**        data,
                    TypeCollection::Allocator_t allocator);
                    
    /**
     * Put data for key in the database
     *
     * @param key      Key object
     * @param typecode Typecode (if multitype)
     * @param data     Data object
     * @param flags    Bit vector of DurableStoreFlags_t values.
     * @return DS_OK, DS_ERR // XXX/bowei
     */
    virtual int put(const SerializableObject&  key,
                    TypeCollection::TypeCode_t typecode,
                    const SerializableObject*  data,
                    int flags) = 0;
    
    /**
     * Delete a (key,data) pair from the database
     * @return DS_OK, DS_NOTFOUND if key is not found
     */
    virtual int del(const SerializableObject& key) = 0;

    /**
     * Return the number of elements in the table.
     */
    virtual size_t size() const = 0;
    
    /**
     * Get an iterator to this table. 
     *
     * @return The new iterator. Caller deletes this pointer.
     */
    virtual DurableIterator* iter() = 0;

    /**
     * Return the name of this table.
     */
    const char* name() { return table_name_.c_str(); }

protected:
    /**
     * Helper method to flatten a serializable object into a buffer.
     */
    size_t flatten(const SerializableObject& key, 
                   u_char* key_buf, size_t size);
    
    template<size_t _size>
    size_t flatten(const SerializableObject&      key,
                   ScratchBuffer<u_char*, _size>* scratch);
    
    std::string table_name_;	///< Name of the table
    bool multitype_;		///< Whether single or multi-type table
};

// Implementation of the templated method must be in the header
template<size_t _size>
size_t
DurableTableImpl::flatten(const SerializableObject&      key,
                          ScratchBuffer<u_char*, _size>* scratch)
{
    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    sizer.action(&key);

    Marshal marshaller(Serialize::CONTEXT_LOCAL, 
                       scratch->buf(sizer.size()), 
                       sizer.size());
    const_cast<SerializableObject&>(key).serialize(&marshaller);
    
    return sizer.size();
}
