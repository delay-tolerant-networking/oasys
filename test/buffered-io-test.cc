#if 0  // Python part ---------------------------------------------------------

def InputTest():
    print "XXX not done yet";

# C++ part --------------------------------------------------------------------
CC=""" # "
#endif // 0

#include <vector>

#include <io/IOClient.h>
#include <io/BufferedIO.h>

#include <util/UnitTest.h>

using namespace oasys;

/*
struct TestClient : public IOClient {
    TestClient() : read_num_(0) {}

    int readv(const struct iovec* iov, int iovcnt)            { return 0; }
    int readall(char* bp, size_t len)                         { return 0; }
    int readvall(const struct iovec* iov, int iovcnt)         { return 0; }
    int writev(const struct iovec* iov, int iovcnt)           { return 0; }
    int writeall(const char* bp, size_t len)                  { return 0; }
    int writevall(const struct iovec* iov, int iovcnt)        { return 0; }
    int timeout_read(char* bp, size_t len, int timeout_ms)    { return 0; }
    int timeout_readall(char* bp, size_t len, int timeout_ms) { return 0; }
    int timeout_readv(const struct iovec* iov, int iovcnt,
                      int timeout_ms) { return 0; }
    int timeout_readvall(const struct iovec* iov, int iovcnt, 
                         int timeout_ms) { return 0; }
    int set_nonblocking(bool b)  { return 0; }
    int get_nonblocking(bool* b) { return 0; }


    int read(char* bp, size_t len) {
        return 0;
    }
    int write(const char* bp, size_t len) { 
        return 0;
    }

    static int    amount_[] = { 3, 1, 7, 4000, 2, 529, 10120, 1, 0 };
    ByteGenerator byte_gen_;
};
*/

DECLARE_UNIT_TEST(ATest) {
    return 0;
}

DECLARE_UNIT_TEST(AnotherTest) {
    return UNIT_TEST_FAILED;
}

DECLARE_UNIT_TEST(InputTest) {
    return UNIT_TEST_INPUT;
}

class Test : public UnitTester {
    DECLARE_TESTER(Test);

    void add_tests() {
        ADD_UNIT_TEST(ATest);
        ADD_UNIT_TEST(AnotherTest);
        ADD_UNIT_TEST(InputTest);
    }
};

DECLARE_TEST_FILE(Test, "BufferedIO");

// """ # ----------------------------------------------------------------------
