#if 0  // tcl

proc checkInputTest {output} {
    return -1
}

set CC {
#endif // tcl

#include <util/UnitTest.h>
using namespace oasys;

DECLARE_UNIT_TEST(ATest) {
    return 0;
}

DECLARE_UNIT_TEST(AnotherTest) {
    return UNIT_TEST_FAILED;
}

DECLARE_UNIT_TEST(InputTest) {
    log_debug("/test", "InputTest: should be foo");
    
    return UNIT_TEST_INPUT;
}

class Test : public UnitTester {
    DECLARE_TESTER(Test) {
        ADD_UNIT_TEST(ATest);
        ADD_UNIT_TEST(AnotherTest);
        ADD_UNIT_TEST(InputTest);
    }
};

DECLARE_TEST_FILE(Test, "sample test");

// }
