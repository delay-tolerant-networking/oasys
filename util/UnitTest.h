#ifndef __UNIT_TEST_H__
#define __UNIT_TEST_H__

#include <string>
#include <vector>
#include <stdio.h>

#include "../debug/Log.h"

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
        Log::init(LOG_DEBUG);
    }
    virtual ~UnitTester() {}

    int run_tests(int argc, const char* argv[]) {
        bool in_tcl = false;
        
        if (argc >= 2 && (strcmp(argv[1], "-test") == 0)) {
            argc++;
            argv++;
            in_tcl = true;
        }
        // XXX/bowei parse arguments for test
        // for running just a single test

        add_tests();

        if (in_tcl) {
            print_header();
        }

        int test_num = 1;
        for(UnitTestList::iterator i=tests_.begin(); 
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
                    printf("%s... Passed\n",  (*i)->name_.c_str());
                }
                passed_++;
                break;
            case UNIT_TEST_FAILED:
                if (in_tcl) {
                    fprintf(stderr, "{ %d %s F } ", 
                            test_num, (*i)->name_.c_str());
                } else {
                    printf("%s... Failed\n",  (*i)->name_.c_str());
                }
                failed_++;
                break;
            case UNIT_TEST_INPUT:
                if (in_tcl) {
                    fprintf(stderr, "{ %d %s I } ", 
                            test_num, (*i)->name_.c_str());
                } else {
                    printf("%s... Unknown (UNIT_TEST_INPUT)\n",  (*i)->name_.c_str());
                }
                input_++;
                break;                
            }
        }

        if (in_tcl) {
            print_tail();
        }

        return 0;
    }

    void print_header() {
        fprintf(stderr, "set result { \"%s\" { ", name_.c_str());
    }
    void print_tail() {
        fprintf(stderr, "} { %d %d %d %d } }\n", 
                tests_.size(), passed_, failed_, input_);
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

#define DECLARE_TEST(_name)                                                     \
    struct _name ## UnitTest : public oasys::UnitTest, public oasys::Logger {   \
        _name ## UnitTest() : UnitTest(#_name), Logger("/unit") {}              \
        int run();                                                              \
    };                                                                          \
    int _name ## UnitTest::run()

#define DECLARE_TEST_FILE(_UnitTesterClass, testname)   \
int main(int argc, const char* argv[]) {                \
    _UnitTesterClass test(testname);                    \
                                                        \
    return test.run_tests(argc, argv);                  \
}                                               

#define DECLARE_TESTER(_name)                                   \
class _name : public oasys::UnitTester {                        \
public:                                                         \
    _name::_name(std::string name) : UnitTester(name) {}        \
protected:                                                      \
    void add_tests();                                           \
};                                                              \
void _name::add_tests()                                         \

#define CHECK(x)                                        \
    do { if (! (x)) {                                   \
        oasys::logf("/test", oasys::LOG_CRIT,           \
                    "CHECK FAILED (" #x ") at %s:%d",   \
                    __FILE__, __LINE__);                \
        return oasys::UNIT_TEST_FAILED;                 \
    } else {                                            \
        oasys::logf("/test", oasys::LOG_DEBUG,          \
                    "CHECK (" #x ") ok");               \
    } } while(0)

#define CHECK_EQUAL(a, b)                                                               \
    do { if ((a) != (b)) {                                                              \
        oasys::logf("/test", oasys::LOG_CRIT,                                           \
                    "CHECK FAILED: '" #a "' (%d) != '" #b "' (%d) at %s:%d",            \
                    (int)(a), (int)(b), __FILE__, __LINE__);                            \
        return oasys::UNIT_TEST_FAILED;                                                 \
    } else {                                                                            \
        oasys::logf("/test", oasys::LOG_DEBUG,                                          \
                    "CHECK '" #a "' (%d) == '" #b "' (%d)", (int)(a), (int)(b));        \
    } } while(0)

#define CHECK_EQUALSTR(a, b)                                                    \
    do { if (strcmp((a), (b)) != 0) {                                           \
        oasys::logf("/test", oasys::LOG_CRIT,                                   \
                    "CHECK FAILED: '" #a "' (%s) != '" #b "' (%s) at %s:%d",    \
                    (a), (b), __FILE__, __LINE__);                              \
        return oasys::UNIT_TEST_FAILED;                                         \
    } else {                                                                    \
        oasys::logf("/test", oasys::LOG_DEBUG,                                  \
                    "CHECK '" #a "' (%s) == '" #b "' (%s)", (a), (b));          \
    } } while(0);

#define CHECK_EQUALSTRN(a, b, len)                                              \
    do { if (strncmp((a), (b), (len)) != 0) {                                   \
        oasys::logf("/test", oasys::LOG_CRIT,                                   \
                    "CHECK FAILED: '" #a "' (%s) != '" #b "' (%s) at %s:%d",    \
                    (a), (b), __FILE__, __LINE__);                              \
        return oasys::UNIT_TEST_FAILED;                                         \
    } else {                                                                    \
        oasys::logf("/test", oasys::LOG_DEBUG,                                  \
                    "CHECK '" #a "' (%s) == '" #b "' (%s)", (a), (b));          \
    } } while(0)


/// @}

}; // namespace oasys

#endif //__UNIT_TEST_H__
