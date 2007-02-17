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
#include "util/STLUtil.h"
#include "util/Cache.h"

using namespace oasys;

struct Helper {
    bool over_limit(const std::string& key, const int& value)
    {
        (void) key; (void) value;
        return elts_ + 1> max_;
    }

    void put(const std::string& key, const int& value)
    {
        (void) key; (void) value;
        ++elts_;
        birth_log_.push_back(value);
    }

    void cleanup(std::string& key, const int& value)
    {
        (void) key; (void) value;
        --elts_;
        death_log_.push_back(value);
        log_notice_p("/test", "%d evicted", value);
    }

    size_t max_;
    size_t elts_;

    std::vector<int> birth_log_;
    std::vector<int> death_log_;
};

typedef Cache<std::string, int, Helper> TestCache;

DECLARE_TEST(Test1) {
    Helper h;
    
    h.max_  = 3;
    h.elts_ = 0;
    
    TestCache cache("/test-cache", h);
    
    cache.put_and_pin("a", 1);
    cache.unpin("a");
    cache.put_and_pin("b", 2);
    cache.unpin("b");
    cache.put_and_pin("c", 3);
    cache.put_and_pin("d", 4);
    cache.unpin("d");

    std::vector<int> c;
    CommaPushBack<std::vector<int>, int> pusher(&c);
    pusher = pusher, 1, 2, 3, 4;
    
    CHECK(cache.get_helper()->birth_log_ == c);
    
    c.clear();
    pusher = pusher, 1;
    CHECK(cache.get_helper()->death_log_ == c);

    cache.put_and_pin("e", 5);
    cache.unpin("e");
    cache.put_and_pin("f", 6);
    cache.unpin("f");
    cache.put_and_pin("g", 7);
    cache.unpin("g");

    c.clear();
    pusher = pusher, 1, 2, 3, 4, 5, 6, 7;
    CHECK(cache.get_helper()->birth_log_ == c);

    c.clear();
    pusher = pusher, 1, 2, 4, 5;
    CHECK(cache.get_helper()->death_log_ == c);

    int i;
    cache.get("f", &i);
    
    cache.put_and_pin("h", 8);
    cache.unpin("h");
    
    c.clear();
    pusher = pusher, 1, 2, 4, 5, 7;
    CHECK(cache.get_helper()->death_log_ == c);

    cache.put_and_pin("i", 9);
    cache.unpin("i");

    c.clear();
    pusher = pusher, 1, 2, 4, 5, 7, 6;
    CHECK(cache.get_helper()->death_log_ == c);

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(Test1);
}

DECLARE_TEST_FILE(Test, "cache test");
