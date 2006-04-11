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

#ifndef _OASYS_NET_UTILS_H_
#define _OASYS_NET_UTILS_H_

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../compat/inttypes.h"

/**
 * Wrapper macro to give the illusion that intoa() is a function call.
 * Which it is, really... or more accurately two inlined calls and one
 * function call.
 */
#define intoa(addr) oasys::Intoa(addr).buf()

namespace oasys {

/**
 * Faster wrapper around inet_ntoa.
 */
extern const char* _intoa(u_int32_t addr, char* buf, size_t bufsize);

/**
 * Class used to allow for safe concurrent calls to _intoa within an
 * argument list.
 */
class Intoa {
public:
    Intoa(in_addr_t addr) {
        str_ = _intoa(addr, buf_, bufsize_);
    }
    
    ~Intoa() {
        buf_[0] = '\0';
    }
    
    const char* buf() { return str_; }

    static const int bufsize_ = sizeof(".xxx.xxx.xxx.xxx");
    
protected:
    char buf_[bufsize_];
    const char* str_;
};

/**
 * Utility wrapper around the ::gethostbyname() system call
 */
extern int gethostbyname(const char* name, in_addr_t* addrp);

/*
 * Various overrides of {ntoh,hton}{l,s} that take a char buffer,
 * which can be used on systems that require alignment for integer
 * operations.
 */

inline u_int32_t
safe_ntohl(const char* bp)
{
    u_int32_t netval;
    memcpy(&netval, bp, sizeof(netval));
    return ntohl(netval);
}

inline u_int16_t
safe_ntohs(const char* bp)
{
    u_int16_t netval;
    memcpy(&netval, bp, sizeof(netval));
    return ntohs(netval);
}

inline void
safe_htonl(u_int32_t val, char* bp)
{
    val = htonl(val);
    memcpy(bp, &val, sizeof(val));
}
    
inline void
safe_htons(u_int16_t val, char* bp)
{
    val = htons(val);
    memcpy(bp, &val, sizeof(val));
}


} // namespace oasys

#endif /* _OASYS_NET_UTILS_H_ */
