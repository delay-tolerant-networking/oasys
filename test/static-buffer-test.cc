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
#include "util/UnitTest.h"
#include "util/StringBuffer.h"
#include "util/ScratchBuffer.h"

using namespace oasys;

DECLARE_TEST(StringTest1) {
    StaticStringBuffer<10> buf;
    
    buf.append('X');

    return (strncmp("X", buf.c_str(), 10) == 0) ? 0 : UNIT_TEST_FAILED;
}

DECLARE_TEST(StringTest2) {
    StaticStringBuffer<10> buf;
    char buf2[256];
    
    buf.appendf("%d%d%d%s", 10, 20, 123, "foobarbaz");
    sprintf(buf2, "%d%d%d%s", 10, 20, 123, "foobarbaz");

    log_debug("\"%s\" \"%s\"", buf.c_str(), buf2);

    return (strncmp(buf2, buf.c_str(), 10) == 0) ? 0 : UNIT_TEST_FAILED;
}

DECLARE_TEST(ScratchTest1) {
    ScratchBuffer<char*> buf;
    char* buf2 = "1234567890";

    memcpy(buf.buf(11), buf2, 11);

    return (strncmp(buf2, buf.buf(), 10) == 0) ? 0 : UNIT_TEST_FAILED;
}

DECLARE_TEST(ScratchTest2) {
    ScratchBuffer<char*, 11> buf;
    char* buf2 = "1234567890";

    memcpy(buf.buf(11), buf2, 11);

    return (strncmp(buf2, buf.buf(), 10) == 0) ? 0 : UNIT_TEST_FAILED;
}

DECLARE_TESTER(Test) {    
    ADD_TEST(StringTest1);
    ADD_TEST(StringTest2);
    ADD_TEST(ScratchTest1);
    ADD_TEST(ScratchTest2);
}

DECLARE_TEST_FILE(Test, "static buffer test");
