#include <stdio.h>
#include <string>
#include "util/SDNV.h"
#include "util/UnitTest.h"

using namespace std;
using namespace oasys;

u_char buf[64];

bool
test(u_int64_t val, int expected_len)
{
    int len;
    len = SDNV::encode(val, buf, sizeof(buf));
    if (len != expected_len) {
        log_err("/sdnv/test", "encode %llu expected %d byte(s), used %d",
                val, expected_len, len);
        return UNIT_TEST_FAILED;
    }

    u_int64_t val2 = 0;
    len = SDNV::decode(buf, len, &val2);
    if (len != expected_len) {
        log_err("/sdnv/test", "decode %llu expected %d byte(s), used %d",
                val, expected_len, len);
        return UNIT_TEST_FAILED;
    }

    CHECK_EQUAL_U64(val, val2);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(OneByte) {
    // a few random value tests
    CHECK(test(0, 1) == UNIT_TEST_PASSED);
    CHECK(test(1, 1) == UNIT_TEST_PASSED);
    CHECK(test(2, 1) == UNIT_TEST_PASSED);
    CHECK(test(16, 1) == UNIT_TEST_PASSED);
    CHECK(test(99, 1) == UNIT_TEST_PASSED);
    CHECK(test(101, 1) == UNIT_TEST_PASSED);
    CHECK(test(127, 1) == UNIT_TEST_PASSED);

    // a few checks that the encoding is actually what we expect
    CHECK_EQUAL(SDNV::encode(0, buf, 1), 1);
    CHECK_EQUAL(buf[0], 0);

    CHECK_EQUAL(SDNV::encode(16, buf, 1), 1);
    CHECK_EQUAL(buf[0], 16);
    
    CHECK_EQUAL(SDNV::encode(127, buf, 1), 1);
    CHECK_EQUAL(buf[0], 127);

    // buffer length checks
    CHECK_EQUAL(SDNV::encode(0, buf, 0), -1);
    CHECK_EQUAL(SDNV::decode(buf, 0, NULL), -1);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(MultiByte) {
    // some random value checks
    CHECK(test(1001, 2) == UNIT_TEST_PASSED);
    CHECK(test(0xabcd, 3) == UNIT_TEST_PASSED);
    CHECK(test(0xabcde, 3) == UNIT_TEST_PASSED);
    CHECK(test(0xabcdef, 4) == UNIT_TEST_PASSED);
    CHECK(test(0x0abcdef, 4) == UNIT_TEST_PASSED);
    CHECK(test(0xfabcdef, 4) == UNIT_TEST_PASSED);

    // now check that the encoding is what we expect 
    CHECK_EQUAL(SDNV::encode(0xab, buf, sizeof(buf)), 2);
    CHECK_EQUAL(buf[0], (1 << 7));
    CHECK_EQUAL(buf[1], 0xab);

    CHECK_EQUAL(SDNV::encode(0xabc, buf, sizeof(buf)), 2);
    CHECK_EQUAL(buf[0], (1 << 7) | 0xa);
    CHECK_EQUAL(buf[1], 0xbc);

    CHECK_EQUAL(SDNV::encode(0xabcde, buf, sizeof(buf)), 3);
    CHECK_EQUAL(buf[0], (1 << 7) | (1 << 4) | 0xa);
    CHECK_EQUAL(buf[1], 0xbc);
    CHECK_EQUAL(buf[2], 0xde);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Bounds) {
    // check the boundary cases
    CHECK(test(0LL,                  1) == UNIT_TEST_PASSED);
    CHECK(test(0x7fLL,               1) == UNIT_TEST_PASSED);
    
    CHECK(test(0x80LL,               2) == UNIT_TEST_PASSED);
    CHECK(test(0xfffLL,              2) == UNIT_TEST_PASSED);

    CHECK(test(0x1000LL,             3) == UNIT_TEST_PASSED);
    CHECK(test(0xfffffLL,            3) == UNIT_TEST_PASSED);

    CHECK(test(0x100000LL,           4) == UNIT_TEST_PASSED);
    CHECK(test(0xfffffffLL,          4) == UNIT_TEST_PASSED);
    
    CHECK(test(0x10000000LL,         5) == UNIT_TEST_PASSED);
    CHECK(test(0xfffffffffLL,        5) == UNIT_TEST_PASSED);

    CHECK(test(0x1000000000LL,       6) == UNIT_TEST_PASSED);
    CHECK(test(0xfffffffffffLL,      6) == UNIT_TEST_PASSED);

    CHECK(test(0x100000000000LL,     7) == UNIT_TEST_PASSED);
    CHECK(test(0xfffffffffffffLL,    7) == UNIT_TEST_PASSED);

    CHECK(test(0x10000000000000LL,   8) == UNIT_TEST_PASSED);
    CHECK(test(0xfffffffffffffffLL,  8) == UNIT_TEST_PASSED);
    
    CHECK(test(0x1000000000000000LL, 9) == UNIT_TEST_PASSED);
    CHECK(test(0xffffffffffffffffLL, 9) == UNIT_TEST_PASSED);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(SDNVTest) {
    ADD_TEST(OneByte);
    ADD_TEST(MultiByte);
    ADD_TEST(Bounds);
}

DECLARE_TEST_FILE(SDNVTest, "sdnv test");
