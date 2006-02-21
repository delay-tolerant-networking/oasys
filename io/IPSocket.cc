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

#include "IPSocket.h"
#include "NetUtils.h"
#include "debug/Log.h"
#include "debug/DebugUtils.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

namespace oasys {

IPSocket::IPSocket(int socktype, const char* logbase)
    : Logger(logbase)
{
    state_       = INIT;
    local_addr_  = INADDR_ANY;
    local_port_  = 0;
    remote_addr_ = INADDR_NONE;
    remote_port_ = 0;
    fd_          = -1;
    socktype_    = socktype;
    logfd_       = true;
}

IPSocket::IPSocket(int socktype, int sock,
                   in_addr_t remote_addr, u_int16_t remote_port,
                   const char* logbase)
{
    fd_       = sock;
    socktype_ = socktype;
    logpathf("%s/%d", logbase, sock);
    
    state_       = ESTABLISHED;
    local_addr_  = INADDR_NONE;
    local_port_  = 0;
    remote_addr_ = remote_addr;
    remote_port_ = remote_port;
    
    configure();
}

IPSocket::~IPSocket()
{
    close();
}

void
IPSocket::init_socket()
{
    // should only be called at real init time or after a call to close()
    ASSERT(state_ == INIT || state_ == FINI);
    ASSERT(fd_ == -1);
    state_ = INIT;
    
    fd_ = socket(PF_INET, socktype_, 0);
    if (fd_ == -1) {
        logf(LOG_ERR, "error creating socket: %s", strerror(errno));
        return;
    }

    if (logfd_)
        Logger::logpath_appendf("/%d", fd_);
    
    logf(LOG_DEBUG, "created socket %d", fd_);
    
    configure();
}

const char*
IPSocket::statetoa(state_t state)
{
    switch (state) {
    case INIT: 		return "INIT";
    case LISTENING: 	return "LISTENING";
    case CONNECTING:	return "CONNECTING";
    case ESTABLISHED:	return "ESTABLISHED";
    case RDCLOSED:	return "RDCLOSED";
    case WRCLOSED:	return "WRCLOSED";
    case CLOSED:	return "CLOSED";
    case FINI:		return "FINI";
    }
    ASSERT(0);
    return NULL;
}

void
IPSocket::set_state(state_t state)
{
    logf(LOG_DEBUG, "state %s -> %s", statetoa(state_), statetoa(state));
    state_ = state;
}

int
IPSocket::bind(in_addr_t local_addr, u_int16_t local_port)
{
    struct sockaddr_in sa;

    if (fd_ == -1) init_socket();

    local_addr_ = local_addr;
    local_port_ = local_port;

    logf(LOG_DEBUG, "binding to %s:%d", intoa(local_addr), local_port);

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = local_addr_;
    sa.sin_port = htons(local_port_);
    if (::bind(fd_, (struct sockaddr*) &sa, sizeof(sa)) != 0) {
        int err = errno;
        logf(LOG_ERR, "error binding to %s:%d: %s",
             intoa(local_addr_), local_port_, strerror(err));
        return -1;
    }

    return 0;
}

int
IPSocket::connect(in_addr_t remote_addr, u_int16_t remote_port)
{
    struct sockaddr_in sa;
    
    if (fd_ == -1) init_socket();
    
    remote_addr_ = remote_addr;
    remote_port_ = remote_port;

    log_debug("connecting to %s:%d", intoa(remote_addr), remote_port);

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = remote_addr;
    sa.sin_port = htons(remote_port);
    
    set_state(CONNECTING);
    
    if (::connect(fd_, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        if (errno != EINPROGRESS) {
            logf(LOG_ERR, "error connecting to %s:%d: %s",
                 intoa(remote_addr_), remote_port_, strerror(errno));
        }
        return -1;
    }

    set_state(ESTABLISHED);

    return 0;
}

void
IPSocket::configure()
{
    if (params_.reuseaddr_) {
	int y = 1;
        logf(LOG_DEBUG, "setting SO_REUSEADDR");
	if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &y, sizeof y) != 0) {
	    logf(LOG_WARN, "error setting SO_REUSEADDR: %s",
		 strerror(errno));
	}
    }

    if (params_.reuseaddr_) {
#ifdef SO_REUSEPORT
	int y = 1;
        logf(LOG_DEBUG, "setting SO_REUSEPORT");
	if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &y, sizeof y) != 0) {
	    logf(LOG_WARN, "error setting SO_REUSEPORT: %s",
		 strerror(errno));
	}
#else
	logf(LOG_WARN, "error setting SO_REUSEPORT: not implemented");
#endif
    }
    
    if (socktype_ == SOCK_STREAM && params_.tcp_nodelay_) {
	int y = 1;
        logf(LOG_DEBUG, "setting TCP_NODELAY");
	if (::setsockopt(fd_, IPPROTO_IP, TCP_NODELAY, &y, sizeof y) != 0) {
	    logf(LOG_WARN, "error setting TCP_NODELAY: %s",
		 strerror(errno));
	}
    }
    
    if (params_.recv_bufsize_ > 0) {
        logf(LOG_DEBUG, "setting SO_RCVBUF to %d",
             params_.recv_bufsize_);
        
	if (::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF,
			 &params_.recv_bufsize_,
                         sizeof (params_.recv_bufsize_)) < 0)
        {
	    logf(LOG_WARN, "error setting SO_RCVBUF to %d: %s",
		 params_.recv_bufsize_, strerror(errno));
	}
    }
    
    if (params_.send_bufsize_ > 0) {
        logf(LOG_WARN, "setting SO_SNDBUF to %d",
             params_.send_bufsize_);
        
	if (::setsockopt(fd_, SOL_SOCKET, SO_SNDBUF,
			 &params_.send_bufsize_,
                         sizeof params_.send_bufsize_) < 0)
        {
	    logf(LOG_WARN, "error setting SO_SNDBUF to %d: %s",
		 params_.send_bufsize_, strerror(errno));
	}
    }
}
    
int
IPSocket::close()
{
    logf(LOG_DEBUG, "closing socket in state %s", statetoa(state_));

    if (fd_ == -1) {
        ASSERT(state_ == INIT || state_ == FINI);
        return 0;
    }
    
    if (::close(fd_) != 0) {
        logf(LOG_ERR, "error closing socket in state %s: %s",
             statetoa(state_), strerror(errno));
        return -1;
    }
    
    set_state(FINI);
    fd_ = -1;
    return 0;
}

int
IPSocket::shutdown(int how)
{
    const char* howstr;

    switch (how) {
    case SHUT_RD:   howstr = "R";  break;
    case SHUT_WR:   howstr = "W";  break;
    case SHUT_RDWR: howstr = "RW"; break;
        
    default:
        logf(LOG_ERR, "shutdown invalid mode %d", how);
        return -1;
    }
      
    logf(LOG_DEBUG, "shutdown(%s) state %s", howstr, statetoa(state_));
    
    if (state_ == INIT || state_ == FINI) {
        ASSERT(fd_ == -1);
        return 0;
    }
    
    if (::shutdown(fd_, how) != 0) {
        logf(LOG_ERR, "error in shutdown(%s) state %s: %s",
             howstr, statetoa(state_), strerror(errno));
    }
    
    if (state_ == ESTABLISHED) {
        if (how == SHUT_RD)	{ set_state(RDCLOSED); }
        if (how == SHUT_WR)	{ set_state(WRCLOSED); }
        if (how == SHUT_RDWR)	{ set_state(CLOSED); }
        
    } else if (state_ == RDCLOSED && how == SHUT_WR) {
        set_state(CLOSED);
        
    } else if (state_ == WRCLOSED && how == SHUT_RD) {
        set_state(CLOSED);

    } else {
        logf(LOG_ERR, "invalid state %s for shutdown(%s)",
             statetoa(state_), howstr);
        return -1;
    }

    return 0;
}

int
IPSocket::send(const char* bp, size_t len, int flags)
{
    return IO::send(fd_, bp, len, flags, get_notifier(), logpath_);
}

int
IPSocket::sendto(char* bp, size_t len, int flags,
                 in_addr_t addr, u_int16_t port)
{
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = addr;
    sa.sin_port = htons(port);

    return IO::sendto(fd_, bp, len, flags, (sockaddr*)&sa, 
                      sizeof(sa), get_notifier(), logpath_);
}

int
IPSocket::sendmsg(const struct msghdr* msg, int flags)
{
    return IO::sendmsg(fd_, msg, flags, get_notifier(), logpath_);
}

int
IPSocket::recv(char* bp, size_t len, int flags)
{
    return IO::recv(fd_, bp, len, flags, 
                    get_notifier(), logpath_);
}

int
IPSocket::recvfrom(char* bp, size_t len, int flags,
                   in_addr_t *addr, u_int16_t *port)
{
    struct sockaddr_in sa;
    socklen_t sl = sizeof(sa);
    memset(&sa, 0, sizeof(sa));
    
    int cc = IO::recvfrom(fd_, bp, len, flags, (sockaddr*)&sa, &sl, 
                          get_notifier(), logpath_);
    
    if (cc < 0) {
        if (cc != IOINTR)
            logf(LOG_ERR, "error in recv(): %s", strerror(errno));
        return cc;
    }

    if (addr)
        *addr = sa.sin_addr.s_addr;

    if (port)
        *port = htons(sa.sin_port);

    return cc;
}

int
IPSocket::recvmsg(struct msghdr* msg, int flags)
{
    return IO::recvmsg(fd_, msg, flags, get_notifier(), logpath_);
}

int
IPSocket::poll_sockfd(int events, int* revents, int timeout_ms)
{
    short s_events = events;
    short s_revents;
    
    int cc = IO::poll_single(fd_, s_events, &s_revents, timeout_ms, 
                             get_notifier(), logpath_);
    
    if (revents != 0) {
        *revents = s_revents;
    }

    return cc;
}

} // namespace oasys
