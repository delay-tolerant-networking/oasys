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

#include <iostream>
#include <debug/DebugUtils.h>

#include "util/UnitTest.h"
#include "serialize/MarshalSerialize.h"
#include "serialize/TextSerialize.h"
#include "serialize/TypeShims.h"
#include "util/ScratchBuffer.h"


using namespace std;
using namespace oasys;

#define CRC Serialize::USE_CRC

class OneOfEach : public oasys::SerializableObject {
public:
    OneOfEach() : 
	a(200), 
	b(-100), 
	c(0x77), 
	d(0xbaddf00d), 
	e(56789), 
	u(INT_MAX), 
	s1("hello") 
    {
        memset(s2, 0, sizeof(s2));
        strcpy(s2, "Zanzibar");
    }
    OneOfEach(bool fool) : a(0), b(0), c(0), d(0), e(0), u(0), s1("") {
        memset(s2, 0, sizeof(s2));
    }
    
    virtual ~OneOfEach() {}
    
    void serialize(oasys::SerializeAction* act) {
        act->process("a", &a);
        act->process("b", &b);
        act->process("c", &c);
        act->process("d", &d);
        act->process("e", &e);
        act->process("u", &u);
        act->process("s1", &s1);
        act->process("s2", s2, sizeof(s2));
    }

    bool equals(const OneOfEach& other) {
        return (a == other.a &&
                b == other.b &&
                c == other.c &&
                d == other.d &&
                e == other.e &&
                u == other.u &&
                s1.compare(other.s1) == 0 &&
                !memcmp(s2, other.s2, sizeof(s2)));
    }

private:
    int32_t    a, b, c, d;
    short  e;
    u_int32_t u;
    string s1;
    char   s2[32];
};

int CompareTest(bool crc) {
    OneOfEach o1, o2(false);

#define LEN 256
    u_char buf[LEN];
    memset(buf, 0, sizeof(char) * LEN);

    oasys::MarshalSize sizer(Serialize::CONTEXT_NETWORK, crc);
    sizer.logpath("/marshal-test");
    sizer.action(&o1);

    oasys::Marshal v(Serialize::CONTEXT_NETWORK, buf, LEN, crc);
    v.logpath("/marshal-test");
    v.action(&o1);
    
    oasys::Unmarshal uv(Serialize::CONTEXT_NETWORK, buf, sizer.size(), crc);
    uv.logpath("/marshal-test");
    uv.action(&o2);

    ASSERT(o1.equals(o2));

    return 0;
}

DECLARE_TEST(CompareTest_NOCRC) {
    return CompareTest(false);
}

DECLARE_TEST(CompareTest_CRC) {
    return CompareTest(true);
}

DECLARE_TEST(SizeTest) {
    OneOfEach o;
    size_t sz = 4 + 4 + 4 + 4 +
	        2 + 
	        4 +
		4 + 5 +
	        32;

    oasys::MarshalSize sizer1(Serialize::CONTEXT_NETWORK, 0);
    sizer1.action(&o);
    CHECK_EQUAL(sizer1.size(), sz);
    
    oasys::MarshalSize sizer2(Serialize::CONTEXT_NETWORK, Serialize::USE_CRC);
    sizer2.action(&o);
    CHECK_EQUAL(sizer2.size(), sz + 4);

    return 0;
}

DECLARE_TEST(NullStringTest1) {
    char* test = "test string END";

    NullStringShim id(0);
    oasys::Unmarshal uv(Serialize::CONTEXT_LOCAL, 
	                reinterpret_cast<u_char*>(test), 
	                strlen(test) + 1, 0);
    uv.action(&id);
    CHECK(strcmp(test, id.value()) == 0);

    return 0;
}

DECLARE_TEST(NullStringTest2) {
    char* test = "test string END";

    // XXX/demmer why doesn't the temporary work??
    Builder b;
    NullStringShim id(b);
    oasys::Unmarshal uv(Serialize::CONTEXT_LOCAL, 
	                reinterpret_cast<u_char*>(test), 
	                strlen(test) + 1, 0);
    uv.action(&id);
    CHECK(strcmp(test, id.value()) == 0);

    return 0;
}

DECLARE_TEST(TextSerializeTest1) {
    OneOfEach one;
    ScratchBuffer<u_char*> buf;
    
    TextMarshal marshal(Serialize::CONTEXT_LOCAL, &buf);
    marshal.action(&one);

    log_info("%s", (const char*)buf.buf());
        
    return 0;
}

struct TestObject : oasys::SerializableObject {
    TestObject() : next_exists_(false), next_(0) {}

    void serialize(oasys::SerializeAction* action) {
        action->process("int32", &int32_);
        action->process("int16", &int16_);
        action->process("int8", &int8_);
        
        action->process("next_exists", &next_exists_);
        if (next_exists_) {
            action->process("next", next_);
        }
    }

    u_int32_t int32_;
    u_int32_t int16_;
    u_int32_t int8_;
    bool      next_exists_;

    TestObject* next_;
};

DECLARE_TEST(TextSerializeTest2) {
    ScratchBuffer<u_char*> buf;
    TestObject obj1;
    
    obj1.int32_ = 1111;
    obj1.int16_ = 123;
    obj1.int8_  = 5;
    
    TextMarshal marshal(Serialize::CONTEXT_LOCAL, &buf);
    marshal.action(&obj1);

    log_info("/test", "%s", (const char*)buf.buf());

    const char* txt = "# -- text marshal start --\n"
                      "# comment\n"
                      "# another comment\n"
                      "int32: 1111\n"
                      "\t\tint16: 123\n"
                      "int8: 5\n"
                      "next_exists: false\n";

    TestObject obj2;
    TextUnmarshal unmarshal(Serialize::CONTEXT_LOCAL, (u_char*)txt, strlen(txt));

    unmarshal.action(&obj2);
               
    CHECK(obj2.int32_ == 1111);
    CHECK(obj2.int16_ == 123);
    CHECK(obj2.int8_  == 5);

    return 0;
}

DECLARE_TEST(TextSerializeTest3) {

    return 0;
}


DECLARE_TESTER(MarshalTester) {
    ADD_TEST(SizeTest);
    ADD_TEST(CompareTest_NOCRC);
    ADD_TEST(CompareTest_CRC);
    ADD_TEST(NullStringTest1);
    ADD_TEST(NullStringTest2);
    ADD_TEST(TextSerializeTest1);
    ADD_TEST(TextSerializeTest2);
    ADD_TEST(TextSerializeTest3);
};

DECLARE_TEST_FILE(MarshalTester, "marshalling test");
