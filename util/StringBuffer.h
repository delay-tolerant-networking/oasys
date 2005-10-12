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

#ifndef _OASYS_STRING_BUFFER_H_
#define _OASYS_STRING_BUFFER_H_

#include <algorithm>

#include "../debug/Log.h" 	// for PRINTFLIKE macro
#include "ScratchBuffer.h"

namespace oasys {

class ExpandableBuffer;

/**
 * Utility class that wraps a growable string buffer, similar to
 * std::ostringstream, but with printf() style arguments instead of
 * the << operator.
 *
 * The initial size of the buffer is 256 bytes, but can be overridden
 * at the constructor and/or through the reserve() function call.
 */
class StringBuffer {
public:
    /*!
     * @param initsz the initial buffer size
     * @param initstr the initial buffer contents 
     */
    StringBuffer(size_t initsz = 256, const char* initstr = 0);

    //! @param fmt the initial buffer contents
    StringBuffer(const char* fmt, ...) PRINTFLIKE(2, 3);

    //! @{ Create a string buffer with an expandable buffer 
    StringBuffer(ExpandableBuffer* buffer, const char* fmt, ...) 
        PRINTFLIKE(3,4);
    StringBuffer(ExpandableBuffer* buffer);
    //! @}
        
    ~StringBuffer();

    /**
     * @return the data buffer (const variant).
     */
    const char* data() const { return buf_->raw_buf(); }

    /**
     * @return the data buffer (non-const variant)
     */
    char* data() { return buf_->raw_buf(); }

    /**
     * @return length of the buffer.
     */
    size_t length() const { return buf_->len(); }

    /**
     * @return the data buffer, ensuring null termination.
     */
    const char* c_str() const;

    /**
     * Append the string to the tail of the buffer.
     *
     * @param str string data
     * @param len string length (if unspecified, will call strlen())
     * @return the number of bytes written
     */
    size_t append(const char* str, size_t len = 0);

    /**
     * Append the string to the tail of the buffer.
     *
     * @param str string data
     * @return the number of bytes written
     */
    size_t append(const std::string& str)
    {
        return append(str.data(), str.length());
    }

    /**
     * Append the character to the tail of the buffer.
     *
     * @param c the character
     * @return the number of bytes written (always one)
     */
    size_t append(char c);

    /**
     * Formatting append function.
     *
     * @param fmt the format string
     * @return the number of bytes written
     */
    size_t appendf(const char* fmt, ...) PRINTFLIKE(2, 3);

    /**
     * Formatting append function.
     *
     * @param fmt the format string
     * @param ap the format argument list
     * @return the number of bytes written
     */
    size_t vappendf(const char* fmt, va_list ap);

    /**
     * Trim cnt characters from the tail of the string.
     */
    void trim(size_t cnt)
    {
        ASSERT(buf_->len() >= cnt);
        buf_->set_len(buf_->len() - cnt);
    }

    /**
     * Forcibly set the buffer length to len.
     */
    void set_length(size_t len)
    {
        ASSERT(buf_->buf_len() >= len);
        buf_->set_len(len);
    }

private:
    mutable ExpandableBuffer* buf_;
    bool    own_buf_;
};

/**
 * Initially stack allocated StringBuffer, which handles the common
 * cases where the StringBuffer is used to sprintf a bunch of stuff
 * together.
 *
 * Basically this is just syntactic sugar for passing the correct type
 * of expandable buffer to the StringBuffer class.
 */
template<size_t _sz>
class StaticStringBuffer : public StringBuffer {
public:
    /**
     * Default constructor
     *
     * @param init_str Initial string value. If the string is longer
     * than _sz, then the string is truncated.
     */
    StaticStringBuffer(char* init_str = 0)
        : StringBuffer(new ScratchBuffer<char*, _sz>(), init_str)
    {}

};

} // namespace oasys

#endif /* _OASYS_STRING_BUFFER_H_ */
