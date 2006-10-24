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
        char c = Random::rand(26) + 'a';
        buf.append(c);
        scratch[i] = c;
    }

    // this should cause a realloc
    CHECK_EQUALSTR(buf.c_str(), scratch);
    CHECK_EQUAL(strlen(buf.c_str()), 256);    

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
