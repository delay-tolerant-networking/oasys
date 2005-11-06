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

#include "util/Regex.h"
#include "util/UnitTest.h"

using namespace oasys;

DECLARE_TEST(Regex1) {
    CHECK(Regex::match("ab*c", "abbbbbbbc") == 0);
    CHECK(Regex::match("ab*c", "ac") == 0);
    CHECK(Regex::match("ab*c", "abbbbbbb1333346c") == REG_NOMATCH);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Regex2) {
    Regex re("ab(:r+)bd", REG_EXTENDED);

    CHECK(re.match("ab:rrrbd") == 0);
    CHECK_EQUAL(re.get_match(1).rm_so, 2);
    CHECK_EQUAL(re.get_match(1).rm_eo, 6);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Regsub) {
    std::string s;
    CHECK(Regsub::subst("ab*c", "abbbbbbbc", "nothing", &s, REG_EXTENDED) == 0);
    CHECK_EQUALSTR(s.c_str(), "nothing");

    CHECK(Regsub::subst("ab*c", "abbbbbbbc", "\\0", &s, REG_EXTENDED) == 0);
    CHECK_EQUALSTR(s.c_str(), "abbbbbbbc");

    CHECK(Regsub::subst("a(b*)c", "abbbbbbbc", "\\1", &s, REG_EXTENDED) == 0);
    CHECK_EQUALSTR(s.c_str(), "bbbbbbb");

    CHECK(Regsub::subst("(a)(b*)(c)", "abbbbbbbc", "\\0 \\1 \\2 \\3", &s, REG_EXTENDED) == 0);
    CHECK_EQUALSTR(s.c_str(), "abbbbbbbc a bbbbbbb c");

    CHECK(Regsub::subst("ab*c", "xyz", "", &s, REG_EXTENDED) == REG_NOMATCH);

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(Regex1);
    ADD_TEST(Regex2);
    ADD_TEST(Regsub);
}

DECLARE_TEST_FILE(Test, "regex test");
