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
#include <cstdlib>

#include "util/UnitTest.h"
#include "util/StringBuffer.h"
#include "util/ExpandableBuffer.h"
#include "util/ScratchBuffer.h"
#include "util/Random.h"

using namespace oasys;

DECLARE_TEST(ExpandableBuffer1) {
    ExpandableBuffer buf;
    
    CHECK(buf.len()     == 0);
    CHECK(buf.buf_len() == 0);
    
    buf.reserve(10);

    const char* str = "1234567890";

    memcpy(buf.raw_buf(), str, 10);
    CHECK_EQUALSTRN(buf.raw_buf(), str, 10);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(ExpandableBuffer2) {
    ScratchBuffer<char*, 10> scratch;
    StringBuffer buf(&scratch, false);
    
    buf.appendf("%d%x%s%x%d%x", 1, 2, "abracadabra", 
                4, 5, 6);

    char str[256];
    sprintf(str, "%d%x%s%x%d%x", 1, 2, "abracadabra", 
            4, 5, 6);
    CHECK_EQUALSTR(buf.c_str(), str);
    
    return UNIT_TEST_PASSED;
}

void memory1Func() {
    ScratchBuffer<char*, 4096> scratch;
    scratch.buf();
}

DECLARE_TEST(Memory1) {
    for (int i=0; i<20000; ++i) {
        memory1Func();
    }

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {    
    ADD_TEST(ExpandableBuffer1);
    ADD_TEST(ExpandableBuffer2);
    ADD_TEST(Memory1);
}

DECLARE_TEST_FILE(Test, "static buffer test");
