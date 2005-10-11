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
#include "io/IOClient.h"

// XXX/demmer don't malloc by default in the default constructor but
// rather wait to allow a reserve call to come in.

namespace oasys {

StringBuffer::StringBuffer(size_t initsz, const char* initstr)
    : exbuf_(0)
{
    exbuf_ = new ExpandableBuffer();
    ASSERT(exbuf_);
    
    int err = exbuf_->reserve(initsz, initsz);
    ASSERT(err == 0);

    if (initstr) {
        append(initstr);
    }
}

StringBuffer::StringBuffer(const char* fmt, ...)
    : exbuf_(0)
{
    exbuf_ = new ExpandableBuffer();
    ASSERT(exbuf_);

    if (fmt != 0) 
    {
        va_list ap;
        va_start(ap, fmt);
        vappendf(fmt, ap);
        va_end(ap);
    }
}

StringBuffer::StringBuffer(ExpandableBuffer* buffer)
    : exbuf_(buffer)
{
    ASSERT(exbuf_);
}

StringBuffer::StringBuffer(ExpandableBuffer* buffer, 
                           const char* fmt, ...) 
    : exbuf_(buffer)
{
    ASSERT(exbuf_);

    if (fmt != 0) 
    {
        va_list ap;
        va_start(ap, fmt);
        vappendf(fmt, ap);
        va_end(ap);
    }
}

StringBuffer::~StringBuffer() {
    delete_z(exbuf_);
}

size_t
StringBuffer::append(const char* str, size_t len)
{
    if (len == 0) {
        len = strlen(str);
    }
    
    int err = exbuf_->reserve(exbuf_->len_ + len);    
    ASSERT(err == 0);

    memcpy(exbuf_->buf_end(), str, len);
    exbuf_->add_to_len(len);
    
    return len;
}

size_t
StringBuffer::append(char c)
{
    reserve(exbuf_->len_ + 1);
    *exbuf_->buf_end() = c;
    exbuf_->add_to_len(1);

    return 1;
}

/**
 * Append len bytes from the given IOClient.
 */
void
StringBuffer::append(IOClient* io, size_t len)
{
    reserve(exbuf_->len_ + len);
    io->readall(exbuf_->buf_end(), len);
    // XXX/bowei -- need to handle errors from readall
    exbuf_->add_to_len(len);
}

size_t
StringBuffer::vappendf(const char* fmt, va_list ap)
{
    int ret = vsnprintf(exbuf_->buf_end(), exbuf_->nfree(), fmt, ap);
    
    if(ret == -1)
    {
        // Retarded glibc implementation in Fedora Core 1. From the
        // man pages:
        //
        // The glibc implementation of the functions snprintf and
        // vsnprintf conforms to the C99 standard, i.e., behaves as
        // described above, since glibc version 2.1. Until glibc 2.0.6
        // they would return -1 when the output was truncated.
        while(ret == -1)
        {
            exbuf_->reserve();
            ret = vsnprintf(exbuf_->buf_end(), exbuf_->nfree(), fmt, ap);
        }
    }
    else if(ret >= exbuf_->nfree())
    {
        exbuf_->reserve(ret + 1);
        ret = vsnprintf(exbuf_->buf_end(), exbuf_->nfree(), fmt, ap);
    }

    exbuf_->add_to_len(ret + 1);

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
