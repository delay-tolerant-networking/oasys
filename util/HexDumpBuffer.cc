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

#include <ctype.h>
#include "HexDumpBuffer.h"

namespace oasys {

void
HexDumpBuffer::hexify()
{
    // make a copy of the current data
    size_t len = length();
    std::string contents(data(), len);
    char strbuf[16];

    // rewind the string buffer backwards
    trim(length());

    // generate the dump
    u_char* bp = (u_char*)contents.data();
    for (size_t i = 0; i < len; ++i, ++bp)
    {
        // print the offset on each new line
        if (i % 16 == 0) {
            appendf("%07x ", i);
        }
        
        // otherwise print a space every two bytes (except the first)
        else if (i % 2 == 0) {
            append(" ");
        }

        // print the hex character
        appendf("%02x", *bp);
        
        // tack on the ascii character if it's printable, '.' otherwise
        if (isalnum(*bp) || ispunct(*bp) || (*bp == ' ')) {
            strbuf[i % 16] = *bp;
        } else {
            strbuf[i % 16] = '.';
        }
        
        if (i % 16 == 15)
        {
            appendf(" |  %.*s\n", 16, strbuf);
        }
    }
}

} // namespace oasys
