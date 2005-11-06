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
    : err_(0)
{
    err_ = regcomp(&regex_, regex, cflags);
}

Regex::~Regex()
{
    if (err_ == 0)
        regfree(&regex_);
}

int
Regex::match(const char* str, int flags)
{
    if (err_ == 0) {
        int err = regexec(&regex_, str, MATCH_LIMIT, matches_, flags);
        return err;
    }

    return err_;
}

int 
Regex::match(const char* regex, const char* str, int cflags, int rflags)
{
    Regex r(regex, cflags);
    return r.match(str, rflags);
}

int
Regex::subst(const char* str, const char* sub_str,
             std::string* result, int flags)
{
    match(str, flags);
    if (err_ != 0) {
        return err_;
    }

    size_t len = strlen(sub_str);
    size_t i = 0;
    int nmatches = num_matches();

    result->clear();
    
    while (i < len) {
        if (sub_str[i] == '\\') {

            // safe since there's a trailing null in sub_str
            char c = sub_str[i + 1];

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
                err_ = REG_ESUBREG;
                result->clear();
                return err_;
            }
            
        } else {
            // just copy the character
            result->push_back(sub_str[i]);
            ++i;
        }
    }

    return err_;
}

int
Regex::subst(const char* regex, const char* str,
             const char* subst, std::string* result,
             int cflags, int rflags)
{
    Regex r(regex, cflags);
    return r.subst(str, subst, result, rflags);
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

} // namespace oasys
