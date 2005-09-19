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

#ifndef _OASYS_TCP_SERVER_H_
#define _OASYS_TCP_SERVER_H_

#include "IPSocket.h"
#include "../thread/Thread.h"

namespace oasys {

/**
 * \class TCPServer
 *
 * Wrapper class for a tcp server socket.
 */
class TCPServer : public IPSocket {
public:
    TCPServer(char* logbase = "/tcpserver");

    //@{
    /// System call wrapper
    int listen();
    int accept(int *fd, in_addr_t *addr, u_int16_t *port);
    //@}

    /**
     * @brief Try to accept a new connection, but don't block for more
     * than the timeout milliseconds.
     *
     * @return 0 on timeout, 1 on success, -1 on error
     */
    int timeout_accept(int *fd, in_addr_t *addr, u_int16_t *port, 
                       int timeout_ms);
};

/**
 * \class TCPServerThread
 *
 * Simple class that implements a thread of control that loops,
 * blocking on accept(), and issuing the accepted() callback when new
 * connections arrive.
 */
class TCPServerThread : public TCPServer, public Thread {
public:
    TCPServerThread(char* logbase = "/tcpserver",
                    int flags = 0)
        : TCPServer(logbase), Thread(flags)
    {
    }
    
    /**
     * Virtual callback hook that gets called when new connections
     * arrive.
     */
    virtual void accepted(int fd, in_addr_t addr, u_int16_t port) = 0;

    /**
     * Loop forever, issuing blocking calls to TCPServer::accept(),
     * then calling the accepted() function when new connections
     * arrive
     * 
     * Note that unlike in the Thread base class, this run() method is
     * public in case we don't want to actually create a new thread
     * for this guy, but instead just want to run the main loop.
     */
    void run();

    /**
     * @brief Bind to an address, open the port for listening and
     * start the thread.
     *
     * Most uses of TcpServerThread will simply call these functions
     * in sequence, so this helper function is to merge such
     * redundancy.
     *
     * @return -1 on error, 0 otherwise.
     */
    int bind_listen_start(in_addr_t local_addr, u_int16_t local_port);
};

} // namespace oasys
                        
#endif /* _OASYS_TCP_SERVER_H_ */
