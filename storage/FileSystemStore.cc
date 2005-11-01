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


#include "FileSystemStore.h"
#include "StorageConfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "../util/ExpandableBuffer.h"
#include "../serialize/TypeCollection.h"
#include "../io/IO.h"

namespace oasys {

//----------------------------------------------------------------------------
FileSystemStore::FileSystemStore()
    : DurableStoreImpl("/FileSystemStore"),
      tables_dir_("INVALID"),
      dir_(0),
      default_perm_(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP)
{}

//----------------------------------------------------------------------------
FileSystemStore::~FileSystemStore()
{
    if (dir_ != 0) {
        closedir(dir_);
    }
}

//----------------------------------------------------------------------------
int 
FileSystemStore::init(StorageConfig* cfg)
{
    if (cfg->dbdir_ == "") {
        return -1;
    }

    if (cfg->dbname_ == "") {
        return -1;
    }

    tables_dir_ = cfg->dbdir_ + "/" + cfg->dbname_;

    // Always regenerate the directories if we are going to be
    // deleting them anyways
    if (cfg->tidy_) {
        cfg->init_ = true;
    }

    if (cfg->init_ && cfg->tidy_) 
    {
        if (check_database() == 0) {
            tidy_database();
        }
        int err = init_database();
        if (err != 0) {
            return -1;
        }
    }
    else if (cfg->init_ && ! cfg->tidy_) 
    {
        if (check_database() != -2) {
            log_warn("Database already exists - not clobbering...");
            return -1;
        }
        int err = init_database();
        if (err != 0) {
            return -1;
        } 
    }
    else 
    {
        if (check_database() != 0) {
            log_err("Database directory not found");
            return -1;
        }
    }

    log_info("init() done");
    init_ = true;

    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemStore::get_table(DurableTableImpl** table,
                           const std::string& name,
                           int                flags,
                           PrototypeVector&   prototypes)
{
    ASSERT(init_);

    std::string dir_path =  tables_dir_;
    dir_path.append("/");
    dir_path.append(name);

    DIR* dir = opendir(dir_path.c_str());
    if (dir == 0) {
        if ( ! (flags & DS_CREATE)) {
            return DS_NOTFOUND;
        }

        int err = mkdir(dir_path.c_str(), default_perm_);
        if (err != 0) {
            int err = errno;
            log_err("Couldn't mkdir: %s", strerror(err));
            return DS_ERR;
        }
    } else if (flags & DS_EXCL) {
        return DS_EXISTS;
    }
    
    if (dir != 0) {
        closedir(dir);
    }

    FileSystemTable* table_ptr = new FileSystemTable(name);
    ASSERT(table_ptr);
    
    *table = table_ptr;

    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemStore::del_table(const std::string& name)
{
    ASSERT(init_);

    std::string dir_path = tables_dir_;
    dir_path.append("/");
    dir_path.append(name);
    
    // XXX/bowei -- don't handle table sharing for now

    int err;

    // clean out the directory
    DIR* dir = opendir(dir_path.c_str());
    struct dirent* ent = readdir(dir);
    while (ent != 0) {
        std::string ent_name = dir_path + "/" + ent->d_name;
        err = unlink(ent_name.c_str());
        ASSERT(err != 0);
    }
    
    err = rmdir(dir_path.c_str());
    ASSERT(err != 0);

    return 0; 
}

//----------------------------------------------------------------------------
int 
FileSystemStore::check_database()
{
    dir_ = opendir(tables_dir_.c_str());
    if (dir_ == 0) {
        if (errno == ENOENT) {
            return -2;
        } else {
            return -1;
        }
    }
    closedir(dir_);

    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemStore::init_database()
{
    log_notice("init() database");

    int err = mkdir(tables_dir_.c_str(), default_perm_);
    if (err != 0) {
        log_warn("init() failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

//----------------------------------------------------------------------------
void
FileSystemStore::tidy_database()
{
    log_notice("Tidy() database");
    
    char cmd[256];
    int cc = snprintf(cmd, 256, "rm -rf %s", tables_dir_.c_str());
    ASSERT(cc < 256);
    system(cmd);
}

//----------------------------------------------------------------------------
FileSystemTable::FileSystemTable(const std::string& path)
    : Logger("/FileSystemTable"),
      DurableTableImpl("/FileSystemTable", false),
      path_(path)
{}

FileSystemTable::~FileSystemTable()
{}

//----------------------------------------------------------------------------
int 
FileSystemTable::get(const SerializableObject& key,
                     SerializableObject* data)
{
    ScratchBuffer<u_char*, 4096> buf;
    int err = get_common(key, &buf);
    if (err != 0) {
        return err;
    }
    
    Marshal marshaller(Serialize::CONTEXT_LOCAL, buf.buf(), buf.len());
    err = marshaller.action(data);
    if (err != 0) {
        return DS_ERR;
    }

    return 0;
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::get(const SerializableObject& key,
                     SerializableObject** data,
                     TypeCollection::Allocator_t allocator)
{
    ScratchBuffer<u_char*, 4096> buf;
    int err = get_common(key, &buf);
    if (err != 0) {
        return err;
    }
    
    Marshal marshaller(Serialize::CONTEXT_LOCAL, buf.buf(), buf.len());

    TypeCollection::TypeCode_t typecode;
    marshaller.process("typecode", &typecode);
    
    err = allocator(typecode, data);
    if (err != 0) {
        return DS_ERR;
    }
    err = marshaller.action(*data);
    if (err != 0) {
        return DS_ERR;
    }

    return 0;
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::put(const SerializableObject& key,
                     TypeCollection::TypeCode_t typecode,
                     const SerializableObject* data,
                     int flags)
{
    ScratchBuffer<u_char*, 256> key_str;

    return 0; // XXX/bowei
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::del(const SerializableObject& key)
{
    ScratchBuffer<u_char*, 256> key_str;

    return 0; // XXX/bowei
}

//----------------------------------------------------------------------------
size_t 
FileSystemTable::size()
{
    

    return 0; // XXX/bowei
}
    
//----------------------------------------------------------------------------
DurableIterator* 
FileSystemTable::iter()
{
    return 0; // XXX/bowei
}

//----------------------------------------------------------------------------
int 
FileSystemTable::get_common(const SerializableObject& key,
                            ExpandableBuffer* buf)
{
    ScratchBuffer<u_char*, 256>  key_str;
    StringSerialize serialize(Serialize::CONTEXT_LOCAL,
                              Serialize::DOT_SEPARATED);
    if (serialize.action(&key) != 0) {
        log_err("Can't get key");
        return DS_ERR;
    }
    
    std::string file_name(serialize.buf().data(), serialize.buf().length());
    std::string file_path = path_ + "/" + file_name;
    int fd = open(file_path.c_str(), 0);
    if (fd == -1) {
        if (errno == ENOENT) {
            return DS_NOTFOUND;
        }
        
        return DS_ERR;
    }
    
    // Snarf all of the bytes
    int cc;
    do {
        buf->reserve(buf->len() + 4096);
        cc = IO::readall(fd, buf->end(), 4096);
        ASSERT(cc >= 0);
        buf->set_len(buf->len() + cc);
    } while (cc > 0);
    close(fd);
    
    return 0;
}

//----------------------------------------------------------------------------
FileSystemIterator::FileSystemIterator(FileSystemTable* t)
{
}

//----------------------------------------------------------------------------
FileSystemIterator::~FileSystemIterator()
{
}
    
//----------------------------------------------------------------------------
int 
FileSystemIterator::next()
{
    return 0; // XXX/bowei
}

//----------------------------------------------------------------------------
int 
FileSystemIterator::get(SerializableObject* key)
{
    return 0; // XXX/bowei
}

} // namespace oasys
