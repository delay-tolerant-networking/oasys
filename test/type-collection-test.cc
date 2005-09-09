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
#include "serialize/TypeCollection.h"
#include "serialize/TypeShims.h"

using namespace oasys;

struct TestC {};

TYPE_COLLECTION_INSTANTIATE(TestC);

class Obj  {
public:
    Obj(int id) : id_(id) {}
    virtual const char* name() = 0;

    int id_;
};

class Foo : public Obj {
public:
    static const int ID = 1;
    Foo(const Builder& b) : Obj(ID) {}
    virtual const char* name() { return "foo"; }

private:
    Foo();
    Foo(const Foo&);
};

class Bar : public Obj {
public:
    static const int ID = 2;
    Bar(const Builder& b) : Obj(ID) {}
    virtual const char* name() { return "bar"; }

private:
    Bar();
    Bar(const Foo&);
};

class Baz : public Obj {
public:
    static const int ID = 3;
    Baz(const Builder& b) : Obj(ID) {}
    virtual const char* name() { return "baz"; }

private:
    Baz();
    Baz(const Baz&);
};

TYPE_COLLECTION_DECLARE(TestC, Foo, 1);
TYPE_COLLECTION_DECLARE(TestC, Bar, 2);
TYPE_COLLECTION_DECLARE(TestC, Baz, 3);
TYPE_COLLECTION_GROUP(TestC, Obj, 1, 2);

TYPE_COLLECTION_DEFINE(TestC, Foo, 1);
TYPE_COLLECTION_DEFINE(TestC, Bar, 2);

DECLARE_TEST(TypeCollection1) {
    Foo* foo;

    CHECK(TypeCollectionInstance<TestC>::instance()->
          new_object(TypeCollectionCode<TestC, Foo>::TYPECODE, &foo) == 0);
    CHECK_EQUAL(foo->id_, Foo::ID);
    CHECK_EQUALSTR(foo->name(), "foo");
    CHECK(dynamic_cast<Foo*>(foo) != NULL);
    
    return 0;
}


DECLARE_TEST(TypeCollection2) {
    Bar* bar;

    CHECK(TypeCollectionInstance<TestC>::instance()->
          new_object(TypeCollectionCode<TestC, Bar>::TYPECODE, &bar) == 0);
    CHECK(bar->id_ == Bar::ID);
    CHECK_EQUALSTR(bar->name(), "bar");
    CHECK(dynamic_cast<Bar*>(bar) != NULL);
    
    return 0;
}

DECLARE_TEST(TypeCollectionBogusCode) {
    Obj* obj = 0;
    CHECK(TypeCollectionInstance<TestC>::instance()->new_object(9999, &obj) == 
          TypeCollectionErr::TYPECODE);
    CHECK(obj == 0);
    
    return 0;
}

DECLARE_TEST(TypeCollectionMismatchCode) {
    Foo* foo = 0;
    CHECK(TypeCollectionInstance<TestC>::instance()->
          new_object(TypeCollectionCode<TestC, Bar>::TYPECODE, &foo) ==
          TypeCollectionErr::TYPECODE);
    CHECK(foo == 0);
    
    return 0;
}

DECLARE_TEST(TypeCollectionGroup) {
    Obj* obj = 0;
    CHECK(TypeCollectionInstance<TestC>::instance()->
          new_object(TypeCollectionCode<TestC, Foo>::TYPECODE, &obj) == 0);
    
    CHECK(obj != 0);
    CHECK(obj->id_ == Foo::ID);
    CHECK_EQUALSTR(obj->name(), "foo");
    CHECK(dynamic_cast<Foo*>(obj) != NULL);

    return 0;
}

DECLARE_TEST(TypeCollectionNotInGroup) {
    Obj* obj = 0;
    CHECK(TypeCollectionInstance<TestC>::instance()->
          new_object(TypeCollectionCode<TestC, Baz>::TYPECODE, &obj) ==
          TypeCollectionErr::TYPECODE);
    CHECK(obj == 0);

    return 0;
}

DECLARE_TEST(TypeCollectionNames) {
    CHECK_EQUALSTR(TypeCollectionInstance<TestC>::instance()->type_name(1),
	           "TestC::Foo");
    CHECK_EQUALSTR(TypeCollectionInstance<TestC>::instance()->type_name(2),
	           "TestC::Bar");
    
    return 0;
}

DECLARE_TESTER(TypeCollectionTest) {
    ADD_TEST(TypeCollection1);
    ADD_TEST(TypeCollection2);
    ADD_TEST(TypeCollectionBogusCode);
    ADD_TEST(TypeCollectionMismatchCode);
    ADD_TEST(TypeCollectionGroup);
    ADD_TEST(TypeCollectionNotInGroup);
    ADD_TEST(TypeCollectionNames);
}

DECLARE_TEST_FILE(TypeCollectionTest, "builder test");
