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

#ifndef __FILESTORE_H__
#define __FILESTORE_H__

#include <sys/types.h>
#include <dirent.h>

#include "../debug/Logger.h"
#include "../thread/SpinLock.h"
#include "../util/ScratchBuffer.h"

#include "DurableStore.h"

namespace oasys {

class ExpandableBuffer;

class StorageConfig;

class FileSystemStore;
class FileSystemTable;
class FileSystemIterator;

/*!
 * The most obvious layering of backing store -- use the file system
 * directly. 
 */
class FileSystemStore : public DurableStoreImpl {
    friend class FileSystemTable;
public:
    FileSystemStore();

    // Can't copy or =, don't implement these
    FileSystemStore& operator=(const FileSystemStore&);
    FileSystemStore(const FileSystemStore&);

    ~FileSystemStore();

    //! @{ virtual from DurableStoreImpl
    int init(const StorageConfig& cfg);
    int get_table(DurableTableImpl** table,
                  const std::string& name,
                  int                flags,
                  PrototypeVector&   prototypes);
    int del_table(const std::string& name);
    int get_table_names(StringVector* names);
    //! @}

private:
    bool        init_;
    std::string tables_dir_; //!< directory where the tables are stored

    RefCountMap ref_count_; // XXX/bowei -- not used for now
    int default_perm_; //!< Default permissions on database files
    
    //! Check for the existance of databases. @return 0 on no error.
    //! @return -2 if the database file doesn't exist. Otherwise -1.
    int check_database();

    //! Create the database. @return 0 on no error.
    int init_database();
    
    //! Wipe the database. @return 0 on no error.
    void tidy_database();

    /// @{ Changes the ref count on the tables
    // XXX/bowei -- implement me
    int acquire_table(const std::string& table);
    int release_table(const std::string& table);
    /// @}
};

class FileSystemTable : public Logger, 
                        public DurableTableImpl {
    friend class FileSystemStore;
public:
    ~FileSystemTable();

    //! @{ virtual from DurableTableInpl
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
    //! @}

private:
    std::string path_;

    FileSystemTable(const std::string& path,
                    bool               multitype);

    int get_common(const SerializableObject& key,
                   ExpandableBuffer* buf);
};

class FileSystemIterator : public DurableIterator, 
                           public Logger {
    friend class FileSystemTable;
private:
    /**
     * Create an iterator for table t. These should not be called
     * except by FileSystemTable.
     */
    FileSystemIterator(const std::string& directory);

public:
    virtual ~FileSystemIterator();
    
    //! @{ virtual from DurableIteratorImpl
    int next();
    int get(SerializableObject* key);
    //! @}

protected:
    struct dirent* ent_;
    DIR*           dir_;
};

} // namespace oasys

#endif /* __FILESTORE_H__ */
