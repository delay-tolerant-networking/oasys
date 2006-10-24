/*
 *    Copyright 2005-2006 Intel Corporation
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
