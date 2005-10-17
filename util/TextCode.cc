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

#include "StringBuffer.h"
#include "TextCode.h"

namespace oasys {

TextCode::TextCode(const char* input_buf, size_t length, 
                   ExpandableBuffer* buf, int cols, const char* pad)
    : input_buf_(input_buf), length_(length), 
      buf_(buf, false), cols_(cols), pad_(pad)
{
    textcodify();
}

bool
TextCode::is_not_escaped(int c) {
    return c >= 32 && c <= 126 && c != '\\';
}

void 
TextCode::append(int c) {
    if (is_not_escaped(c)) {
        buf_.append(static_cast<char>(c));
    } else if (c == '\\') {
        buf_.appendf("\\\\");
    } else {
        buf_.appendf("\\%02x", c);
    }
}

void
TextCode::textcodify()
{
    for (size_t i=0; i<length_; ++i) 
    {
        if (i != 0 && (i % cols_ == 0)) {
            buf_.appendf("\n%s", pad_);
        }
        append(input_buf_[i]);
    }
    buf_.appendf("\n%s\n", pad_);
}

} // namespace oasys
