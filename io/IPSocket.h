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
#ifndef _OASYS_IP_SOCKET_H_
#define _OASYS_IP_SOCKET_H_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "IO.h"
#include "../debug/Log.h"

namespace oasys {

// XXX/demmer this should be in some system header somewhere
#define MAX_UDP_PACKET 65536

#ifndef INADDR_NONE
#define INADDR_NONE 0
#endif /* INADDR_NONE */

/**
 * \class IPSocket
 *
 * IPSocket is a base class that wraps a network socket. It is a base
 * for TCPClient, TCPServer, and UDPSocket.
 */
class IPSocket : public Logger {
public:
    // Constructor / destructor
    IPSocket(const char* logbase, int socktype);
    IPSocket(int fd, in_addr_t remote_addr, u_int16_t remote_port,
             const char* logbase);
    virtual ~IPSocket();

    /// Set the socket parameters
    void configure();

    //@{
    /// System call wrappers
    virtual int bind(in_addr_t local_addr, u_int16_t local_port);
    virtual int connect(in_addr_t local_addr, u_int16_t local_port);
    virtual int close();
    virtual int shutdown(int how);
    
    virtual int send(const char* bp, size_t len, int flags);
    virtual int sendto(char* bp, size_t len, int flags,
                       in_addr_t addr, u_int16_t port);
    virtual int sendmsg(const struct msghdr* msg, int flags);
    
    virtual int recv(char* bp, size_t len, int flags);
    virtual int recvfrom(char* bp, size_t len, int flags,
                         in_addr_t *addr, u_int16_t *port);
    virtual int recvmsg(struct msghdr* msg, int flags);

    //@}
    
    /**
     * @brief Try to receive messages on binded port, but don't 
     * block for more than the timeout milliseconds.
     *
     * @return 0 on timeout, 1 on success, -1 on error
     */
    
    /// Wrapper around poll() for this socket's fd
    virtual inline int poll(int events, int* revents, int timeout_ms);
    
    /// Socket State values
    enum state_t {
        INIT, 		///< initial state
        LISTENING,	///< server socket, called listen()
        CONNECTING,	///< client socket, called connect()
        ESTABLISHED, 	///< connected socket, data can flow
        RDCLOSED,	///< shutdown(SHUT_RD) called, writing still enabled
        WRCLOSED,	///< shutdown(SHUT_WR) called, reading still enabled
        CLOSED,		///< shutdown called for both read and write
        FINI		///< close() called on the socket
    };
        
    /**
     * Return the current state.
     */
    state_t state() { return state_; }
        
    /**
     * Socket parameters are public fields that should be set after
     * creating the socket but before the socket is used.
     */
    struct ip_socket_params {
        ip_socket_params() :
            reuseaddr_    (1),
            tcp_nodelay_  (0),
            recv_bufsize_ (0),
            send_bufsize_ (0)
        {
        }
        
        u_int32_t reuseaddr_:1;		// default: on
        u_int32_t tcp_nodelay_:1;	// default: off
        u_int32_t _unused:29;
        
        int recv_bufsize_;		// default: system setting
        int send_bufsize_;		// default: system setting
    } params_;
    
    /// The socket file descriptor
    inline int fd();
    
    /// The local address that the socket is bound to
    inline in_addr_t local_addr();
                
    /// The local port that the socket is bound to
    inline u_int16_t local_port();
                          
    /// The remote address that the socket is connected to
    inline in_addr_t remote_addr();
                              
    /// The remote port that the socket is connected to
    inline u_int16_t remote_port();
                                  
    /// Set the local address that the socket is bound to
    inline void set_local_addr(in_addr_t addr);
                                      
    /// Set the local port that the socket is bound to
    inline void set_local_port(u_int16_t port);
                                          
    /// Set the remote address that the socket is connected to
    inline void set_remote_addr(in_addr_t addr);
                                              
    /// Set the remote port that the socket is connected to
    inline void set_remote_port(u_int16_t port);
                                                  
    /**
     * Hook to abort a test if any unexpected errors occur within the
     * socket. Implemented by trapping all the logf calls and aborting
     * on logs of error or higher.
     */
    static void abort_on_error() {
        abort_on_error_ = 1;
    }
    
    /// Wrapper around the logging function needed for abort_on_error
    inline int logf(log_level_t level, const char *fmt, ...) PRINTFLIKE(3, 4);
     
    /// logfd can be set to false to disable the appending of the
    /// socket file descriptor
    void set_logfd(bool logfd) { logfd_ = logfd; }
                                                          
protected:
    void init_socket();
    
    static int abort_on_error_;    
    
    int fd_;
    int socktype_;
    state_t state_;
    bool logfd_;
    
    in_addr_t local_addr_;
    u_int16_t local_port_;
    in_addr_t remote_addr_;
    u_int16_t remote_port_;
    
    void set_state(state_t state);
    const char* statetoa(state_t state);
    
    inline void get_local();
    inline void get_remote();
    
};

int
IPSocket::fd()
{
    return fd_;
}

in_addr_t
IPSocket::local_addr()
{
    if (local_addr_ == INADDR_NONE) get_local();
    return local_addr_;
}

u_int16_t
IPSocket::local_port()
{
    if (local_port_ == 0) get_local();
    return local_port_;
}

in_addr_t
IPSocket::remote_addr()
{
    if (remote_addr_ == INADDR_NONE) get_remote();
    return remote_addr_;
}

u_int16_t
IPSocket::remote_port()
{
    if (remote_port_ == 0) get_remote();
    return remote_port_;
}

void
IPSocket::set_local_addr(in_addr_t addr)
{
    local_addr_ = addr;
}

void
IPSocket::set_local_port(u_int16_t port)
{
    local_port_ = port;
}

void
IPSocket::set_remote_addr(in_addr_t addr)
{
    remote_addr_ = addr;
}

void
IPSocket::set_remote_port(u_int16_t port)
{
    remote_port_ = port;
}

void
IPSocket::get_local()
{
    if (fd_ < 0)
        return;
    
    struct sockaddr_in sin;
    socklen_t slen = sizeof sin;
    if (::getsockname(fd_, (struct sockaddr *)&sin, &slen) == 0) {
        local_addr_ = sin.sin_addr.s_addr;
        local_port_ = ntohs(sin.sin_port);
    }
}

void
IPSocket::get_remote()
{
    if (fd_ < 0)
        return;
           
    struct sockaddr_in sin;
    socklen_t slen = sizeof sin;
    if (::getpeername(fd_, (struct sockaddr *)&sin, &slen) == 0) {
        remote_addr_ = sin.sin_addr.s_addr;
        remote_port_ = ntohs(sin.sin_port);
    }
}

int
IPSocket::logf(log_level_t level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vlogf(level, fmt, ap);
    va_end(ap);

    if (abort_on_error_ && (level >= LOG_ERR)) {
        Logger::logf(LOG_CRIT, "aborting due to previous error");
        abort();
    }
        
    return ret;
}

int
IPSocket::poll(int events, int* revents, int timeout_ms)
{
    return IO::poll(fd_, events, revents, timeout_ms, logpath_);
}

} // namespace oasys
 
#endif /* _OASYS_IP_SOCKET_H_ */
