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

#include "SDNV.h"
#include "debug/DebugUtils.h"

namespace oasys {

int
SDNV::encode(u_int64_t val, u_char* bp, size_t len)
{
    u_char* discriminant = bp;
    ++bp;

    /*
     * Deal with values that fit in one byte.
     */
    if (val <= 127) {
        if (len < 1)
            return -1;
        
        *discriminant = (u_char)val;
        return 1;
    }

    /*
     * Figure out how many bytes we need for the encoding.
     */
    size_t val_len = 0;
    u_int64_t tmp = val;

    do {
        tmp = tmp >> 8;
        val_len++;
    } while (tmp > 0xf);

    ASSERT(val_len > 0);
    ASSERT(val_len <= 8);

    /*
     * Make sure we have enough buffer space.
     */
    if (len < val_len) {
        return -1;
    }

    /*
     * Now advance bp to the last byte and fill it in backwards with
     * the value bytes.
     */
    bp += val_len - 1;
    do {
        *bp = (u_char)(val & 0xff);
        val = val >> 8;
        --bp;
    } while (val > 0xf);

    ASSERT(bp == discriminant);

    /*
     * Finally, construct the discriminant and we're done.
     */
    *discriminant = (1 << 7) | ((val_len - 1) << 4) | ((u_char)val & 0xf);
    
    return val_len + 1;
}

int
SDNV::decode(const u_char* bp, size_t len, u_int64_t* val)
{
    if (len == 0)
        return -1;

    u_char discriminant = *bp;
    ++bp;
    --len;

    /*
     * If the high-order bit is zero, then the remaining 7 bits encode
     * the value and we're done.
     */
    if ((discriminant & (1 << 7)) == 0) {
        *val = discriminant & 0x7f;
        return 1;
    }
    
    /*
     * Otherwise, bits 2-4 encode the total number of bytes used in
     * the value, and the lower four bits encode the most significant
     * bits of the value itself.
     */
    size_t val_len = ((discriminant & 0x70) >> 4) + 1;
    ASSERT(val_len > 0);
    ASSERT(val_len <= 8);
    
    /*
     * First make sure the total length is long enough.
     */
    if (len < val_len) {
        return -1;
    }

    /*
     * Grab the high-order nibble.
     */
    *val = discriminant & 0x0f;

    /*
     * Since this implementation only handles 64-bit values but the
     * spec allows for 68-bits, we need to check for overflow.
     */
    if ((val_len == 8) && (*val != 0)) {
        log_err("/sdnv", "overflow value in sdnv!!!");
        return -1;
    }

    /*
     * Now we loop through the rest of the value bytes, shifting up
     * the existing value and adding in one byte at a time.
     */
    size_t todo = val_len;
    do {
        *val = (*val << 8) | *bp;

        ++bp;
        --todo;
    } while (todo != 0);

    return val_len + 1;
}


} // namespace oasys
