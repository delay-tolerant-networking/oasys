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

#include "../debug/DebugUtils.h"
#include "Regex.h"

namespace oasys {

Regex::Regex(const char* regex, int cflags)
{
    compilation_err_ = regcomp(&regex_, regex, cflags);
}

Regex::~Regex()
{
    if (compilation_err_ == 0)
        regfree(&regex_);
}

int
Regex::match(const char* str, int flags)
{
    if (compilation_err_ != 0) {
        return compilation_err_;
    }
    
    return regexec(&regex_, str, MATCH_LIMIT, matches_, flags);
}

int 
Regex::match(const char* regex, const char* str, int cflags, int rflags)
{
    Regex r(regex, cflags);
    return r.match(str, rflags);
}

int
Regex::num_matches()
{
    for(size_t i = 0; i<MATCH_LIMIT; ++i) {
        if (matches_[i].rm_so == -1) {
            return i;
        }
    }

    return MATCH_LIMIT;
}

const regmatch_t&
Regex::get_match(size_t i)
{
    ASSERT(i <= MATCH_LIMIT);
    return matches_[i];
}

std::string
Regex::regerror_str(int err)
{
    char buf[1024];
    size_t len = regerror(err, &regex_, buf, sizeof(buf));
    return std::string(buf, len);
}

Regsub::Regsub(const char* regex, const char* sub_spec, int flags)
    : Regex(regex, flags), sub_spec_(sub_spec)
{
}

Regsub::~Regsub()
{
}

int
Regsub::subst(const char* str, std::string* result, int flags)
{
    int match_err = match(str, flags);
    if (match_err != 0) {
        return match_err;
    }

    size_t len = sub_spec_.length();
    size_t i = 0;
    int nmatches = num_matches();

    result->clear();
    
    while (i < len) {
        if (sub_spec_[i] == '\\') {

            // safe since there's a trailing null in sub_spec
            char c = sub_spec_[i + 1];

            // handle '\\'
            if (c == '\\') {
                result->push_back('\\');
                result->push_back('\\');
                i += 2;
                continue;
            }

            // handle \0, \1, etc
            int match_num = c - '0';
            if ((match_num >= 0) && (match_num < nmatches))
            {
                regmatch_t* match = &matches_[match_num];
                result->append(str + match->rm_so, match->rm_eo - match->rm_so);
                i += 2;
                continue;
            }
            else
            {
                // out of range
                result->clear();
                return REG_ESUBREG;;
            }
            
        } else {
            // just copy the character
            result->push_back(sub_spec_[i]);
            ++i;
        }
    }

    return 0;
}

int
Regsub::subst(const char* regex, const char* str,
              const char* sub_spec, std::string* result,
              int cflags, int rflags)
{
    Regsub r(regex, sub_spec, cflags);
    return r.subst(str, result, rflags);
}

} // namespace oasys
