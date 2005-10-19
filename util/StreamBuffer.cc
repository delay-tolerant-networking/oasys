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

#include "debug/DebugUtils.h"
#include "StreamBuffer.h"

namespace oasys {

/***************************************************************************
 *
 * StreamBuffer
 *
 **************************************************************************/
StreamBuffer::StreamBuffer(size_t size) : 
    start_(0), end_(0), size_(size)
{
    if(size_ == 0)
        size_ = 4;

    buf_ = static_cast<char*>(malloc(size_));
    ASSERT(buf_);
}

StreamBuffer::~StreamBuffer()
{
    if(buf_) 
    {
        free(buf_);
        buf_ = 0;
    }
}

void
StreamBuffer::set_size(size_t size)
{
    ASSERT(fullbytes() <= size);
    moveup();
    
    realloc(size);
}

char*
StreamBuffer::start()
{
    return &buf_[start_];
}

char*
StreamBuffer::end()
{
    return &buf_[end_];
}

void
StreamBuffer::reserve(size_t amount)
{
    if (amount <= tailbytes())
    {
        // do nothing
    } 
    else if(amount <= start_) 
    {
        moveup();
    } 
    else
    {
        moveup();
        realloc(((amount + fullbytes())> size_*2) ? 
                (amount + fullbytes()): (size_*2));
    }
}

void
StreamBuffer::fill(size_t amount)
{
    ASSERT(amount <= tailbytes());
    
    end_ += amount;
}

void
StreamBuffer::consume(size_t amount)
{
    ASSERT(amount <= fullbytes());

    start_ += amount;
    if(start_ == end_)
    {
        start_ = end_ = 0;
    }
}

void
StreamBuffer::clear()
{
    start_ = end_ = 0;
}

size_t
StreamBuffer::fullbytes() 
{
    return end_ - start_;
}

size_t
StreamBuffer::tailbytes() 
{
    return size_ - end_;
}

void
StreamBuffer::realloc(size_t size)
{
    if(size < size_)
        return;

    buf_ = (char*)::realloc(buf_, size);
    if(buf_ == 0)
    {
        logf("/StreamBuffer", LOG_CRIT, "Out of memory");
        ASSERT(0);
    }
    
    size_ = size;
}

void
StreamBuffer::moveup()
{
    memmove(&buf_[0], &buf_[start_], end_ - start_);
    end_   = end_ - start_;
    start_ = 0;
}

} // namespace oasys
