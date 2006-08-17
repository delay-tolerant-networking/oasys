/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2006 Intel Corporation. All rights reserved. 
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
#ifndef _OASYS_RATE_LIMITED_SOCKET_H_
#define _OASYS_RATE_LIMITED_SOCKET_H_

// XXX/demmer for now this only works for IPSocket, but once Bowei
// finishes the IO porting, there will be a new base class that this
// class should use
#include "IPSocket.h"
#include "../debug/Log.h"
#include "../util/TokenBucket.h"

namespace oasys {

/**
 * The RateLimitedSocket class contains a socket class and a token
 * bucket and provides an interface to send data only if there is
 * enough space to send it out in.
 *
 * Note that the rate is configured in bits per second.
 */
class RateLimitedSocket : public Logger {
public:
    /**
     * Constructor.
     */
    RateLimitedSocket(const char* logpath,
                      u_int32_t rate,
                      IPSocket* socket = NULL);

    /**
     * Send the given data on the socket iff the rate controller
     * indicates that there is space.
     *
     * @return IORATELIMIT if there isn't space in the token bucket
     * for the given number of bytes, the return from IPSocket::send
     * if there is space.
     */
    int send(const char* bp, size_t len, int flags);

    /**
     * Send the given data on the socket iff the rate controller
     * indicates that there is space.
     *
     * @return IORATELIMIT if there isn't space in the token bucket
     * for the given number of bytes, the return from IPSocket::sendto
     * if there is space.
     */
    int sendto(char* bp, size_t len, int flags,
               in_addr_t addr, u_int16_t port);
    
    
    /// @{ Accessors
    TokenBucket* bucket() { return &bucket_; }
    IPSocket*    socket() { return socket_; }
    /// @}

    /// @{ Setters
    void set_socket(IPSocket* sock) { socket_ = sock; }
    /// @}
    
protected:
    TokenBucket bucket_;
    IPSocket*   socket_;
};

} // namespace oasys

#endif /* _OASYS_RATE_LIMITED_SOCKET_H_ */
