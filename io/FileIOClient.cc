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

#include <errno.h>
#include "FileIOClient.h"
#include "IO.h"

namespace oasys {

FileIOClient::FileIOClient()
    : FdIOClient(-1)
{
}

FileIOClient::~FileIOClient()
{
    if (fd_ != -1)
        close();
}

int
FileIOClient::open(const char* path, int flags, int* errnop)
{
    path_.assign(path);
    fd_ = IO::open(path, flags, errnop, logpath_);
    return fd_;
}

int
FileIOClient::open(const char* path, int flags, mode_t mode, int* errnop)
{
    path_.assign(path);
    fd_ = IO::open(path, flags, mode, errnop, logpath_);
    return fd_;
}

int
FileIOClient::close()
{
    int ret = IO::close(fd_, logpath_, path_.c_str());
    fd_ = -1;
    return ret;
}

int
FileIOClient::reopen(int flags)
{
    ASSERT(path_.length() != 0);
    fd_ = IO::open(path_.c_str(), flags, NULL, logpath_);
    return fd_;
}

int
FileIOClient::unlink()
{
    if (path_.length() == 0)
        return 0;
    
    int ret = 0;
    ret = IO::unlink(path_.c_str(), logpath_);
    path_.assign("");
    return ret;
}

int
FileIOClient::lseek(off_t offset, int whence)
{
    return IO::lseek(fd_, offset, whence, logpath_);
}

int
FileIOClient::truncate(off_t length)
{
    return IO::truncate(fd_, length, logpath_);
}

int
FileIOClient::mkstemp(char* temp)
{
    if (fd_ != -1) {
        log_err("can't call mkstemp on open file");
        return -1;
    }

    fd_ = IO::mkstemp(temp, logpath_);

    path_.assign(temp);
    return fd_;
}

int
FileIOClient::copy_contents(size_t len, FileIOClient* dest)
{
    char buf[1024];
    int cnt;
    int origlen = len;

    while (len > 0) {
        cnt = (len < sizeof(buf)) ? len : sizeof(buf);

        if (readall(buf, cnt) != cnt) {
            log_err("copy_contents: error reading %d bytes: %s",
                    cnt, strerror(errno));
            return -1;
        }

        if (dest->writeall(buf, cnt) != cnt) {
            log_err("copy_contents: error writing %d bytes: %s",
                    cnt, strerror(errno));
            return -1;
        }

        len -= cnt;
    }
    
    return origlen;
}

} // namespace oasys
