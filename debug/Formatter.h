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

#ifndef _FORMATTER_H_
#define _FORMATTER_H_

#include <stdio.h>
#include <stdarg.h>

namespace oasys {

/**
 * This class is intended to be used with a modified implementation of
 * snprintf/vsnprintf, defined in Formatter.cc.
 *
 * The modification implements a special control code combination of
 * '*%p' that triggers a call to Formatter::format(), called on the
 * object pointer passed in the vararg list.
 *
 * For example:
 *
 * @code
 * class FormatterTest : public Formatter {
 * public:
 *     virtual int format(char* buf, size_t sz) {
 *         return snprintf(buf, sz, "FormatterTest");
 *     }
 * };
 *
 * FormatterTest f;
 * char buf[256];
 * snprintf(buf, sizeof(buf), "pointer %p, format *%p\n");
 * // buf contains "pointer 0xabcd1234, format FormatterTest\n"
 * @endcode
 */
class Formatter {
public:
    /**
     * Virtual callback, called from this vsnprintf implementation
     * whenever it encounters a format string of the form "*%p".
     *
     * The output routine must not write more than sz bytes. The
     * return is the number of bytes written, or the number of bytes
     * that would have been written if the output is truncated.
     */
    virtual int format(char* buf, size_t sz) = 0;

    /**
     * Assertion check to make sure that obj is a valid formatter.
     * This basically just makes sure that in any multiple inheritance
     * situation where each base class has virtual functions,
     * Formatter _must_ be the first class in the inheritance chain.
     */
    static inline void assert_valid(Formatter* obj);

#ifndef NDEBUG
#define FORMAT_MAGIC 0xffeeeedd
    Formatter() : format_magic_(FORMAT_MAGIC) {}
    unsigned int format_magic_;
#endif // NDEBUG
    
};

void __log_assert(bool x, const char* what, const char* file, int line);

inline void
Formatter::assert_valid(Formatter* obj)
{
#ifndef NDEBUG
    __log_assert(obj->format_magic_ == FORMAT_MAGIC,
                 "Formatter object invalid -- maybe need a cast to Formatter*",
                 __FILE__, __LINE__);
#endif
}

} // namespace oasys

#endif /* _FORMATTER_H_ */
