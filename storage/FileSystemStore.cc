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
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#include "../util/ExpandableBuffer.h"
#include "../serialize/KeySerialize.h"
#include "../serialize/TypeCollection.h"
#include "../io/FileUtils.h"
#include "../io/IO.h"


namespace oasys {

//----------------------------------------------------------------------------
FileSystemStore::FileSystemStore()
    : DurableStoreImpl("/FileSystemStore"),
      tables_dir_("INVALID"),
      default_perm_(S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP)
{}

//----------------------------------------------------------------------------
FileSystemStore::~FileSystemStore()
{}

//----------------------------------------------------------------------------
int 
FileSystemStore::init(const StorageConfig& cfg)
{
    if (cfg.dbdir_ == "") {
        return -1;
    }

    if (cfg.dbname_ == "") {
        return -1;
    }
    
    std::string dbdir = cfg.dbdir_;
    FileUtils::abspath(&dbdir);

    tables_dir_ = dbdir + "/" + cfg.dbname_;

    bool tidy = cfg.tidy_;
    bool init = cfg.init_;

    // Always regenerate the directories if we are going to be
    // deleting them anyways
    if (tidy) {
        init = true;
    }

    if (init && tidy) 
    {
        if (check_database() == 0) {
            tidy_database();
        }
        int err = init_database();
        if (err != 0) {
            return -1;
        }
    }
    else if (init && ! tidy) 
    {
        if (check_database() == -2) {
            int err = init_database();
            if (err != 0) {
                return -1;
            }
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

    struct stat st;
    int err = stat(dir_path.c_str(), &st);
    
    // Already existing directory
    if (err != 0 && errno == ENOENT) {
        if ( ! (flags & DS_CREATE)) {
            return DS_NOTFOUND;
        }
        
        int err = mkdir(dir_path.c_str(), default_perm_);
        if (err != 0) {
            err = errno;
            log_err("Couldn't mkdir: %s", strerror(err));
            return DS_ERR;
        }
    } else if (err != 0) { 
        return DS_ERR;
    } else if (err == 0 && (flags & DS_EXCL)) {
        return DS_EXISTS;
    }
    
    FileSystemTable* table_ptr = new FileSystemTable(dir_path, flags & DS_MULTITYPE);
    ASSERT(table_ptr);
    
    *table = table_ptr;

    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemStore::del_table(const std::string& name)
{
    // XXX/bowei -- don't handle table sharing for now
    ASSERT(init_);

    std::string dir_path = tables_dir_;
    dir_path.append("/");
    dir_path.append(name);
    
    FileUtils::rm_all_from_dir(dir_path.c_str());

    // clean out the directory
    int err;
    err = rmdir(dir_path.c_str());
    if (err != 0) {
        log_warn("couldn't remove directory, %s", strerror(errno));
        return -1;
    }

    return 0; 
}

//----------------------------------------------------------------------------
int 
FileSystemStore::get_table_names(StringVector* names)
{
    names->clear();
    
    DIR* dir = opendir(tables_dir_.c_str());
    if (dir == 0) {
        log_err("Can't get table names from directory");
        return DS_ERR;
    }

    struct dirent* ent = readdir(dir);
    while (ent != 0) {
        names->push_back(ent->d_name);
        ent = readdir(dir);
    }

    closedir(dir);

    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemStore::check_database()
{
    DIR* dir = opendir(tables_dir_.c_str());
    if (dir == 0) {
        if (errno == ENOENT) {
            return -2;
        } else {
            return -1;
        }
    }
    closedir(dir);

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
FileSystemTable::FileSystemTable(const std::string& path,
                                 bool               multitype)
    : Logger("/FileSystemTable"),
      DurableTableImpl("/FileSystemTable", multitype),
      path_(path)
{}

FileSystemTable::~FileSystemTable()
{}

//----------------------------------------------------------------------------
int 
FileSystemTable::get(const SerializableObject& key,
                     SerializableObject*       data)
{
    ASSERTF(!multitype_, "single-type get called for multi-type table");

    ScratchBuffer<u_char*, 4096> buf;
    int err = get_common(key, &buf);
    if (err != 0) {
        return err;
    }
    
    Unmarshal um(Serialize::CONTEXT_LOCAL, buf.buf(), buf.len());
    err = um.action(data);
    if (err != 0) {
        return DS_ERR;
    }

    return 0;
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::get(const SerializableObject&   key,
                     SerializableObject**        data,
                     TypeCollection::Allocator_t allocator)
{
    ASSERTF(multitype_, "multi-type get called for single-type table");

    ScratchBuffer<u_char*, 4096> buf;
    int err = get_common(key, &buf);
    if (err != 0) {
        return err;
    }
    
    Unmarshal um(Serialize::CONTEXT_LOCAL, buf.buf(), buf.len());

    TypeCollection::TypeCode_t typecode;
    um.process("typecode", &typecode);
    
    err = allocator(typecode, data);
    if (err != 0) {
        return DS_ERR;
    }
    err = um.action(*data);
    if (err != 0) {
        return DS_ERR;
    }

    return 0;
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::put(const SerializableObject&  key,
                     TypeCollection::TypeCode_t typecode,
                     const SerializableObject*  data,
                     int flags)
{
    ScratchBuffer<char*, 512> key_str;
    KeyMarshal s_key(&key_str, "-");

    if (s_key.action(&key) != 0) {
        log_err("Can't get key");
        return DS_ERR;
    }
    
    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    if (multitype_) {
        sizer.process("typecode", &typecode);
    }
    sizer.action(data);

    ScratchBuffer<u_char*, 4096> scratch;
    scratch.reserve(sizer.size());

    Marshal m(Serialize::CONTEXT_LOCAL, scratch.buf(), sizer.size());
    if (multitype_) {
        m.process("typecode", &typecode);
    }
    if (m.action(data) != 0) {
        log_warn("can't marshal data");
        return DS_ERR;
    }

    std::string filename = path_ + "/" + key_str.buf();
    int data_elt_fd      = -1;
    int open_flags       = O_TRUNC | O_WRONLY;

    if (flags & DS_EXCL) {
        open_flags |= O_EXCL;       
    }
    
    if (flags & DS_CREATE) {
        open_flags |= O_CREAT;
    }

    data_elt_fd = open(filename.c_str(), open_flags, 
                       S_IRUSR | S_IWUSR | S_IRGRP);

    if (data_elt_fd == -1) 
    {
        if (errno == ENOENT) {
            log_debug("file not found and DS_CREATE not specified");
            return DS_NOTFOUND;
        } else if (errno == EEXIST) {
            log_debug("file found and DS_EXCL specified");
            return DS_EXISTS;
        } else {
            log_warn("can't open %s: %s", 
                     filename.c_str(), strerror(errno));
            return DS_ERR;
        }
    }
    
    log_debug("created file %s, fd = %d", 
              filename.c_str(), data_elt_fd);
    
    int cc = IO::writeall(data_elt_fd, 
                          reinterpret_cast<char*>(scratch.buf()), 
                          sizer.size());
    if (cc != static_cast<int>(sizer.size())) {
        log_warn("put() - errors writing to file %s, %d: %s",
                 filename.c_str(), cc, strerror(errno));
        return DS_ERR;
    }
    close(data_elt_fd);

    return 0;
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::del(const SerializableObject& key)
{
    ScratchBuffer<char*, 512> key_str;
    KeyMarshal s_key(&key_str, "-");

    if (s_key.action(&key) != 0) {
        log_err("Can't get key");
        return DS_ERR;
    }
    
    std::string filename = path_ + "/" + key_str.buf();
    int err = unlink(filename.c_str());
    if (err == -1) 
    {
        if (errno == ENOENT) {
            return DS_NOTFOUND;
        }
        
        log_warn("can't unlink file %s - %s", filename.c_str(),
                 strerror(errno));
        return DS_ERR;
    }
    
    return 0;
}

//----------------------------------------------------------------------------
size_t 
FileSystemTable::size() const
{
    // XXX/bowei -- be inefficient for now
    DIR* dir = opendir(path_.c_str());
    ASSERT(dir != 0);
    
    size_t count;
    struct dirent* ent;

    for (count = 0, ent = readdir(dir); 
         ent != 0; ent = readdir(dir))
    {
        ASSERT(ent != 0);
        ++count;
    }

    closedir(dir);

    // count always includes '.' and '..'
    log_debug("table size = %u", (u_int)count - 2);

    return count - 2; 
}
    
//----------------------------------------------------------------------------
DurableIterator* 
FileSystemTable::iter()
{
    return new FileSystemIterator(path_);
}

//----------------------------------------------------------------------------
int 
FileSystemTable::get_common(const SerializableObject& key,
                            ExpandableBuffer*         buf)
{
    ScratchBuffer<char*, 512> key_str;
    KeyMarshal serialize(&key_str, "-");
    if (serialize.action(&key) != 0) {
        log_err("Can't get key");
        return DS_ERR;
    }
    
    std::string file_name(key_str.at(0));
    std::string file_path = path_ + "/" + file_name;
    int fd = open(file_path.c_str(), O_RDONLY);
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
        cc = IO::read(fd, buf->end(), 4096);
        ASSERT(cc >= 0);
        buf->set_len(buf->len() + cc);
    } while (cc > 0);
    close(fd);
    
    return 0;
}

//----------------------------------------------------------------------------
FileSystemIterator::FileSystemIterator(const std::string& path)
    : ent_(0)
{
    dir_ = opendir(path.c_str());
    ASSERT(dir_ != 0);
}

//----------------------------------------------------------------------------
FileSystemIterator::~FileSystemIterator()
{
    closedir(dir_);
}
    
//----------------------------------------------------------------------------
int 
FileSystemIterator::next()
{
    ent_ = readdir(dir_);
    
    while (ent_ != 0 && 
           (strcmp(ent_->d_name, ".") == 0 ||
            strcmp(ent_->d_name, "..") == 0))
    {
        ent_ = readdir(dir_);
    }

    if (ent_ == 0) {
        if (errno == ENOENT) {
            return DS_NOTFOUND;
        } else {
            return DS_ERR;
        }
    }

    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemIterator::get(SerializableObject* key)
{
    ASSERT(ent_ != 0);
    
    KeyUnmarshal um(ent_->d_name, strlen(ent_->d_name), "-");
    int err = um.action(key);
    if (err != 0) {
        return DS_ERR;
    }

    return 0;
}

} // namespace oasys
