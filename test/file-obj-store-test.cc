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

#include <stdio.h>

#include "util/UnitTest.h"
#include "storage/FileBackedObject.h"
#include "storage/FileBackedObjectStore.h"

using namespace oasys;

FileBackedObjectStore* g_store = 0;

DECLARE_TEST(Init) {
    system("rm -rf testdir");
    system("mkdir testdir");
    g_store = new FileBackedObjectStore("testdir");

    CHECK(g_store != 0);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(StoreTest) {
    CHECK(! g_store->object_exists("test1"));
    CHECK(g_store->new_object("test1") == 0);
    CHECK(g_store->new_object("test1") != 0);
    CHECK(g_store->object_exists("test1"));
    CHECK(g_store->copy_object("test1", "test2") == 0);
    CHECK(g_store->copy_object("test1", "test2") != 0);
    CHECK(g_store->object_exists("test2"));
    CHECK(g_store->del_object("test1") == 0);
    CHECK(g_store->del_object("test1") != 0);
    CHECK(! g_store->object_exists("test1"));
    CHECK(g_store->del_object("test2") == 0);
    CHECK(g_store->del_object("test2") != 0);
    CHECK(! g_store->object_exists("test2"));

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(FileTest) {
    

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Cleanup) {
    delete g_store;
    system("rm -rf testdir");

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(Init);
    ADD_TEST(StoreTest);
    ADD_TEST(FileTest);
    ADD_TEST(Cleanup);
}

DECLARE_TEST_FILE(Test, "sample test");
