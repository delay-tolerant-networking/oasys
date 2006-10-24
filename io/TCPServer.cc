/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */


#include "NetUtils.h"
#include "TCPServer.h"
#include "debug/DebugUtils.h"
#include "debug/Log.h"

#include <errno.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace oasys {

TCPServer::TCPServer(const char* logbase)
    : IPSocket(SOCK_STREAM, logbase)
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
    ASSERTF(state_ == LISTENING,
            "accept() expected state LISTENING, not %s", statetoa(state_));
    
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

    monitor(IO::ACCEPT, 0); // XXX/bowei

    return 0;
}

int
TCPServer::timeout_accept(int *fd, in_addr_t *addr, u_int16_t *port,
                          int timeout_ms)
{
    int ret = poll_sockfd(POLLIN, NULL, timeout_ms);

    if (ret != 1) return ret;
    ASSERT(ret == 1);

    ret = accept(fd, addr, port);

    if (ret < 0) {
        return IOERROR;
    }

    monitor(IO::ACCEPT, 0); // XXX/bowei

    return 0; 
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
        
        // check the accept_timeout parameter to see if we should
        // block or poll when calling accept
        int ret;
        if (accept_timeout_ == -1) {
            ret = accept(&fd, &addr, &port);
        } else {
            ret = timeout_accept(&fd, &addr, &port, accept_timeout_);
            if (ret == IOTIMEOUT)
                continue;
        }

        if (ret != 0) {
            if (errno == EINTR)
                continue;
            
            logf(LOG_ERR, "error %d in accept(): %d %s",
                 ret, errno, strerror(errno));
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
    if (bind(local_addr, local_port) != 0)
        return -1;
    
    if (listen() != 0)
        return -1;
    
    start();
    
    return 0;
}

} // namespace oasys
