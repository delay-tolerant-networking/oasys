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

#include "NetUtils.h"
#include "TCPServer.h"
#include "debug/Debug.h"
#include "debug/Log.h"

#include <sys/errno.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace oasys {

TCPServer::TCPServer(char* logbase)
    : IPSocket(logbase, SOCK_STREAM)
{
    params_.reuseaddr_ = 1;
}

int
TCPServer::listen()
{
    logf(LOG_DEBUG, "listening");
    ASSERT(fd_ != -1);

    if (::listen(fd_, SOMAXCONN) == -1) {
        logf(LOG_ERR, "error in listen(): %s",strerror(errno));
        return -1;
    }
    
    set_state(LISTENING);
    return 0;
}
    
int
TCPServer::accept(int *fd, in_addr_t *addr, u_int16_t *port)
{
    ASSERT(state_ == LISTENING);
    
    struct sockaddr_in sa;
    socklen_t sl = sizeof(sa);
    memset(&sa, 0, sizeof(sa));
    int ret = ::accept(fd_, (sockaddr*)&sa, &sl);
    if (ret == -1) {
        if (errno != EINTR)
            logf(LOG_ERR, "error in accept(): %s", strerror(errno));
        return ret;
    }
    
    *fd = ret;
    *addr = sa.sin_addr.s_addr;
    *port = ntohs(sa.sin_port);

    return 0;
}

int
TCPServer::timeout_accept(int *fd, in_addr_t *addr, u_int16_t *port,
                          int timeout_ms)
{
    int ret = poll(POLLIN, NULL, timeout_ms);

    if (ret < 0)  return IOERROR;
    if (ret == 0) return IOTIMEOUT;
    ASSERT(ret == 1);

    ret = accept(fd, addr, port);

    if (ret < 0) {
        return IOERROR;
    }

    return 1; // accept'd
}

void
TCPServerThread::run()
{
    int fd;
    in_addr_t addr;
    u_int16_t port;

    while (1) {
        // check if someone has told us to quit by setting the
        // should_stop flag. if so, we're all done.
        if (should_stop())
            break;
        
        // block in accept waiting for new connections
        if (accept(&fd, &addr, &port) != 0) {
            if (errno == EINTR)
                continue;

            logf(LOG_ERR, "error in accept(): %d %s", errno, strerror(errno));
            close();
            break;
        }
        
        logf(LOG_DEBUG, "accepted connection fd %d from %s:%d",
             fd, intoa(addr), port);

        accepted(fd, addr, port);
    }
}

int
TCPServerThread::bind_listen_start(in_addr_t local_addr,
                                   u_int16_t local_port)
{
    int ret = bind(local_addr, local_port) || listen();
    if(!ret)
    {
        start();
    }

    return ret;
}

} // namespace oasys
