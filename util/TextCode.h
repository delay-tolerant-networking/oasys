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

#ifndef __TEXTCODE_H__
#define __TEXTCODE_H__

#include "StringBuffer.h"

namespace oasys {

/*! 
 * Outputs a string that a certain column length with all non-printable
 * ascii characters rewritten in hex.
 * 
 * A TextCode block ends with a single raw control-L character
 * followed by a newline character ("\n") on a single line.
 */
class TextCode {
public:
    /*! 
     * @param input_buf Input buffer
     * @param length Length of the input buffer
     * @param buf  Buffer to put the text coded block into.
     * @param cols Number of characters to put in a column.
     * @param pad  String to put in front of each line.
     */
    TextCode(const char* input_buf, size_t length, 
             ExpandableBuffer* buf, int cols, int pad);
    
private:
    const char*  input_buf_;
    size_t       length_;        
    StringBuffer buf_;

    int cols_;
    int pad_;

    //! Whether or not the character is printable ascii
    bool is_not_escaped(char c);

    //! Perform the conversion
    void textcodify(); 

    //! Append a character to the text code
    void append(char c);
};

} // namespace oasys

#endif /* __TEXTCODE_H__ */
