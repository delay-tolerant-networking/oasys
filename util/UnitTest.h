/*
 *    Copyright 2005-2006 Intel Corporation
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

#ifndef __UNIT_TEST_H__
#define __UNIT_TEST_H__

#include <cstring>
#include <string>
#include <vector>
#include <stdio.h>
#include <errno.h>

#include "../debug/FatalSignals.h"
#include "../debug/Log.h"
#include "../io/PrettyPrintBuffer.h"
#include "../util/Random.h"

namespace oasys {

/**
 * @file
 *
 * Goes along with UnitTest.tcl in the test/ directory.
 *
 * See sample-test.cc for a good example of the UnitTesting
 * macros. They should cover most common case usages of the unit
 * testing framework. 
 *
 * Each small test is a thunk run by deriving from the UnitTest class
 * and adding the test to the list maintained in UnitTester. The
 * UnitTester then goes through each test and spits out to stderr a
 * Tcl struct which represents the success/failure of a particular
 * run. There is also a third type of test success condition, input,
 * which means that the test results cannot be determined by the
 * program and needs to be done externally.
 *
 * Each *-test.cc file is both a Tcl program and C++ program. For each
 * unit test class Foo that is defined to return UNIT_TEST_INPUT,
 * there should be Tcl proc checkFoo that checks the output of the
 * program for the correct property. The reason for putting the
 * Tcl/C++ together in the same file is to make it easy to maintain
 * the tests.
 *
 * Note that all Tcl code should be bracketed by conditional
 * compilation, using #ifdef DECLARE_TEST_TCL ... #endif.
 *
 * Each *-test in the directory is run and output is placed in
 * output/ *-test/std[out|err].
 *
 * TODO: save and rerun only those that failed
 */

/**
 * Override the method run to create an individual unit test.
 */
struct UnitTest {
    UnitTest(std::string name) : name_(name), failed_(false) {}
    virtual ~UnitTest() {}

    virtual int run() = 0;
    
    std::string name_;
    bool failed_;

    int         errno_;
    const char* strerror_;
};

enum {
    UNIT_TEST_PASSED = 0,
    UNIT_TEST_FAILED,
    UNIT_TEST_INPUT,      ///< Run Tcl script to query for success
};

/**
 * UnitTester runs all unit test and produces a nice format which
 * hooks into the parsing script.
 *
 * Output of the UnitTester is directed (for now) to stderr as a Tcl
 * list:
 *
 * {
 *   "testname" {
 *     {1 firstTest P} 
 *     {2 secondTest F} 
 *     {3 thirdTest I}
 *   }
 *   {2 1 1 1}
 * }
 */
class UnitTester {
    typedef std::vector<UnitTest*> UnitTestList;

public:
    UnitTester(std::string name) : 
        name_(name), passed_(0), failed_(0), input_(0) 
    {
    }
    
    virtual ~UnitTester() {}

    int run_tests(int argc, const char* argv[], bool init_log) {
        log_level_t level = LOG_NOTICE;

        if (argc != 0) 
        {
            // always skip argv[0]
            argc -= 1;
            argv += 1;
            
            // first look for -l debug (which must come first)
            if (argc >= 2 && (strcmp(argv[0], "-l") == 0))
            {
                level = str2level(argv[1]);
                argc -= 2;
                argv += 2;
            }
            
        }
        
        if (init_log)
        {
            Log::init(level);
        }
        FatalSignals::init(name_.c_str());
        
        add_tests();

        bool in_tcl = false;
        
        if (argc >= 1 &&
            ((strcmp(argv[0], "-h") == 0) ||
             (strcmp(argv[0], "-help") == 0) ||
             (strcmp(argv[0], "--help") == 0)))
        {
            printf("%s [-h] {[test name]}*\n", argv[0]);
            printf("test names:\n");
            for (UnitTestList::const_iterator i = tests_.begin();
                 i != tests_.end(); ++i)
            {
                printf("    %s\n", (*i)->name_.c_str());
            }
                   
            exit(0);
        }

        struct timeval tv;
        ::gettimeofday(&tv, 0);
        u_int random_seed = tv.tv_sec;
        
        if (argc >= 2 &&
            (strcmp(argv[0], "--seed") == 0))
        {
            char* end;
            random_seed = strtoul(argv[1], &end, 10);
            argc -= 2;
            argv += 2;
        }

        oasys::Random::seed(random_seed);
        printf("Test random seed: %u\n", random_seed);

        if (argc >= 1 && (strcmp(argv[1], "-test") == 0)) {
            argc -= 1;
            argv += 1;
            in_tcl = true;
        }
        
        UnitTestList new_tests;
        while (argc != 0) {
            for (UnitTestList::iterator i = tests_.begin();
                 i != tests_.end(); ++i)
            {
                const char* testname = argv[0];
                if (strcmp((*i)->name_.c_str(), testname) == 0) {
                    new_tests.push_back(*i);
                }
            }            
            argc--;
            argv++;
        }
        if (new_tests.size() > 0) {
            std::swap(tests_, new_tests);
        }

        if (in_tcl) {
            print_tcl_header();
        } else {
            print_header();
        }

        int test_num = 1;
        for (UnitTestList::iterator i=tests_.begin(); 
             i != tests_.end(); ++i, ++test_num) 
        {
            printf("%s...\n",  (*i)->name_.c_str());
            fflush(stdout);
            
            int err = (*i)->run();
            switch(err) {
            case UNIT_TEST_PASSED:
                if (in_tcl) {
                    fprintf(stderr, "{ %d %s P } ", 
                            test_num, (*i)->name_.c_str());
                } else {
                    printf("%s... Passed\n\n",  (*i)->name_.c_str());
                }
                passed_++;
                break;
            case UNIT_TEST_FAILED:
                if (in_tcl) {
                    fprintf(stderr, "{ %d %s F } ", 
                            test_num, (*i)->name_.c_str());
                } else {
                    printf("%s... Failed\n\n",  (*i)->name_.c_str());
                }
                failed_++;
                break;
            case UNIT_TEST_INPUT:
                if (in_tcl) {
                    fprintf(stderr, "{ %d %s I } ", 
                            test_num, (*i)->name_.c_str());
                } else {
                    printf("%s... Unknown (UNIT_TEST_INPUT)\n\n",
                           (*i)->name_.c_str());
                }
                input_++;
                break;                
            }
        }

        if (in_tcl) {
            print_tcl_tail();
        } else {
            print_results();
        }

        return 0;
    }

    void print_tcl_header() {
        fprintf(stderr, "set result { \"%s\" { ", name_.c_str());
    }
    void print_tcl_tail() {
        fprintf(stderr, "} { %zu %d %d %d } }\n", 
                tests_.size(), passed_, failed_, input_);
    }
    void print_header() {
        printf("\n\nRunning Test: %s\n\n", name_.c_str());
    }
    
    void print_results() {
        printf("\n%s Complete:\n", name_.c_str());
        if (passed_ != 0) {
            printf("\t\t%u Passed\n", passed_);
        }
        if (failed_ != 0) {
            printf("\t\t%u Failed\n", failed_);
        }
    }

protected:
    /**
     * Override this to add your tests.
     */
    virtual void add_tests() = 0;

    /**
     * Add a unit test to the suite.
     */
    void add(UnitTest* unit) {
        tests_.push_back(unit);
    }
    
private:
    std::string  name_;
    UnitTestList tests_;

    int passed_;
    int failed_;
    int input_;
};

/// @{ Helper macros for implementing unit tests
#define ADD_TEST(_name)                         \
        add(new _name ## UnitTest())

#define DECLARE_TEST(_name)                             \
    struct _name ## UnitTest : public oasys::UnitTest { \
        _name ## UnitTest() : UnitTest(#_name) {}       \
        int run();                                      \
    };                                                  \
    int _name ## UnitTest::run()

#define RUN_TESTER(_UnitTesterClass, testname, argc, argv)      \
    _UnitTesterClass test(testname);                            \
    int _ret = test.run_tests(argc, argv, true);                \
    return _ret;

#define RUN_TESTER_NO_LOG(_UnitTesterClass, testname, argc, argv) \
    _UnitTesterClass test(testname);                              \
    int _ret = test.run_tests(argc, argv, false);                 \
    return _ret;

#define DECLARE_TEST_FILE(_UnitTesterClass, testname)           \
int main(int argc, const char* argv[]) {                        \
    RUN_TESTER(_UnitTesterClass, testname, argc, argv);         \
}

#define DECLARE_TESTER(_name)                                   \
class _name : public oasys::UnitTester {                        \
public:                                                         \
    _name(std::string name) : UnitTester(name) {}               \
protected:                                                      \
    void add_tests();                                           \
};                                                              \
void _name::add_tests()                                         \

#define DO(x)                                                           \
    do {                                                                \
        ::oasys::__logf(oasys::LOG_NOTICE, "/test",                     \
                    "DO (%s) at %s:%d", #x, __FILE__, __LINE__);        \
        x;                                                              \
    } while (0)

#define CHECK(x)                                                        \
    do { if (! (x)) {                                                   \
	errno_ = errno;							\
	strerror_ = strerror(errno_);					\
        ::oasys::Breaker::break_here();                                 \
        ::oasys::__logf(oasys::LOG_ERR, "/test",                        \
                    "CHECK FAILED (%s) at %s:%d",                       \
                    #x, __FILE__, __LINE__);                            \
        return oasys::UNIT_TEST_FAILED;                                 \
    } else {                                                            \
        ::oasys::__logf(oasys::LOG_NOTICE, "/test",                     \
                    "CHECK (%s) ok at %s:%d", #x, __FILE__, __LINE__);  \
    } } while(0)

#define CHECK_SYS(x)                                                    \
    do { if (! (x)) {                                                   \
	errno_ = errno;							\
	strerror_ = strerror(errno_);					\
        ::oasys::Breaker::break_here();                                 \
        ::oasys::__logf(oasys::LOG_ERR, "/test",                        \
                    "CHECK FAILED (%s) at %s:%d, errno=%s",             \
                    #x, __FILE__, __LINE__, strerror_);                 \
        return oasys::UNIT_TEST_FAILED;                                 \
    } else {                                                            \
        ::oasys::__logf(oasys::LOG_NOTICE, "/test",                     \
                    "CHECK (%s) ok at %s:%d", #x, __FILE__, __LINE__);  \
    } } while(0)

#define CHECK_EQUAL(_a, _b)                                                     \
    do { int a = _a; int b = _b; if ((a) != (b)) {                              \
	errno_ = errno;								\
	strerror_ = strerror(errno_);						\
        ::oasys::Breaker::break_here();                                         \
        oasys::logf("/test", oasys::LOG_ERR,                                    \
                    "CHECK FAILED: '" #_a "' (%d) != '" #_b "' (%d) at %s:%d",  \
                    (a), (b), __FILE__, __LINE__);                              \
        return oasys::UNIT_TEST_FAILED;                                         \
    } else {                                                                    \
        oasys::logf("/test", oasys::LOG_NOTICE,                                 \
                    "CHECK '" #_a "' (%d) == '" #_b "' (%d) "                   \
                    "at %s:%d", (a), (b), __FILE__, __LINE__);                  \
    } } while(0)

#define CHECK_LT(_a, _b)                                                        \
    do { int a = _a; int b = _b; if (! (((a) < (b))) {                          \
	errno_ = errno;								\
	strerror_ = strerror(errno_);						\
        ::oasys::Breaker::break_here();                                         \
        oasys::logf("/test", oasys::LOG_ERR,                                    \
                    "CHECK FAILED: '" #_a "' (%d) < '" #_b "' (%d) at %s:%d",  \
                    (a), (b), __FILE__, __LINE__);                              \
        return oasys::UNIT_TEST_FAILED;                                         \
    } else {                                                                    \
        oasys::logf("/test", oasys::LOG_NOTICE,                                 \
                    "CHECK '" #_a "' (%d) < '" #_b "' (%d) "                    \
                    "at %s:%d", (a), (b), __FILE__, __LINE__);                  \
    } } while(0)

#define CHECK_GT(_a, _b)                                                        \
    do { int a = _a; int b = _b; if (! ((a) > (b))) {                           \
	errno_ = errno;								\
	strerror_ = strerror(errno_);						\
        ::oasys::Breaker::break_here();                                         \
        oasys::logf("/test", oasys::LOG_ERR,                                    \
                    "CHECK FAILED: '" #_a "' (%d) > '" #_b "' (%d) at %s:%d",  \
                    (a), (b), __FILE__, __LINE__);                              \
        return oasys::UNIT_TEST_FAILED;                                         \
    } else {                                                                    \
        oasys::logf("/test", oasys::LOG_NOTICE,                                 \
                    "CHECK '" #_a "' (%d) > '" #_b "' (%d) "                    \
                    "at %s:%d", (a), (b), __FILE__, __LINE__);                  \
    } } while(0)

#define CHECK_LTU(_a, _b)                                                       \
    do { u_int a = _a; u_int b = _b; if (! ((a) <= (b))) {                      \
	errno_ = errno;								\
	strerror_ = strerror(errno_);						\
        ::oasys::Breaker::break_here();                                         \
        oasys::logf("/test", oasys::LOG_ERR,                                    \
                    "CHECK FAILED: '" #_a "' (%u) <= '" #_b "' (%u) at %s:%u",  \
                    (a), (b), __FILE__, __LINE__);                              \
        return oasys::UNIT_TEST_FAILED;                                         \
    } else {                                                                    \
        oasys::logf("/test", oasys::LOG_NOTICE,                                 \
                    "CHECK '" #_a "' (%u) <= '" #_b "' (%u) "                   \
                    "at %s:%d", (a), (b), __FILE__, __LINE__);                  \
    } } while(0)

#define CHECK_GTU(_a, _b)                                                       \
    do { u_int a = _a; u_int b = _b; if (! ((a) >= (b))) {                      \
	errno_ = errno;								\
	strerror_ = strerror(errno_);						\
        ::oasys::Breaker::break_here();                                         \
        oasys::logf("/test", oasys::LOG_ERR,                                    \
                    "CHECK FAILED: '" #_a "' (%u) >= '" #_b "' (%u) at %s:%u",  \
                    (a), (b), __FILE__, __LINE__);                              \
        return oasys::UNIT_TEST_FAILED;                                         \
    } else {                                                                    \
        oasys::logf("/test", oasys::LOG_NOTICE,                                 \
                    "CHECK '" #_a "' (%u) >= '" #_b "' (%u) "                   \
                    "at %s:%d", (a), (b), __FILE__, __LINE__);                  \
    } } while(0)

#define CHECK_EQUAL_U64(a, b)                                                           \
    do { if ((a) != (b)) {                                                              \
	errno_ = errno;									\
	strerror_ = strerror(errno_);							\
        ::oasys::Breaker::break_here();                                                 \
        oasys::logf("/test", oasys::LOG_ERR,                                            \
                    "CHECK FAILED: '" #a "' (%llu) != '" #b "' (%llu) at %s:%d",        \
                    (long long unsigned int)(a),                                        \
                    (long long unsigned int)(b),                                        \
                    __FILE__, __LINE__);                                                \
        return oasys::UNIT_TEST_FAILED;                                                 \
    } else {                                                                            \
        oasys::logf("/test", oasys::LOG_NOTICE,                                         \
                    "CHECK '" #a "' (%llu) == '" #b "' (%llu) "                         \
                    "at %s:%d",                                                         \
                    (long long unsigned int)(a),                                        \
                    (long long unsigned int)(b),                                        \
                    __FILE__, __LINE__);                                                \
    } } while(0)

#define CHECK_EQUALSTR(a, b)                                            \
    do { if (strcmp((a), (b)) != 0) {                                   \
	errno_ = errno;							\
	strerror_ = strerror(errno_);					\
        ::oasys::Breaker::break_here();                                 \
        oasys::logf("/test", oasys::LOG_ERR,                            \
                    "CHECK FAILED: '" #a "' != '" #b "' at %s:%d.",     \
                    __FILE__, __LINE__);                                \
        oasys::logf("/test", oasys::LOG_ERR, "Contents of " #a          \
                    " (length %zu): ", strlen(a));                      \
        oasys::PrettyPrintBuf buf_a(a, strlen(a));                      \
        std::string s;                                                  \
        bool done;                                                      \
        do {                                                            \
            done = buf_a.next_str(&s);                                  \
            oasys::logf("/test", oasys::LOG_ERR, s.c_str());            \
        } while (!done);                                                \
                                                                        \
        oasys::logf("/test", oasys::LOG_ERR, "Contents of " #b          \
                    " (length %zu): ", strlen(b));                      \
        oasys::PrettyPrintBuf buf_b(b, strlen(b));                      \
                                                                        \
        do {                                                            \
            done = buf_b.next_str(&s);                                  \
            oasys::logf("/test", oasys::LOG_ERR, s.c_str());            \
        } while (!done);                                                \
                                                                        \
        return oasys::UNIT_TEST_FAILED;                                 \
    } else {                                                            \
        oasys::logf("/test", oasys::LOG_NOTICE,                         \
                    "CHECK '" #a "' (%s) == '" #b "' (%s) "             \
                    "at %s:%d", (a), (b), __FILE__, __LINE__);          \
    } } while(0);

#define CHECK_EQUALSTRN(a, b, len)                                              \
    do { u_int print_len = (len > 32) ? 32 : len;                               \
         if (strncmp((const char*)(a), (const char*)(b), (len)) != 0) {         \
	     errno_ = errno;							\
	     strerror_ = strerror(errno_);					\
             ::oasys::Breaker::break_here();                                    \
             oasys::logf("/test", oasys::LOG_ERR,  "CHECK FAILED: "             \
                         "'" #a "' (%.*s...) != '" #b "' (%.*s...) at %s:%d",   \
                         print_len, (a), print_len, (b),                        \
                         __FILE__, __LINE__);                                   \
             return oasys::UNIT_TEST_FAILED;                                    \
         } else {                                                               \
             oasys::logf("/test", oasys::LOG_NOTICE,                            \
                         "CHECK '" #a "' (%.*s...) == '" #b "' (%.*s...) "      \
                         "at %s:%d",                                            \
                         print_len, (a), print_len, (b), __FILE__, __LINE__);   \
        }                                                                       \
    } while(0)
 
/// @}

}; // namespace oasys

#endif //__UNIT_TEST_H__
