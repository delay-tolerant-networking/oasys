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
#include "util/StringUtils.h"

using namespace oasys;
using namespace std;
using namespace _std;

struct BogusStringHash {
    size_t operator()(const std::string& str) const
    {
        return _std::__stl_hash_string(str.data());
    }
};

typedef StringHashSet TestHashSet;
typedef StringHashMap<int> TestHashMap;
typedef hash_map<string, int, BogusStringHash, StringEquals> BadHashMap;

DECLARE_TEST(HashSetSmokeTest) {
    TestHashSet s;

    CHECK(s.insert("the").second == true);
    CHECK(s.insert("quick").second == true);
    CHECK(s.insert("brown").second == true);
    CHECK(s.insert("fox").second == true);

    CHECK(s.find("quick") != s.end());
    CHECK(*(s.find("quick")) == "quick");
    CHECK(s.find("the") != s.end());
    CHECK(s.find("bogus") == s.end());

    CHECK(s.insert("the").second == false);
    CHECK(s.insert("xxx").second == true);
    CHECK(s.erase("xxx") == 1);
    CHECK(s.erase("xxx") == 0);
    CHECK(s.find("xxx") == s.end());

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(HashMapSmokeTest) {
    TestHashMap m;

    m["the"] = 1;
    m["quick"] = 2;
    m["brown"] = 3;
    m["fox"] = 4;

    CHECK(m.find("quick") != m.end());
    CHECK(m["quick"] == 2);
    CHECK(m.find("quick")->first == "quick");
    CHECK(m.find("quick")->second == 2);
    
    CHECK(m["quick"] == 2);
    m["quick"] = 5;
    CHECK(m["quick"] == 5);
    CHECK(m.find("quick")->first == "quick");
    CHECK(m.find("quick")->second == 5);
    
    CHECK(m.find("bogus") == m.end());

    CHECK(m.find("the") != m.end());
    CHECK(m["the"] == 1);

    CHECK(m.insert(TestHashMap::value_type("the", 100)).second == false);
    CHECK(m["the"] == 1);

    CHECK(m.insert(TestHashMap::value_type("xxx", 100)).second == true);
    CHECK(m["xxx"] == 100);

    CHECK(m.erase("xxx") == 1);
    CHECK(m.erase("xxx") == 0);

    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(NullTerminationTest) {
    TestHashMap m;

    char buf[256];
    strcpy(buf, "abcdef");
    std::string str(buf);
    size_t end = str.find('e');
    std::string key(str, 0, end);

    m["abcd"] = 1;
    key[4] = 'x';
    CHECK(m.find(key) != m.end());
    return UNIT_TEST_PASSED;
}

// This test case uses the BadHashMap which has an old bug in the
// implementation of StringHash that uses string::data() instead of
// string::c_str()
DECLARE_TEST(NullTerminationTestBug) {
    BadHashMap bm;

    char buf[256];
    strcpy(buf, "abcdef");
    std::string str(buf);
    size_t end = str.find('e');
    std::string key(str, 0, end);
    
    // this elucidates the bug by scribbling on the null character and
    // some others that follow the actual string data
    bm["abcd"] = 1;
    for (int i = 4; i < 10; ++i) {
        ((char*)key.data())[i] = 'x';
    }
    CHECK(bm.find("abcd") != bm.end());
    CHECK(bm.find(key) == bm.end());
    CHECK(bm.find(key.c_str()) != bm.end());

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {    
    ADD_TEST(HashSetSmokeTest);
    ADD_TEST(HashMapSmokeTest);
    ADD_TEST(NullTerminationTest);
    ADD_TEST(NullTerminationTestBug);
}

DECLARE_TEST_FILE(Test, "string hash set/map test");
