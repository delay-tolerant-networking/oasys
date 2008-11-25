/*
 *    Copyright 2006 Intel Corporation
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

#ifdef HAVE_CONFIG_H
#  include <oasys-config.h>
#endif

#include "RateLimitedSocket.h"

namespace oasys {

//----------------------------------------------------------------------
RateLimitedSocket::RateLimitedSocket(const char* logpath,
                                     u_int32_t rate,
                                     IPSocket* socket)
    : Logger("RateLimitedSocket", "%s", logpath),
      bucket_(logpath, rate, 65535 * 8 /* max udp packet */),
      socket_(socket)
{
}

//----------------------------------------------------------------------
int
RateLimitedSocket::send(const char* bp, size_t len, int flags)
{
    ASSERT(socket_ != NULL);

    if (bucket_.rate() != 0) {
        bool can_send = bucket_.try_to_drain(len * 8);
        if (!can_send) {
            log_debug("can't send %zu byte packet since only %llu tokens in bucket",
                      len, U64FMT(bucket_.tokens()));
            return IORATELIMIT;
        }

        log_debug("%llu tokens sufficient for %zu byte packet",
                  U64FMT(bucket_.tokens()), len);
    }

    return socket_->send(bp, len, flags);
}

//----------------------------------------------------------------------
int
RateLimitedSocket::sendto(char* bp, size_t len, int flags,
                          in_addr_t addr, u_int16_t port)
{
    ASSERT(socket_ != NULL);

    if (bucket_.rate() != 0) {
        bool can_send = bucket_.try_to_drain(len * 8);
        if (!can_send) {
            log_debug("can't send %zu byte packet since only %llu tokens in bucket",
                      len, U64FMT(bucket_.tokens()));
            return IORATELIMIT;
        }

        log_debug("%llu tokens sufficient for %zu byte packet",
                  U64FMT(bucket_.tokens()), len);
    }

    return socket_->sendto(bp, len, flags, addr, port);
}

} // namespace oasys
