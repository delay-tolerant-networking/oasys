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

#ifndef _OASYS_HEX_DUMP_BUFFER_H_
#define _OASYS_HEX_DUMP_BUFFER_H_

#include "StringBuffer.h"

namespace oasys {

/**
 * Class to produce pretty printing output from data that may be
 * binary (ala emacs' hexl-mode). Each line includes the offset, plus
 * 16 characters from the file as well as their ascii values (or . for
 * unprintable characters).
 *
 * For example:
 *
 * 00000000: 5468 6973 2069 7320 6865 786c 2d6d 6f64  This is hexl-mod
 * 00000010: 652e 2020 4561 6368 206c 696e 6520 7265  e.  Each line re
 * 00000020: 7072 6573 656e 7473 2031 3620 6279 7465  presents 16 byte
 * 00000030: 7320 6173 2068 6578 6164 6563 696d 616c  s as hexadecimal
 * 00000040: 2041 5343 4949 0a61 6e64 2070 7269 6e74   ASCII.and print
 */
class HexDumpBuffer : public StringBuffer {
public:
    /**
     * Constructor
     *
     * @param initsz the initial buffer size
     * @param initstr the initial buffer contents 
     */
    HexDumpBuffer(size_t initsz = 256, const char* initstr = 0)
        : StringBuffer(initsz, initstr) {}

    /**
     * Convert the internal buffer (accumulated into the StringBuffer)
     * into hex dump output format.
     */
    void hexify();
};

} // namespace oasys

#endif /* _OASYS_HEX_DUMP_BUFFER_H_ */
