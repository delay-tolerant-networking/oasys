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

#ifndef __OASYS_REGEX_H__
#define __OASYS_REGEX_H__

#include <string>
#include <sys/types.h>
#include <regex.h>

namespace oasys {

class Regex {
public:
    static const size_t MATCH_LIMIT = 8;

    static int match(const char* regex, const char* str, 
                     int cflags = 0, int rflags = 0);

    Regex(const char* regex, int cflags = 0);
    virtual ~Regex();
    
    int match(const char* str, int flags = 0);

    bool valid() { return compilation_err_ == 0; }
    
    int num_matches();
    const regmatch_t& get_match(size_t i);
    
protected:
    int compilation_err_;

    regex_t    regex_;
    regmatch_t matches_[MATCH_LIMIT];
};

class Regsub : public Regex {
public:
    static int subst(const char* regex, const char* str,
                     const char* sub_spec, std::string* result,
                     int cflags = 0, int rflags = 0);
    
    Regsub(const char* regex, const char* sub_spec, int flags = 0);
    ~Regsub();
        
    int subst(const char* str, std::string* result, int flags = 0);

protected:
    std::string sub_spec_;
};
    
} // namespace oasys

#endif /* __OASYS_REGEX_H__ */
