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

#include "../debug/Log.h" 	// for PRINTFLIKE macro

namespace oasys {

class IOClient;

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
    /**
     * Constructor
     *
     * @param initsz the initial buffer size
     * @param initstr the initial buffer contents 
     */
    StringBuffer(size_t initsz = 256, const char* initstr = 0);

    /**
     * Constructor
     *
     * @param fmt the initial buffer contents
     */
    StringBuffer(const char* fmt, ...) PRINTFLIKE(2, 3);

    /**
     * Destructor. Frees the buffer.
     */
    ~StringBuffer();

    /**
     * Reserve buffer space.
     *
     * @param sz 	the minimum required free buffer space
     * @param grow 	size to alloc if needed (default is 2x buflen)
     */
    void reserve(size_t sz, size_t grow = 0);
    
    /**
     * @return the data buffer (const variant).
     */
    const char* data() const { return buf_; }

    /**
     * @return the data buffer (non-const variant)
     */
    char* data() { return buf_; }

    /**
     * @return length of the buffer.
     */
    size_t length() { return len_; }

    /**
     * @return the data buffer, ensuring null termination.
     */
    char* c_str()
    {
        reserve(len_ + 1);
        buf_[len_] = '\0';
        return buf_;
    }

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
     * Fill the buffer by reading len bytes from the given IOClient.
     */
    void append(IOClient* io, size_t len);
    
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
        ASSERT(len_ >= cnt);
        len_ -= cnt;
    }

    /**
     * Forcibly set the buffer length to len.
     */
    void set_length(size_t len)
    {
        ASSERT(buflen_ >= len);
        len_ = len;
    }

protected:
    char*  buf_;
    size_t len_;
    size_t buflen_;
};

/**
 * Static, stack allocated StringBuffer, which handles the common
 * cases where the StringBuffer is used to sprintf a bunch of stuff
 * together.
 */
template<unsigned int _sz>
class StaticStringBuffer {
public:
    /**
     * @init_str Initial string value. If the string is longer than
     * _sz, then the string is truncated.
     */
    StaticStringBuffer(char* init_str = 0) : len_(0) {
        if(init_str != 0) {
            buf_[_sz - 1] = '\0';
            strncpy(buf_, init_str, _sz);
            len_ = std::min(_sz - 1, strlen(init_str));
        }
    }
    
    /**
     * Append the character to the tail of the buffer.
     *
     * @param c the character
     * @return the number of bytes written (always one)
     */
    size_t append(char c) {
        if(len_ < _sz) {
            buf_[len_] = c;
            return 1;
        }
        return 0;
    }

    /**
     * Formatting append function.
     *
     * @param fmt the format string
     * @return the number of bytes written
     */
    size_t appendf(const char* fmt, ...) PRINTFLIKE(2, 3);
    size_t appendf(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        size_t ret = vappendf(fmt, ap);
        va_end(ap);
        return ret;
    }

    /**
     * Formatting append function, truncating if necessary.
     *
     * @param fmt the format string
     * @param ap the format argument list
     * @return the number of bytes written
     */
    size_t vappendf(const char* fmt, va_list ap) {
        size_t nfree = _sz - len_;
        int ret = vsnprintf(&buf_[len_], nfree, fmt, ap);

        return ret;
    }

private:
    char   buf_[_sz];
    size_t len_;
};

} // namespace oasys

#endif /* _OASYS_STRING_BUFFER_H_ */
