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

#ifndef _FILE_IOCLIENT_H_
#define _FILE_IOCLIENT_H_

#include "FdIOClient.h"
#include <fcntl.h>
#include <string.h>

//XXX/namespace
class FileIOClient;
namespace oasys {
typedef ::FileIOClient FileIOClient;
};

/**
 * IOClient derivative for real files -- not sockets. Unlike the base
 * class FdIOClient, FileIOClient contains the path as a member
 * variable and exposes file specific system calls, i.e. open(),
 * lseek(), etc.
 */
class FileIOClient : public FdIOClient {
public:
    /// Basic constructor, leaves both path and fd unset
    FileIOClient();
    virtual ~FileIOClient();

    ///@{
    /// System call wrappers
    int open(const char* path, int flags);
    int open(const char* path, int flags, mode_t mode);
    int close();
    int unlink();
    int lseek(off_t offset, int whence);
    ///@}

    /// Set the path associated with this file handle
    void set_path(const char* path) {
        path_.assign(path);
    }

    /// Reopen a previously opened path
    int reopen(int flags);

    /// Check if the file descriptor is open
    bool is_open() { return fd_ != -1; }

    /// Path accessor
    const char* path() { return path_.c_str(); }

protected:
    /// Path to the file
    std::string path_;
};

#endif /* _FILE_IOCLIENT_H_ */
