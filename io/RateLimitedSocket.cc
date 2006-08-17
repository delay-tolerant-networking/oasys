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

#include "RateLimitedSocket.h"

namespace oasys {

//----------------------------------------------------------------------
RateLimitedSocket::RateLimitedSocket(const char* logpath,
                                     u_int32_t rate,
                                     IPSocket* socket)
    : Logger("RateLimitedSocket", logpath),
      bucket_(logpath, rate, 65535 * 8 /* max udp packet */),
      socket_(socket)
{
}

//----------------------------------------------------------------------
int
RateLimitedSocket::send(const char* bp, size_t len, int flags)
{
    ASSERT(socket_ != NULL);

    bool can_send = bucket_.drain(len * 8);
    if (!can_send) {
        log_debug("can't send %zu byte packet since only %u tokens in bucket",
                  len, bucket_.tokens());
        return IORATELIMIT;
    }

    log_debug("%u tokens sufficient for %zu byte packet",
              bucket_.tokens(), len);

    return socket_->send(bp, len, flags);
}

//----------------------------------------------------------------------
int
RateLimitedSocket::sendto(char* bp, size_t len, int flags,
                          in_addr_t addr, u_int16_t port)
{
    ASSERT(socket_ != NULL);

    bool can_send = bucket_.drain(len * 8);
    if (!can_send) {
        log_debug("can't send %zu byte packet since only %u tokens in bucket",
                  len, bucket_.tokens());
        return IORATELIMIT;
    }

    log_debug("%u tokens sufficient for %zu byte packet",
              bucket_.tokens(), len);

    return socket_->sendto(bp, len, flags, addr, port);
}

} // namespace oasys
