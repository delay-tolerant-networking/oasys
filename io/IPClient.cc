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


#include <stdlib.h>
#include <sys/poll.h>

#include "IPClient.h"

namespace oasys {

IPClient::IPClient(const char* logbase, int socktype)
    : IPSocket(logbase, socktype)
{
}

IPClient::IPClient(int fd, in_addr_t remote_addr, u_int16_t remote_port,
                   const char* logbase)
    : IPSocket(fd, remote_addr, remote_port, logbase)
{
}

IPClient::~IPClient()
{
}

int
IPClient::read(char* bp, size_t len)
{
// debugging hookto make sure that callers can handle short reads
// #define TEST_SHORT_READ
#ifdef TEST_SHORT_READ
    if (len > 64) {
        int rnd = rand() % len;
        ::logf("/test/shortread", LOG_DEBUG, "read(%d) -> read(%d)", len, rnd);
        len = rnd;
    }
#endif
    return IO::read(fd_, bp, len, logpath_);
}

int
IPClient::readv(const struct iovec* iov, int iovcnt)
{
    return IO::readv(fd_, iov, iovcnt, logpath_);
}

int
IPClient::write(const char* bp, size_t len)
{
    return IO::write(fd_, bp, len, logpath_);
}

int
IPClient::writev(const struct iovec* iov, int iovcnt)
{
    return IO::writev(fd_, iov, iovcnt, logpath_);
}

int
IPClient::poll(int events, int* revents, int timeout_ms)
{
    return IO::poll(fd_, events, revents, timeout_ms, logpath_);
}

int
IPClient::readall(char* bp, size_t len)
{
    return IO::readall(fd_, bp, len, logpath_);
}

int
IPClient::writeall(const char* bp, size_t len)
{
    return IO::writeall(fd_, bp, len, logpath_);
}

int
IPClient::readvall(const struct iovec* iov, int iovcnt)
{
    return IO::readvall(fd_, iov, iovcnt, logpath_);
}

int
IPClient::writevall(const struct iovec* iov, int iovcnt)
{
    return IO::writevall(fd_, iov, iovcnt, logpath_);
}

int
IPClient::timeout_read(char* bp, size_t len, int timeout_ms)
{
    return IO::timeout_read(fd_, bp, len, timeout_ms, logpath_);
}

int
IPClient::timeout_readv(const struct iovec* iov, int iovcnt, int timeout_ms)
{
    return IO::timeout_readv(fd_, iov, iovcnt, timeout_ms, logpath_);
}

int
IPClient::timeout_readall(char* bp, size_t len, int timeout_ms)
{
    return IO::timeout_readall(fd_, bp, len, timeout_ms, logpath_);
}

int
IPClient::timeout_readvall(const struct iovec* iov, int iovcnt, int timeout_ms)
{
    return IO::timeout_readvall(fd_, iov, iovcnt, timeout_ms, logpath_);
}

int
IPClient::get_nonblocking(bool *nonblockingp)
{
    return IO::get_nonblocking(fd_, nonblockingp);
}

int
IPClient::set_nonblocking(bool nonblocking)
{
    return IO::set_nonblocking(fd_, nonblocking);
}

} // namespace oasys
