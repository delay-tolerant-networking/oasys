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
{
    buflen_ = initsz;
    buf_ = (char*)malloc(buflen_);
    len_ = 0;
    if (initstr) append(initstr);
}

StringBuffer::StringBuffer(const char* fmt, ...)
{
    buflen_ = 256;
    buf_ = (char*)malloc(buflen_);
    len_ = 0;

    if (fmt != 0) {
        va_list ap;
        va_start(ap, fmt);
        size_t ret = vsnprintf(&buf_[0], buflen_, fmt, ap);

        if (ret >= buflen_) {
            reserve(ret);
            ret = vsnprintf(&buf_[0], buflen_, fmt, ap);
        }
        
        len_ += ret;
        va_end(ap);
    }
}

StringBuffer::~StringBuffer()
{
    free(buf_);
    buf_ = 0;
    buflen_ = 0;
    len_ = 0;
}

void
StringBuffer::reserve(size_t sz, size_t grow)
{
    if ((len_ + sz) > buflen_) {
        if (grow == 0) {
            grow = buflen_ * 2;
        }
        
        buflen_ = grow;

        // make sure it's enough
        while ((len_ + sz) > buflen_) {
            buflen_ *= 2;
        }
        
        buf_ = (char*)realloc(buf_, buflen_);
    }
}

size_t
StringBuffer::append(const char* str, size_t len)
{
    if (len == 0) {
        len = strlen(str);
    }
    
    reserve(len_ + len);
    
    memcpy(&buf_[len_], str, len);
    len_ += len;
    return len;
}

size_t
StringBuffer::append(char c)
{
    reserve(len_ + 1);
    buf_[len_++] = c;
    return 1;
}

/**
 * Append len bytes from the given IOClient.
 */
void
StringBuffer::append(IOClient* io, size_t len)
{
    reserve(len_ + len);
    io->readall(buf_, len);
    len_ += len;
}

size_t
StringBuffer::vappendf(const char* fmt, va_list ap)
{
    int nfree = buflen_ - len_;
    int ret = vsnprintf(&buf_[len_], nfree, fmt, ap);
    
    if(ret == -1)
    {
        // Retarded glibc implementation. From the man pages:
        //
        // The glibc implementation of the functions snprintf and
        // vsnprintf con- forms to the C99 standard, i.e., behaves as
        // described above, since glibc version 2.1. Until glibc 2.0.6
        // they would return -1 when the out- put was truncated.
        while(ret == -1)
        {
            reserve(buflen_ * 2);
            nfree = buflen_ - len_;
            
            ret = vsnprintf(&buf_[len_], nfree, fmt, ap);

            log_debug("/stringbuffer", "ret = %d", ret);
        }
    }
    else if(ret >= nfree)
    {
        while(nfree <= ret)
        {
            reserve(buflen_ * 2);
            nfree = buflen_ - len_;
        }
        
        ret = vsnprintf(&buf_[len_], nfree, fmt, ap);
    }

    len_ += ret;

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
