/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
            appendf("%07x ", (u_int)i);
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

    // deal with the ending partial line
    for (size_t i = len % 16; i < 16; ++i) {
        if (i % 2 == 0) {
            append(" ");
        }

        append("  ");
    }

    appendf(" |  %.*s\n", (int)len % 16, strbuf);
}

} // namespace oasys
