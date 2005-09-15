
#include "util/UnitTest.h"
#include "serialize/TypeShims.h"
#include "storage/DurableStore.h"

using namespace oasys;

DurableObjectCache<StringShim>* cache_;

DECLARE_TEST(Init) {
    cache_ = new DurableObjectCache<StringShim>(32);
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Add) {
    StringShim* s = new StringShim("test");
    CHECK(cache_->add(IntShim(1), s) == DS_OK);
    CHECK_EQUAL(cache_->size(), 8);

    CHECK(cache_->add(IntShim(1), s) == DS_EXISTS);
    CHECK_EQUAL(cache_->size(), 8);
    
    CHECK(cache_->add(IntShim(2), new StringShim("test test")) == DS_OK);
    CHECK_EQUAL(cache_->size(), 21);

    // both these items put the cache over capacity
    CHECK(cache_->add(IntShim(3), new StringShim("test test test")) == DS_OK);
    CHECK_EQUAL(cache_->size(), 39);

    CHECK(cache_->add(IntShim(4), new StringShim("test")) == DS_OK);
    CHECK_EQUAL(cache_->size(), 47);

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

DECLARE_TEST(AddEvict) {
    StringShim* s;
    CHECK(cache_->add(IntShim(1), new StringShim("test test")) == DS_OK);
    CHECK(cache_->evictions() == 3);
    CHECK_EQUAL(cache_->count(), 2);
    CHECK_EQUAL(cache_->live(), 1);

    CHECK(cache_->get(IntShim(3), &s) == DS_OK);
    CHECK_EQUAL(cache_->count(), 2);
    CHECK_EQUAL(cache_->live(), 2);

    CHECK(cache_->release(IntShim(3), s) == DS_OK);

    CHECK(cache_->get(IntShim(1), &s) == DS_OK);
    CHECK(cache_->release(IntShim(1), s) == DS_OK);
    
    s = new StringShim("really really really really really really big");
    CHECK(cache_->add(IntShim(2), s) == DS_OK);
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
    CHECK(cache_->add(IntShim(1), new StringShim("test")) == DS_OK);
    CHECK(cache_->get(IntShim(1), &s) == DS_OK);
    CHECK(cache_->del(IntShim(1), s) == DS_OK);
    CHECK_EQUAL(cache_->count(), 0);
    CHECK_EQUAL(cache_->live(), 0);
    CHECK_EQUAL(cache_->size(), 0);

    CHECK(cache_->del(IntShim(2), s) == DS_NOTFOUND);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(CacheTester) {
    ADD_TEST(Init);
    ADD_TEST(Add);
    ADD_TEST(Get);
    ADD_TEST(Release);
    ADD_TEST(AddEvict);
    ADD_TEST(Del);
}

DECLARE_TEST_FILE(CacheTester, "DurableCache tester");