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
    ExpandableBuffer buf;
    
    CHECK(buf.len()     == 0);
    CHECK(buf.buf_len() == 0);

    buf.reserve(10);
    const char* str = "1234567890";
    memcpy(buf.raw_buf(), str, 10);
    
    buf.reserve(1024);
    CHECK_EQUALSTRN(buf.raw_buf(), str, 10);

    // this is for valgrind to check
    memset(buf.raw_buf(), 17, 1000);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(ScratchBuffer1) {
    const char* str = "0987654321";
    ScratchBuffer<char*, 20> scratch;
    
    memcpy(scratch.buf(), str, 10);
    CHECK_EQUALSTRN(scratch.buf(), str, 10);

    scratch.buf(1000);
    memcpy(scratch.buf()+500, str, 10);

    CHECK_EQUALSTRN(scratch.buf(), str, 10);
    CHECK_EQUALSTRN(scratch.buf()+500, str, 10);    

    scratch.buf(10000);
    memcpy(scratch.buf()+5000, str, 10);

    CHECK_EQUALSTRN(scratch.buf(), str, 10);
    CHECK_EQUALSTRN(scratch.buf()+500, str, 10);
    CHECK_EQUALSTRN(scratch.buf()+5000, str, 10);    

    scratch.buf(100000);
    memcpy(scratch.buf()+50000, str, 10);

    CHECK_EQUALSTRN(scratch.buf(), str, 10);
    CHECK_EQUALSTRN(scratch.buf()+500, str, 10);
    CHECK_EQUALSTRN(scratch.buf()+5000, str, 10);    
    CHECK_EQUALSTRN(scratch.buf()+50000, str, 10);    
    
    // valgrind
    memset(scratch.buf(), 17, 100000);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(StringBuffer1) {
    StringBuffer buf(256);
    char scratch[1024];

    memset(scratch, 0, 1024);
    
    // append up to the preallocated amount
    for (int i=0; i<256; ++i) {
        char c = Random::rand(26 + 'a');
        buf.append(c);
        scratch[i] = c;
    }

    // this should cause a realloc
    CHECK_EQUALSTR(buf.c_str(), scratch);
    CHECK(strlen(buf.c_str()) == 256);    

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(StringBuffer2) {
    StringBuffer buf(10);
    char scratch[4096];
    char* ptr = scratch;
    
    memset(scratch, 0, 4096);

    for (int i=0; i<40; ++i) {
        buf.appendf("You are in a twisty maze of passages, all alike.");
        ptr += sprintf(ptr, "You are in a twisty maze of passages, all alike.");
    }
    
    CHECK_EQUALSTR(buf.c_str(), scratch);

    return UNIT_TEST_PASSED;
}


DECLARE_TEST(StringBuffer3) {
    StringBuffer buf(10);
    char scratch[1024];
    char* ptr = scratch;
    
    memset(scratch, 0, 1024);

    for (int i=0; i<20; ++i) {
        buf.append("xyzzy");
        buf.append('c');
        ptr += sprintf(ptr, "xyzzy");
        *ptr++ = 'c';
    }
    
    CHECK_EQUALSTR(buf.c_str(), scratch);

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {    
    ADD_TEST(ExpandableBuffer1);
    ADD_TEST(ExpandableBuffer2);
    ADD_TEST(ScratchBuffer1);
    ADD_TEST(StringBuffer1);
    ADD_TEST(StringBuffer2);
    ADD_TEST(StringBuffer3);
}

DECLARE_TEST_FILE(Test, "buffer test");
