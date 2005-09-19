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

IPClient::IPClient(int socktype, const char* logbase, Notifier* intr)
    : IOHandlerBase(intr), IPSocket(socktype, logbase)
{}

IPClient::IPClient(int socktype, int sock,
                   in_addr_t remote_addr, u_int16_t remote_port,
                   const char* logbase, Notifier* intr)
    : IOHandlerBase(intr), 
      IPSocket(socktype, sock, remote_addr, remote_port, logbase)
{}

IPClient::~IPClient() {}

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

    int cc = IO::read(fd_, bp, len, get_notifier(), logpath_);
    monitor(IO::READV, 0); // XXX/bowei

    return cc;
}

int
IPClient::readv(const struct iovec* iov, int iovcnt)
{
    int cc = IO::readv(fd_, iov, iovcnt, get_notifier(), logpath_);
    monitor(IO::READV, 0); // XXX/bowei

    return cc;
}

int
IPClient::write(const char* bp, size_t len)
{
    int cc = IO::write(fd_, bp, len, get_notifier(), logpath_);
    monitor(IO::WRITEV, 0); // XXX/bowei

    return cc;
}

int
IPClient::writev(const struct iovec* iov, int iovcnt)
{
    int cc = IO::writev(fd_, iov, iovcnt, get_notifier(), logpath_);
    monitor(IO::WRITEV, 0); // XXX/bowei

    return cc;
}

int
IPClient::readall(char* bp, size_t len)
{
    int cc = IO::readall(fd_, bp, len, get_notifier(), logpath_);
    monitor(IO::READV, 0); // XXX/bowei

    return cc;
}

int
IPClient::writeall(const char* bp, size_t len)
{
    int cc = IO::writeall(fd_, bp, len, get_notifier(), logpath_);
    monitor(IO::WRITEV, 0); // XXX/bowei

    return cc;
}

int
IPClient::readvall(const struct iovec* iov, int iovcnt)
{
    int cc = IO::readvall(fd_, iov, iovcnt, get_notifier(), logpath_);
    monitor(IO::READV, 0); // XXX/bowei

    return cc;
}

int
IPClient::writevall(const struct iovec* iov, int iovcnt)
{
    int cc = IO::writevall(fd_, iov, iovcnt, get_notifier(), logpath_);
    monitor(IO::WRITEV, 0); // XXX/bowei

    return cc;
}

int
IPClient::timeout_read(char* bp, size_t len, int timeout_ms)
{
    int cc = IO::timeout_read(fd_, bp, len, timeout_ms, 
                            get_notifier(), logpath_);
    monitor(IO::READV, 0); // XXX/bowei

    return cc;
}

int
IPClient::timeout_readv(const struct iovec* iov, int iovcnt, int timeout_ms)
{
    int cc = IO::timeout_readv(fd_, iov, iovcnt, timeout_ms, 
                             get_notifier(), logpath_);
    monitor(IO::READV, 0); // XXX/bowei

    return cc;
}

int
IPClient::timeout_readall(char* bp, size_t len, int timeout_ms)
{
    int cc = IO::timeout_readall(fd_, bp, len, timeout_ms, 
                               get_notifier(), logpath_);
    monitor(IO::READV, 0); // XXX/bowei

    return cc;
}

int
IPClient::timeout_readvall(const struct iovec* iov, int iovcnt, int timeout_ms)
{
    int cc = IO::timeout_readvall(fd_, iov, iovcnt, timeout_ms, 
                                get_notifier(), logpath_);
    monitor(IO::READV, 0); // XXX/bowei

    return cc;
}

int
IPClient::get_nonblocking(bool *nonblockingp)
{
    int cc = IO::get_nonblocking(fd_, nonblockingp, logpath_);
    return cc;
}

int
IPClient::set_nonblocking(bool nonblocking)
{
    int cc = IO::set_nonblocking(fd_, nonblocking, logpath_);
    return cc;
}

} // namespace oasys
