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
    CHECK(g_store->new_object("test") == 0);

    FileBackedObjectHandle h;
    h = g_store->get_handle("test", 0);
    
    const char* teststr = "hello world";

    CHECK(h->write_bytes(0, reinterpret_cast<const u_char*>(teststr), 
                         strlen(teststr)) == strlen(teststr));

    char buf[256];
    memset(buf, 0, 256);
    CHECK(h->read_bytes(0, reinterpret_cast<u_char*>(buf), 
                        strlen(teststr)) == strlen(teststr));
    CHECK_EQUALSTR(buf, teststr);
    CHECK(h->size() == strlen(teststr));
    h.reset();

    FILE* f = fopen("testdir/test", "r");
    fgets(buf, 256, f);
    CHECK_EQUALSTR(buf, teststr);

    fclose(f);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(TXTest) {
    const char* teststr = "hello world";
    const char* teststr2 = "goodbye world";

    FileBackedObjectHandle h;
    h = g_store->get_handle("test", 0);

    FileBackedObject::TxHandle tx = h->start_tx(0);
    tx->object()->write_bytes(0, reinterpret_cast<const u_char*>(teststr2), strlen(teststr2));

    char buf[256];
    memset(buf, 0, 256);
    CHECK(h->read_bytes(0, reinterpret_cast<u_char*>(buf), strlen(teststr)) == strlen(teststr));
    CHECK_EQUALSTR(buf, teststr);    
    CHECK(h->size() == strlen(teststr));

    // commit the transaction
    tx.reset();

    CHECK(h->read_bytes(0, reinterpret_cast<u_char*>(buf), strlen(teststr2)) == strlen(teststr2));
    CHECK_EQUALSTR(buf, teststr2);    
    CHECK(h->size() == strlen(teststr2));    
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Cleanup) {
    delete g_store;
//    system("rm -rf testdir");

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(Init);
    ADD_TEST(StoreTest);
    ADD_TEST(FileTest);
    ADD_TEST(TXTest);
    ADD_TEST(Cleanup);
}

DECLARE_TEST_FILE(Test, "sample test");
