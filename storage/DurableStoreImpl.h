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
#ifndef _OASYS_DURABLE_STORE_IMPL_H_
#define _OASYS_DURABLE_STORE_IMPL_H_

/**
 * Storage implementation specific pieces of the data store.
 */
class DurableStoreImpl {
public:
    /**
     * Typedef for the list of objects passed to get_table.
     */
    typedef std::vector<SerializableObject*> PrototypeVector;

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
};


/**
 * Storage implementation specific piece of a table.
 */
class DurableTableImpl {
public:
    DurableTableImpl(std::string table_name) : table_name_(table_name) {}
    virtual ~DurableTableImpl() {}

    /**
     * Get the data for key.
     *
     * @param key  Key object
     * @param data Data object
     * @return DS_OK, DS_NOTFOUND if key is not found
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
                    const SerializableObject* data,
                    int flags) = 0;

    /**
     * Delete a (key,data) pair from the database
     * @return DS_OK, DS_NOTFOUND if key is not found
     */
    virtual int del(const SerializableObject& key) = 0;

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
    
private:
    std::string table_name_;
};

#endif /* _OASYS_DURABLE_STORE_IMPL_H_ */
