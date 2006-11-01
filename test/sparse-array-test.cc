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

#include "util/UnitTest.h"
#include "util/SparseArray.h"

using namespace oasys;

typedef SparseArray<char> SA;

bool
check_range(const SA&     sa, 
            const size_t* offsets,
            const size_t* sizes,
            const char**  data,
            size_t        count)
{
    (void)data;
    
    size_t i = 0;
    for (SA::BlockList::const_iterator itr = sa.debug_blocks().begin();
         itr != sa.debug_blocks().end(); ++itr)
    {
        if (count - i == 0 ||
            itr->offset_ != offsets[i] || 
            itr->size_   != sizes[i])
        {
            return false;
        }

/*
        if (memcmp(data[i], itr->data_, itr->size_) != 0)
        {
            return false;
        }
*/
        ++i;
    }

    return true;
}

void 
dump_range(const SA& sa)
{
    for (SA::BlockList::const_iterator itr = sa.debug_blocks().begin();
         itr != sa.debug_blocks().end(); ++itr)
    {
        printf("(offset %u size %u) ", itr->offset_, itr->size_);
    }
    printf("\n");
}

DECLARE_TEST(Test1) {
    SA sa;

    sa.range_write(1, 4, "");
    sa.range_write(6, 9, "");
    sa.range_write(20, 25, "");

    size_t offsets[]      = { 1, 6, 20 };
    size_t sizes[]        = { 4, 9, 25 };
    const char* data[]    = { "", "", "" };
    
    CHECK(check_range(sa, offsets, sizes, data, 3));
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Test2) {
    SA sa;

    sa.range_write(1, 4, "");
    sa.range_write(1, 4, "");
    sa.range_write(7, 3, "");

    size_t offsets[]      = {1, 7};
    size_t sizes[]        = {4, 3};
    const char* data[]    = { "", ""};
    
    CHECK(check_range(sa, offsets, sizes, data, 2));
    
    return UNIT_TEST_PASSED;
}


DECLARE_TEST(Test3) {
    SA sa;

    sa.range_write(1, 4, "");
    sa.range_write(3, 4, "");
    sa.range_write(4, 5, "");
    sa.range_write(5, 5, "");

    size_t offsets[]      = {1};
    size_t sizes[]        = {9};
    const char* data[]    = { ""};
    
    CHECK(check_range(sa, offsets, sizes, data, 1));
    
    return UNIT_TEST_PASSED;
}


DECLARE_TEST(Test4) {
    SA sa;

    sa.range_write(2, 1, "");
    sa.range_write(4, 1, "");
    sa.range_write(7, 1, "");
    sa.range_write(0, 10, "");

    size_t offsets[]      = {0};
    size_t sizes[]        = {10};
    const char* data[]    = { ""};
    
    dump_range(sa);
    CHECK(check_range(sa, offsets, sizes, data, 1));
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Test5) {
    SA sa;

    sa.range_write(1, 10, "");
    sa.range_write(11, 10, "");
    sa.range_write(21, 10, "");

    size_t offsets[]      = {1, 11, 21};
    size_t sizes[]        = {10, 10, 10};
    const char* data[]    = {"", "", ""};
    
    dump_range(sa);
    CHECK(check_range(sa, offsets, sizes, data, 3));
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Test6) {
    SA sa;

    sa.range_write(3, 5,  "");
    sa.range_write(0, 4,  "");
    sa.range_write(9, 11, "");
    sa.range_write(7, 7,  "");

    size_t offsets[]      = {0};
    size_t sizes[]        = {20};
    const char* data[]    = {""};
    
    dump_range(sa);
    CHECK(check_range(sa, offsets, sizes, data, 1));
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Test7) {
    SA sa;

    sa.range_write(1, 10, "");
    sa.range_write(11, 10, "");
    sa.range_write(21, 10, "");

    size_t offsets[]      = {1, 11, 21};
    size_t sizes[]        = {10, 10, 10};
    const char* data[]    = {"", "", ""};
    
    dump_range(sa);
    CHECK(check_range(sa, offsets, sizes, data, 3));
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Test8) {
    SA sa;

    sa.range_write(1, 10, "");
    sa.range_write(11, 10, "");
    sa.range_write(21, 10, "");

    size_t offsets[]      = {1, 11, 21};
    size_t sizes[]        = {10, 10, 10};
    const char* data[]    = {"", "", ""};
    
    dump_range(sa);
    CHECK(check_range(sa, offsets, sizes, data, 3));
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Test9) {
    SA sa;

    sa.range_write(1, 10, "");
    sa.range_write(11, 10, "");
    sa.range_write(21, 10, "");

    size_t offsets[]      = {1, 11, 21};
    size_t sizes[]        = {10, 10, 10};
    const char* data[]    = {"", "", ""};
    
    dump_range(sa);
    CHECK(check_range(sa, offsets, sizes, data, 3));
    
    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(Test1);
    ADD_TEST(Test2);
    ADD_TEST(Test3);
    ADD_TEST(Test4);
    ADD_TEST(Test5);
    ADD_TEST(Test6);
    ADD_TEST(Test7);
    ADD_TEST(Test8);
    ADD_TEST(Test9);
}

DECLARE_TEST_FILE(Test, "sparse array test");
