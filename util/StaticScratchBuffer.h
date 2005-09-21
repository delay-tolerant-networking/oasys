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
#ifndef __STATIC_SCRATCH_BUFFER__
#define __STATIC_SCRATCH_BUFFER__

#include <cstdlib>
#include "../debug/DebugUtils.h"

namespace oasys {

/**
 * An initially stack allocated chunk of memory that switches to
 * malloc if it gets too big. Useful for those occasions when the
 * common case is under a certain size, but arbitrary sizes also need
 * to be handled.
 */
template<size_t _size, typename _memory_t = void*>
class StaticScratchBuffer {
public:
    StaticScratchBuffer()
        : buf_(reinterpret_cast<_memory_t>(static_buf_)), 
	  size_(_size)
    {}

    ~StaticScratchBuffer() { 
	if (using_malloc())
	    free(buf_);
    }
    
    _memory_t buf() { return buf_; }
    _memory_t buf(size_t size) {
        if (size > size_)
        {
	    if (! using_malloc()) {
		buf_ = static_cast<_memory_t>(malloc(size));
		ASSERT(buf_);

		memcpy(buf_, static_buf_, size_);
	    } else {
		buf_ = static_cast<_memory_t>(realloc(buf_, size));
		ASSERT(buf_);
	    }
            size_ = size;
        }

        return buf_;
    }
    size_t size() { return size_; }
    
private:
    _memory_t buf_;
    size_t    size_;
    char      static_buf_[_size];

    bool using_malloc() { 
	return reinterpret_cast<char*>(buf_) != static_buf_; 
    }
};

} // namespace oasys

#endif //__STATIC_SCRATCH_BUFFER_H__