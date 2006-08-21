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

#ifndef _OASYS_FD_IOCLIENT_H_
#define _OASYS_FD_IOCLIENT_H_

#include "IOClient.h"
#include "../debug/Logger.h"
#include "../thread/Notifier.h"

namespace oasys {

/**
 * IOClient which uses pure file descriptors.
 */
class FdIOClient : public IOClient, public Logger {
public:
    //! @param fd File descriptor to interact with
    //! @param intr Optional notifier to use to interrupt blocked I/O
    FdIOClient(int fd, Notifier* intr = 0);

    //! @{ Virtual from IOClient
    virtual int read(char* bp, size_t len);
    virtual int readv(const struct iovec* iov, int iovcnt);
    virtual int write(const char* bp, size_t len);
    virtual int writev(const struct iovec* iov, int iovcnt);

    virtual int readall(char* bp, size_t len);
    virtual int writeall(const char* bp, size_t len);
    virtual int readvall(const struct iovec* iov, int iovcnt);
    virtual int writevall(const struct iovec* iov, int iovcnt);
    
    virtual int timeout_read(char* bp, size_t len, int timeout_ms);
    virtual int timeout_readv(const struct iovec* iov, int iovcnt,
                              int timeout_ms);
    virtual int timeout_readall(char* bp, size_t len, int timeout_ms);
    virtual int timeout_readvall(const struct iovec* iov, int iovcnt,
                                 int timeout_ms);

    virtual int timeout_write(const char* bp, size_t len, int timeout_ms);
    virtual int timeout_writev(const struct iovec* iov, int iovcnt,
                               int timeout_ms);
    virtual int timeout_writeall(const char* bp, size_t len, int timeout_ms);
    virtual int timeout_writevall(const struct iovec* iov, int iovcnt,
                                  int timeout_ms);

    virtual int get_nonblocking(bool* nonblockingp);
    virtual int set_nonblocking(bool nonblocking);
    //! @}

protected:
    int       fd_;
};

} // namespace oasys

#endif /* _OASYS_FD_IOCLIENT_H_ */
