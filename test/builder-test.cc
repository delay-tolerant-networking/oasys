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
#include "serialize/Builder.h"
#include "serialize/TypeShims.h"

using namespace oasys;

struct TestC {};

Builder<TestC>* Builder<TestC>::instance_;

struct Obj : public SerializableObject {};

struct Foo : public Obj {
    Foo(Builder<TestC>* b) {}
    void serialize(SerializeAction* a) {
        a->process("int", &i_);
    }

    int i_;
};

struct Bar : public Obj {
    Bar(Builder<TestC>* b) {}
    void serialize(SerializeAction* a) {
        a->process("int", &i_);
    }

    int i_;
};

BUILDER_TYPECODE(TestC, Foo, 1);
BUILDER_TYPECODE(TestC, Bar, 2);
BUILDER_TYPECODE_AGGREGATE(TestC, Obj, 1, 2);

BUILDER_CLASS(TestC, Foo, 1);
BUILDER_CLASS(TestC, Bar, 2);

DECLARE_TEST(Builder1) {
    u_char buf[4];
    Marshal m(Serialize::CONTEXT_LOCAL, buf, 4);
    IntShim i(99);
    m.action(&i);

    Foo* foo;
    CHECK(Builder<TestC>::instance()->
          new_object(BuilderCode<TestC, Foo>::TYPECODE, 
                     &foo, buf, 4, Serialize::CONTEXT_LOCAL) == 0);
    
    CHECK(foo->i_ == 99);

    return 0;
}


DECLARE_TEST(Builder2) {
    u_char buf[4];
    Marshal m(Serialize::CONTEXT_LOCAL, buf, 4);
    IntShim i(55);
    m.action(&i);

    Bar* bar;
    CHECK(Builder<TestC>::instance()->
          new_object(BuilderCode<TestC, Bar>::TYPECODE, 
                     &bar, buf, 4, Serialize::CONTEXT_LOCAL) == 0);
    
    CHECK(bar->i_ == 55);

    return 0;
}

DECLARE_TEST(BuilderTypeCode) {
    u_char buf[4];
    Marshal m(Serialize::CONTEXT_LOCAL, buf, 4);
    IntShim i(55);
    m.action(&i);

    Bar* bar;
    CHECK(Builder<TestC>::instance()->
          new_object(BuilderCode<TestC, Foo>::TYPECODE, 
                     &bar, buf, 4, Serialize::CONTEXT_LOCAL) == BuilderErr::TYPECODE);
    
    return 0;
}

DECLARE_TEST(BuilderCorrupt) {
    u_char buf[4];
    Marshal m(Serialize::CONTEXT_LOCAL, buf, 4);
    IntShim i(55);
    m.action(&i);

    Bar* bar;
    CHECK(Builder<TestC>::instance()->
          new_object(BuilderCode<TestC, Bar>::TYPECODE, 
                     &bar, buf, 2, Serialize::CONTEXT_LOCAL) == BuilderErr::CORRUPT);
    
    return 0;
}

DECLARE_TEST(BuilderAgg) {
    u_char buf[4];
    Marshal m(Serialize::CONTEXT_LOCAL, buf, 4);
    IntShim i(99);
    m.action(&i);

    Obj* obj = 0;
    CHECK(Builder<TestC>::instance()->
          new_object(BuilderCode<TestC, Foo>::TYPECODE, 
                     &obj, buf, 4, Serialize::CONTEXT_LOCAL) == 0);
    
    CHECK(obj != 0);
    CHECK(static_cast<Foo*>(obj)->i_ == 99);

    return 0;
}

DECLARE_TEST(BuilderNames) {
    CHECK_EQUALSTR(Builder<TestC>::instance()->type_name(1),
	           "TestC::Foo");
    CHECK_EQUALSTR(Builder<TestC>::instance()->type_name(2),
	           "TestC::Bar");
    
    return 0;
}

DECLARE_TESTER(BuilderTest) {
    ADD_TEST(Builder1);
    ADD_TEST(Builder2);
    ADD_TEST(BuilderTypeCode);
    ADD_TEST(BuilderCorrupt);
    ADD_TEST(BuilderAgg);
    ADD_TEST(BuilderNames);
}

DECLARE_TEST_FILE(BuilderTest, "builder test");
