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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>

#include "util/UnitTest.h"
#include "storage/FileBackedObject.h"
#include "storage/CheckedLog.h"

using namespace oasys;

const char* str[] = {
    "hello, world",
    "another record",
    "last record"
};

DECLARE_TEST(WriteTest) {
    system("rm -f check-log-test.fbo");
    system("touch check-log-test.fbo");
    
    FileBackedObject obj("check-log-test.fbo", 0);
    CheckedLogWriter wr(&obj);

    CheckedLogWriter::Handle h;

    wr.write_record(reinterpret_cast<const u_char*>(str[0]), 
                    strlen(str[0]));
    h = wr.write_record(reinterpret_cast<const u_char*>(str[1]), 
                        strlen(str[1]));
    wr.write_record(reinterpret_cast<const u_char*>(str[2]), 
                    strlen(str[2]));
    wr.ignore(h);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(ReadTest) {
    FileBackedObject obj("check-log-test.fbo", 0);
    CheckedLogReader rd(&obj);

    int ret;
    ExpandableBuffer buf;

    ret = rd.read_record(&buf);
    CHECK(ret == 0);
    CHECK(memcmp(buf.raw_buf(), str[0], strlen(str[0])) == 0);

    ret = rd.read_record(&buf);
    CHECK(ret == CheckedLogReader::IGNORE);
    CHECK(memcmp(buf.raw_buf(), str[1], strlen(str[1])) == 0);

    ret = rd.read_record(&buf);
    CHECK(ret == 0);
    CHECK(memcmp(buf.raw_buf(), str[2], strlen(str[2])) == 0);

    ret = rd.read_record(&buf);
    CHECK(ret == CheckedLogReader::END);

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(WriteTest);
    ADD_TEST(ReadTest);
}

DECLARE_TEST_FILE(Test, "checked log test");
