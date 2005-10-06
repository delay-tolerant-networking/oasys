
#include "util/UnitTest.h"
#include "serialize/TypeShims.h"
#include "storage/DurableStore.h"

using namespace oasys;

DurableObjectCache<StringShim>* cache_;

DECLARE_TEST(Init) {
    cache_ = new DurableObjectCache<StringShim>(32);
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Put) {
    StringShim* s = new StringShim("test");
    CHECK(cache_->put(IntShim(1), s, 0) == DS_OK);
    CHECK_EQUAL(cache_->size(), 8);

    CHECK(cache_->put(IntShim(1), s, 0) == DS_OK);
    CHECK_EQUAL(cache_->size(), 8);
    
    CHECK(cache_->put(IntShim(1), s, DS_EXCL) == DS_EXISTS);
    CHECK_EQUAL(cache_->size(), 8);
    
    CHECK(cache_->put(IntShim(2), new StringShim("test test"), 0) == DS_OK);
    CHECK_EQUAL(cache_->size(), 21);

    // both these items put the cache over capacity
    log_info("flamebox-ignore ign1 cache already at capacity");
    
    CHECK(cache_->put(IntShim(3), new StringShim("test test test"), 0) == DS_OK);
    CHECK_EQUAL(cache_->size(), 39);

    CHECK(cache_->put(IntShim(4), new StringShim("test"), 0) == DS_OK);
    CHECK_EQUAL(cache_->size(), 47);

    log_info("flamebox-ignore-cancel ign1");
        
    CHECK_EQUAL(cache_->count(), 4);
    CHECK_EQUAL(cache_->live(), 4);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Get) {
    StringShim *s, *s2;

    CHECK(cache_->get(IntShim(1), &s) == DS_OK);
    CHECK_EQUALSTR(s->value().c_str(), "test");
    CHECK(cache_->hits() == 1);
    CHECK(cache_->misses() == 0);
    
    CHECK(cache_->get(IntShim(4), &s2) == DS_OK);
    CHECK_EQUALSTR(s2->value().c_str(), "test");
    CHECK(s != s2);
    CHECK(cache_->hits() == 2);
    CHECK(cache_->misses() == 0);

    CHECK(cache_->get(IntShim(99), &s) == DS_NOTFOUND);
    CHECK(cache_->hits() == 2);
    CHECK(cache_->misses() == 1);

    CHECK_EQUAL(cache_->count(), 4);
    CHECK_EQUAL(cache_->live(), 4);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Release) {
    StringShim* s;
    CHECK(cache_->evictions() == 0);
    CHECK(cache_->get(IntShim(2), &s) == DS_OK);
    CHECK(cache_->release(IntShim(2), s) == DS_OK);
    CHECK(cache_->evictions() == 1);
    CHECK(cache_->get(IntShim(2), &s) == DS_NOTFOUND);
    CHECK_EQUAL(cache_->size(), 34);

    CHECK_EQUAL(cache_->count(), 3);
    CHECK_EQUAL(cache_->live(), 3);

    CHECK(cache_->get(IntShim(1), &s) == DS_OK);
    CHECK(cache_->release(IntShim(1), s) == DS_OK);
    CHECK(cache_->evictions() == 2);
    CHECK(cache_->get(IntShim(2), &s) == DS_NOTFOUND);
    CHECK_EQUAL(cache_->size(), 26);
    
    CHECK(cache_->get(IntShim(3), &s) == DS_OK);
    CHECK(cache_->release(IntShim(3), s) == DS_OK);
    CHECK(cache_->get(IntShim(4), &s) == DS_OK);
    CHECK(cache_->release(IntShim(4), s) == DS_OK);

    CHECK_EQUAL(cache_->count(), 2);
    CHECK_EQUAL(cache_->live(), 0);
    
    CHECK(cache_->get(IntShim(3), &s) == DS_OK);
    CHECK_EQUAL(cache_->count(), 2);
    CHECK_EQUAL(cache_->live(), 1);
    CHECK_EQUALSTR(s->value().c_str(), "test test test");

    CHECK(cache_->release(IntShim(3), s) == DS_OK);
    CHECK_EQUAL(cache_->count(), 2);
    CHECK_EQUAL(cache_->live(), 0);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(PutEvict) {
    StringShim* s;
    CHECK(cache_->put(IntShim(1), new StringShim("test test"), 0) == DS_OK);
    CHECK(cache_->evictions() == 3);
    CHECK_EQUAL(cache_->count(), 2);
    CHECK_EQUAL(cache_->live(), 1);

    CHECK(cache_->get(IntShim(3), &s) == DS_OK);
    CHECK_EQUAL(cache_->count(), 2);
    CHECK_EQUAL(cache_->live(), 2);

    CHECK(cache_->release(IntShim(3), s) == DS_OK);

    CHECK(cache_->get(IntShim(1), &s) == DS_OK);
    CHECK(cache_->release(IntShim(1), s) == DS_OK);
    
    log_info("flamebox-ignore ign1 cache already at capacity");
    s = new StringShim("really really really really really really big");
    CHECK(cache_->put(IntShim(2), s, 0) == DS_OK);
    log_info("flamebox-ignore-cancel ign1");
    CHECK(cache_->evictions() == 5);
    CHECK_EQUAL(cache_->count(), 1);
    CHECK_EQUAL(cache_->live(), 1);

    CHECK(cache_->release(IntShim(2), s) == DS_OK);
    CHECK(cache_->evictions() == 6);
    CHECK_EQUAL(cache_->count(), 0);
    CHECK_EQUAL(cache_->live(), 0);

    return UNIT_TEST_PASSED;
}
    
DECLARE_TEST(Del) {
    StringShim* s;
    CHECK(cache_->put(IntShim(1), new StringShim("test"), 0) == DS_OK);
    CHECK(cache_->get(IntShim(1), &s) == DS_OK);
    CHECK(cache_->del(IntShim(1)) == DS_ERR); // still live
    CHECK(cache_->release(IntShim(1), s) == DS_OK);
    CHECK(cache_->del(IntShim(1)) == DS_OK);
    CHECK(cache_->del(IntShim(1)) == DS_NOTFOUND);
    CHECK_EQUAL(cache_->count(), 0);
    CHECK_EQUAL(cache_->live(), 0);
    CHECK_EQUAL(cache_->size(), 0);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Flush) {
    StringShim* s1 = new StringShim("test");
    StringShim* s2 = new StringShim("test 2");
    CHECK(cache_->put(IntShim(1), s1, 0) == DS_OK);
    CHECK(cache_->release(IntShim(1), s1) == DS_OK);
    CHECK(cache_->put(IntShim(2), s2, 0) == DS_OK);
    
    CHECK_EQUAL(cache_->count(), 2);
    CHECK_EQUAL(cache_->live(), 1);
    
    CHECK_EQUAL(cache_->flush(), 1);

    CHECK_EQUAL(cache_->count(), 1);
    CHECK_EQUAL(cache_->live(), 1);

    StringShim* s;
    CHECK(cache_->get(IntShim(1), &s) == DS_NOTFOUND);
    CHECK(cache_->get(IntShim(2), &s) == DS_OK);

    CHECK(s == s2);
    CHECK(cache_->release(IntShim(2), s) == DS_OK);
    CHECK_EQUAL(cache_->flush(), 1);
    
    CHECK_EQUAL(cache_->count(), 0);
    CHECK_EQUAL(cache_->live(), 0);
    CHECK_EQUAL(cache_->size(), 0);
    
    CHECK_EQUAL(cache_->flush(), 0);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(CacheTester) {
    ADD_TEST(Init);
    ADD_TEST(Put);
    ADD_TEST(Get);
    ADD_TEST(Release);
    ADD_TEST(PutEvict);
    ADD_TEST(Del);
    ADD_TEST(Flush);
}

DECLARE_TEST_FILE(CacheTester, "DurableCache tester");
