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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "DurableStore.h"
#include "debug/DebugUtils.h"
#include "serialize/MarshalSerialize.h"

namespace oasys {

void
DurableStoreImpl::prune_db_dir(const char* dir, int tidy_wait)
{
    char cmd[256];
    for (int i = tidy_wait; i > 0; --i) 
    {
        log_warn("PRUNING CONTENTS OF %s IN %d SECONDS", dir, i);
        sleep(1);
    }
    sprintf(cmd, "/bin/rm -rf %s", dir);
    log_notice("tidy option removing directory '%s'", cmd);
    system(cmd);
}

int
DurableStoreImpl::check_db_dir(const char* db_dir, bool* dir_exists)
{
    *dir_exists = false;

    struct stat f_stat;
    if (stat(db_dir, &f_stat) == -1)
    {
        if (errno == ENOENT)
        {
            *dir_exists = false;
        }
        else 
        {
            log_err("error trying to stat database directory %s: %s",
                    db_dir, strerror(errno));
            return DS_ERR;
        }
    }
    else
    {
        *dir_exists = true;
    }

    return 0;
}

int
DurableStoreImpl::create_db_dir(const char* db_dir)
{
    // create database directory
    char pwd[PATH_MAX];
    
    log_notice("creating new database directory %s%s%s",
               db_dir[0] == '/' ? "" : getcwd(pwd, PATH_MAX),
               db_dir[0] == '/' ? "" : "/",
               db_dir);
            
    if (mkdir(db_dir, 0700) != 0) 
    {
        log_crit("can't create datastore directory %s: %s",
                 db_dir, strerror(errno));
        return DS_ERR;
    }
    return 0;
}

int
DurableTableImpl::get(const SerializableObject&   key,
                      SerializableObject**        data,
                      TypeCollection::Allocator_t allocator)
{
    PANIC("Generic DurableTableImpl get method called for "
          "multi-type tables");
}

size_t
DurableTableImpl::flatten(const SerializableObject& key, 
                          u_char* key_buf, size_t size)
{
    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    sizer.action(&key);

    if (sizer.size() > size)
    {
        return 0;
    }

    Marshal marshaller(Serialize::CONTEXT_LOCAL, key_buf, 256);
    marshaller.action(&key);
    
    return sizer.size();
}

} // namespace oasys
