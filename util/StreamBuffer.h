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
#ifndef __STREAM_BUFFER_H__
#define __STREAM_BUFFER_H__

#include <stdlib.h>

namespace oasys {

/**
 * @brief Stream oriented resizable buffer.
 *
 * StreamBuffer is a resizable buffer which is designed to efficiently
 * support growing at the end of the buffer and consumption of the
 * buffer from the front.
 */
class StreamBuffer {
public:
    /**
     * Create a new StreamBuffer with initial size
     */
    StreamBuffer(size_t size = DEFAULT_BUFSIZE);
    ~StreamBuffer();

    /**
     * Set the size of the buffer. New size should not be smaller than
     * size of data in the StreamBuffer.
     */
    void set_size(size_t size);

    /** @return Pointer to the beginning of the data. */
    char* start();
    
    /** @return Pointer to the end of the data. */
    char* end();
    
    /** Reserve amount bytes in the buffer */
    void reserve(size_t amount);

    /** Fill amount bytes, e.g. move the end ptr up by that amount */
    void fill(size_t amount);
    
    /** Consume amount bytes from the front of the buffer */
    void consume(size_t amount);

    /** Clear all data from the buffer */
    void clear();
    
    /** Amount of bytes stored in the stream buffer */
    size_t fullbytes();

    /** Amount of free bytes available at the end of the buffer */
    size_t tailbytes();

private:
    /** Resize buffer to size. */
    void realloc(size_t size);

    /** Compact buffer, moving all data up to the start of the
     * dynamically allocated buffer, eliminating the empty space at
     * the front. */
    void moveup();

    size_t start_;
    size_t end_;
    size_t size_;

    char* buf_;

    static const size_t DEFAULT_BUFSIZE = 512;
};

} // namespace oasys
 
#endif //__STREAM_BUFFER_H__
