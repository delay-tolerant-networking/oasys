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

#ifndef _OASYS_MD5_H_
#define _OASYS_MD5_H_

#include <sys/types.h>
#include <string>

#include "StringUtils.h"
#include "../serialize/Serialize.h"

extern "C" {
#define PROTOTYPES 1
#include "md5-rsa.h"
}

namespace oasys {
/**
 * Simple wrapper class to calculate an MD5 digest.
 */
class MD5 {
public:
    static const unsigned int MD5LEN = 16;
    
    MD5();
    ~MD5() {}
    
    /*! MD5 initialization. Begins an MD5 operation, writing a new context. */
    void init();

    /*! Update the md5 hash with data bytes */ 
    void update(const u_char* data, size_t len);

    /*! Update the md5 hash with data bytes */ 
    void update(const char* data, size_t len);

    /*! Finish up the md5 hashing process */
    void finalize();
    
    /*! \return MD5 hash value. */
    const u_char* digest();

    /*! \return MD5 hash value in ascii, std::string varient */
    static void digest_ascii(std::string* str, const u_char* digest);

    /*! \return MD5 hash value in ascii */
    static std::string digest_ascii(const u_char* digest);

    /*! \return MD5 hash value in ascii */
    void digest_ascii(std::string* str);

    /*! \return MD5 hash value in ascii, std::string varient */
    std::string digest_ascii();

    /*! Obtain the digest from ascii */
    static void digest_fromascii(const char* str, u_char* digest);

private:
    MD5_CTX ctx_;
    u_char digest_[MD5LEN];
};

/**
 * Helper for storing the hash
 */
struct MD5Hash_t : public SerializableObject {
    u_char hash_[MD5::MD5LEN];
    
    MD5Hash_t& operator=(const MD5Hash_t& hash);

    // virtual from SerializableObject
    void serialize(SerializeAction* a);
};

} // namespace oasys

#endif /* _OASYS_MD5_H_ */
