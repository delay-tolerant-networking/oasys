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

#include "TCPClient.h"
#include "NetUtils.h"
#include "debug/DebugUtils.h"
#include "debug/Log.h"

namespace oasys {

TCPClient::TCPClient(const char* logbase)
    : IPClient(SOCK_STREAM, logbase)
{
}

TCPClient::TCPClient(int fd, in_addr_t remote_addr, u_int16_t remote_port,
                     const char* logbase)
    : IPClient(SOCK_STREAM, fd, remote_addr, remote_port, logbase)
{
}

int
TCPClient::timeout_connect(in_addr_t remote_addr, u_int16_t remote_port,
                           int timeout_ms, int* errp)
{
    int ret, err;
    socklen_t len = sizeof(err);

    if (fd_ == -1) init_socket();

    if (IO::set_nonblocking(fd_, true) < 0) {
        log_err("error setting fd %d to nonblocking: %s",
                fd_, strerror(errno));
        if (errp) *errp = errno;
        return IOERROR;
    }
    
    ret = IPSocket::connect(remote_addr, remote_port);
    
    if (ret == 0)
    {
        log_debug("timeout_connect: succeeded immediately");
        if (errp) *errp = 0;
        ASSERT(state_ == ESTABLISHED); // set by IPSocket
    }
    else if (ret < 0 && errno != EINPROGRESS)
    {
        log_err("timeout_connect: error from connect: %s",
                strerror(errno));
        if (errp) *errp = errno;
        ret = IOERROR;
    }
    else
    {
        ASSERT(errno == EINPROGRESS);
        log_debug("EINPROGRESS from connect(), calling poll()");
        ret = IO::poll(fd_, POLLOUT, NULL, timeout_ms, logpath_);
        
        if (ret < 0) {
            log_err("error in poll(): %s", strerror(errno));
            if (errp) *errp = errno;
            ret = IOERROR;
        }
        else if (ret == 0)
        {
            log_debug("timeout_connect: poll timeout");
            ret = IOTIMEOUT;
        }
        else
        {
            ASSERT(ret == 1);
            // call getsockopt() to see if connect succeeded
            ret = getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
            ASSERT(ret == 0);
            if (err == 0) {
                log_debug("return from poll, connect succeeded");
                ret = 0;
                set_state(ESTABLISHED);
            } else {
                log_debug("return from poll, connect failed");
                ret = IOERROR;
            }
        }
    }

    // always make sure to set the fd back to blocking
    if (IO::set_nonblocking(fd_, false) < 0) {
        log_err("error setting fd %d back to blocking: %s",
                fd_, strerror(errno));
        if (errp) *errp = errno;
        return IOERROR;
    }

    return ret;
}

} // namespace oasys
