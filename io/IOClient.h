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

#ifndef _OASYS_IOCLIENT_H_
#define _OASYS_IOCLIENT_H_

#include <sys/uio.h>

namespace oasys {

/**
 * Abstract interface for any stream type output channel.
 */
class IOClient {
public:
    //@{
    /**
     * System call wrappers.
     */
    virtual int read(char* bp, size_t len) = 0;
    virtual int write(const char* bp, size_t len) = 0;
    virtual int readv(const struct iovec* iov, int iovcnt)  = 0;
    virtual int writev(const struct iovec* iov, int iovcnt) = 0;
    //@}
    
    //@{
    /**
     * Read/write out the entire supplied buffer, potentially
     * requiring multiple system calls.
     *
     * @return the total number of bytes written, or -1 on error
     */
    virtual int readall(char* bp, size_t len) = 0;
    virtual int writeall(const char* bp, size_t len) = 0;
    virtual int readvall(const struct iovec* iov, int iovcnt) = 0;
    virtual int writevall(const struct iovec* iov, int iovcnt) = 0;
    //@}

    //@{
    /**
     * @brief Try to read the specified number of bytes, but don't
     * block for more than timeout milliseconds.
     *
     * @return the number of bytes read or the appropriate
     * IOTimeoutReturn_t code
     */
    virtual int timeout_read(char* bp, size_t len, int timeout_ms) = 0;
    virtual int timeout_readv(const struct iovec* iov, int iovcnt,
                              int timeout_ms) = 0;
    virtual int timeout_readall(char* bp, size_t len, int timeout_ms) = 0;
    virtual int timeout_readvall(const struct iovec* iov, int iovcnt,
                                 int timeout_ms) = 0;
    //@}

    /// Set the file descriptor's nonblocking status
    virtual int get_nonblocking(bool* nonblockingp) = 0;
    virtual int set_nonblocking(bool nonblocking) = 0;
};

} // namespace oasys

#endif // _OASYS_IOCLIENT_H_
