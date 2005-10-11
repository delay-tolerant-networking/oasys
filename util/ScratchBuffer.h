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
#include <cstring>

#include "../debug/DebugUtils.h"
#include "../util/ExpandableBuffer.h"

namespace oasys {

/*!
 * ScratchBuffer template.
 *
 * @param _memory_t    Memory type to return from buf()
 *
 * @param _static_size Size of the buffer to allocate statically from
 *     the stack. This is useful where the size is commonly small, but
 *     unusual and large sizes need to be handled as well. Specifying 
 *     this value will can potentially save many calls to malloc().
 */
template<typename _memory_t = void*, size_t _static_size = 0>
class ScratchBuffer;

/*!
 * ScratchBuffer class that doesn't use a static stack to begin with.
 */
template<typename _memory_t>
class ScratchBuffer<_memory_t, 0> : public ExpandableBuffer {
public:
    ScratchBuffer(size_t size = 0) : ExpandableBuffer(size) {}
    
    //! @return Pointer of buffer of size, otherwise 0
    _memory_t buf(size_t size = 0) {
        if (size > len_) {
            int err = reserve(size, size);
            if (err != 0) {
                return 0;
            }

            len_ = size;
        }

        return reinterpret_cast<_memory_t>(buf_);
    }
};

/*!
 * ScratchBuffer class that uses a static stack allocated to begin
 * with.
 */
template<typename _memory_t, size_t _static_size>
class ScratchBuffer : public ExpandableBuffer {
public:
    ScratchBuffer() {
        buf_     = static_buf_; 
        buf_len_ = _static_size;
    }

    //! @return Pointer of buffer of size, otherwise 0
    _memory_t buf(size_t size = 0) {
        if (size > len_) {
            if (using_malloc() || size <= _static_size) {
                int err = reserve(size, size);
                if (err != 0) {
                    return 0;
                }
            }

            len_ = size;
        }

        return reinterpret_cast<_memory_t>(buf_);
    }

private:
    char static_buf_[_static_size]; // gcc automatically aligns to
                                    // maximal data type alignment
    bool using_malloc() { 
	return reinterpret_cast<char*>(buf_) != static_buf_;
    }
};

} // namespace oasys

#endif //__STATIC_SCRATCH_BUFFER_H__
