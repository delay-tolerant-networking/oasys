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

#ifndef tier_md5_h
#define tier_md5_h

#include <sys/types.h>
#include <string>
#include "StringUtils.h"

extern "C" {
#define PROTOTYPES 1
#include "md5-rsa.h"
}

#define MD5LEN 16

/**
 * Simple wrapper class to calculate an MD5 digest.
 */
class MD5 {
public:
    MD5() {
        init();
    }
    ~MD5() {}

    void init()
    {
        MD5Init(&ctx_);
    }
    
    void update(const u_char* data, size_t len)
    {
        MD5Update(&ctx_, (u_char*)data, len);
    }

    void update(const char* data, size_t len)
    {
        MD5Update(&ctx_, (u_char*)data, len);
    }

    void finalize()
    {
        MD5Final(digest_, &ctx_);
    }
    
    const u_char* digest()
    {
        return digest_;
    }

    static void digest_ascii(std::string* str,
                             const u_char* digest)
    {
        hex2str(str, digest, MD5LEN);
    }

    static std::string digest_ascii(const u_char* digest)
    {
        std::string str;
        digest_ascii(&str, digest);
        return str;
    }

    void digest_ascii(std::string* str)
    {
        digest_ascii(str, digest_);
    }

    std::string digest_ascii()
    {
        return digest_ascii(digest_);
    }

    static void digest_fromascii(const char* str, u_char* digest)
    {
        str2hex(str, digest, MD5LEN);
    }

private:
    MD5_CTX ctx_;
    u_char digest_[MD5LEN];
};

#endif
