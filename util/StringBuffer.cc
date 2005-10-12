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

#include <stdlib.h>

#include "StringBuffer.h"

#include "ExpandableBuffer.h"
#include "io/IOClient.h"

namespace oasys {

StringBuffer::StringBuffer(size_t initsz, const char* initstr)
    : buf_(0), own_buf_(true)
{
    buf_ = new ExpandableBuffer();
    ASSERT(buf_ != 0);
    
    buf_->reserve((initsz == 0) ? 256 : initsz);

    if (initstr) {
        append(initstr);
    }
}
    
StringBuffer::StringBuffer(const char* fmt, ...)
    : buf_(0), own_buf_(true)
{
    buf_ = new ExpandableBuffer();
    ASSERT(buf_);
    buf_->reserve(256);

    if (fmt != 0) 
    {
        va_list ap;
        va_start(ap, fmt);
        vappendf(fmt, ap);
        va_end(ap);
    }
}

StringBuffer::StringBuffer(ExpandableBuffer* buffer)
    : buf_(buffer), own_buf_(false)
{
    ASSERT(buf_ != 0);
    buf_->reserve(256);
}

StringBuffer::StringBuffer(ExpandableBuffer* buffer, 
                           const char* fmt, ...) 
    : buf_(buffer), own_buf_(false)
{
    ASSERT(buf_ != 0);
    buf_->reserve(256);

    if (fmt != 0) 
    {
        va_list ap;
        va_start(ap, fmt);
        vappendf(fmt, ap);
        va_end(ap);
    }
}

StringBuffer::~StringBuffer()
{
    if (own_buf_)
        delete_z(buf_);
}

const char*
StringBuffer::c_str() const
{
    if (buf_->len() == 0 || 
        *buf_->at(buf_->len() - 1) != '\0') 
    {
        // Don't want the null termination to trigger a doubling
        // of the size.
        buf_->reserve(buf_->len() + 1);
        *buf_->end() = '\0';
        
        buf_->set_len(buf_->len() + 1);
    }
    
    return data();
}

size_t
StringBuffer::append(const char* str, size_t len)
{
    if (len == 0) {
        len = strlen(str);
    }
    
    // len is not past the end of str
    ASSERT(len == strnlen(str, len));

    buf_->reserve(buf_->len() + len);
    memcpy(buf_->end(), str, len);
    buf_->set_len(buf_->len() + len);
    
    return len;
}

size_t
StringBuffer::append(char c)
{
    buf_->reserve(buf_->len() + 1);
    *buf_->end() = c;
    buf_->set_len(buf_->len() + 1);

    return 1;
}

size_t
StringBuffer::vappendf(const char* fmt, va_list ap)
{
    if (buf_->nfree() == 0) {
        buf_->reserve();
    }
    
    int ret = vsnprintf(buf_->end(), buf_->nfree(), fmt, ap);
    
    if(ret == -1)
    {
        // Retarded glibc implementation in Fedora Core 1. From the
        // man pages:
        //
        // The glibc implementation of the functions snprintf and
        // vsnprintf conforms to the C99 standard, i.e., behaves as
        // described above, since glibc version 2.1. Until glibc 2.0.6
        // they would return -1 when the output was truncated.
        do {
            buf_->reserve();
            ret = vsnprintf(buf_->end(), buf_->nfree(), fmt, ap);
        } while(ret == -1);
        
        // we should be safe now, either the string has been written
        // or we write it again below
    }

    if(ret >= buf_->nfree())
    {
        buf_->reserve(buf_->len() + ret + 1);
        ret = vsnprintf(buf_->end(), buf_->nfree(), fmt, ap);
        ASSERT(ret > 0 && ret <= buf_->nfree());
    }
    
    ASSERT(ret > 0);
    buf_->set_len(buf_->len() + ret + 1);
        
    return ret;
}

size_t
StringBuffer::appendf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    size_t ret = vappendf(fmt, ap);
    va_end(ap);
    return ret;
}

} // namespace oasys
