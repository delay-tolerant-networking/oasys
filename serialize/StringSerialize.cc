/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2005 Intel Corporation. All rights reserved. 
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

#include "StringSerialize.h"

namespace oasys {

StringSerialize::StringSerialize(context_t context, int options)
    : SerializeAction(Serialize::INFO, context, options)
{
    if (options_ & DOT_SEPARATED) {
        sep_ = '.';
    } else {
        sep_ = ' ';
    }
}

void
StringSerialize::process(const char* name, u_int32_t* i)
{
    if (options_ & INCLUDE_NAME) {
        buf_.append(name);
        buf_.append(sep_);
    }

    buf_.append_int(*i, 10);
    buf_.append(sep_);
}

void
StringSerialize::process(const char* name, u_int16_t* i)
{
    if (options_ & INCLUDE_NAME) {
        buf_.append(name);
        buf_.append(sep_);
    }
    
    buf_.append_int(*i, 10);
    buf_.append(sep_);
}

void
StringSerialize::process(const char* name, u_int8_t* i)
{
    if (options_ & INCLUDE_NAME) {
        buf_.append(name);
        buf_.append(sep_);
    }
    
    buf_.append_int(*i, 10);
    buf_.append(sep_);
}

void
StringSerialize::process(const char* name, bool* b)
{
    if (options_ & INCLUDE_NAME) {
        buf_.append(name);
        buf_.append(sep_);
    }

    if (*b) {
        buf_.append("true", 4);
    } else {
        buf_.append("false", 5);
    }
        
    buf_.append(sep_);
}

void
StringSerialize::process(const char* name, u_char* bp, size_t len)
{
    if (options_ & INCLUDE_NAME) {
        buf_.append(name);
        buf_.append(sep_);
    }

    buf_.append((const char*)bp, len);
    buf_.append(sep_);
}

void
StringSerialize::process(const char* name, std::string* s)
{
    if (options_ & INCLUDE_NAME) {
        buf_.append(name);
        buf_.append(sep_);
    }

    buf_.append(s->data(), s->length());
    buf_.append(sep_);
}

void
StringSerialize::process(const char* name, u_char** bp,
                         size_t* lenp, int flags)
{
    if (options_ & INCLUDE_NAME) {
        buf_.append(name);
        buf_.append(sep_);
    }
    
    if (flags & Serialize::NULL_TERMINATED) {
        buf_.append((const char*)*bp);
        buf_.append(sep_);
    } else {
        buf_.append((const char*)*bp, *lenp);
        buf_.append(sep_);
    }
}

void
StringSerialize::end_action()
{
    // trim trailing separator
    if (buf_.length() != 0) {
        buf_.trim(1);
    }
}

} // namespace oasys
