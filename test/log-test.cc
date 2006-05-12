
#include <sys/stat.h>

#include "debug/Formatter.h"
#include "debug/Log.h"
#include "io/FileIOClient.h"
#include "util/UnitTest.h"
#include "util/StringBuffer.h"
#include "thread/Thread.h"

using namespace oasys;

StringBuffer path1, path2;
FileIOClient *f1, *f2;

const char* debug1 =
"/log-test          debug\n"
"/log-test          info\n"
"/log-test/disabled crit\n"
"/log-test/thread   info\n";
    
const char* debug1_sorted = 
"/log-test/disabled critical\n"
"/log-test/thread info\n"
"/log-test debug\n"
"/log-test info\n";
    
const char* debug2 =
"/log-test          debug\n"
"/log-test/disabled crit\n"
"/log-test/thread   warn\n";

const char* debug2_sorted = 
"/log-test/disabled critical\n"
"/log-test/thread warning\n"
"/log-test debug\n";

DECLARE_TEST(Init) {
    
    log_notice("/test", "flamebox-ignore logs /log-test");

    // create two files, one with the test rule enabled, one without
    path1.appendf("/tmp/log-test-%s-1-%d", getenv("USER"), getpid());
    path2.appendf("/tmp/log-test-%s-2-%d", getenv("USER"), getpid());
    
    f1 = new FileIOClient();
    f2 = new FileIOClient();

    f1->logpathf("/log/file1");
    f2->logpathf("/log/file2");
    
    CHECK(f1->open(path1.c_str(),
                   O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR) > 0);

    CHECK(f2->open(path2.c_str(),
                   O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR) > 0);

    CHECK(f1->write(debug1, strlen(debug1)) == (int)strlen(debug1));
    CHECK(f2->write(debug2, strlen(debug2)) == (int)strlen(debug2));
    
    CHECK(f1->close() == 0);
    CHECK(f2->close() == 0);

    // parse debug1
    Log::instance()->parse_debug_file(path1.c_str());
    StringBuffer rules;
    Log::instance()->dump_rules(&rules);
    CHECK_EQUALSTR(rules.c_str(), debug1_sorted);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(RulesTest) {
    
    // these should all print
    CHECK(logf("/log-test", LOG_DEBUG, "print me") != 0);
    CHECK(logf("/log-test", LOG_INFO,  "print me") != 0);
    CHECK(logf("/log-test", LOG_WARN,  "print me") != 0);
    CHECK(logf("/log-test", LOG_ERR,   "print me") != 0);
    CHECK(logf("/log-test", LOG_CRIT,  "print me") != 0);

    CHECK(log_debug("/log-test", "print me") != 0);
    CHECK(log_info("/log-test",  "print me") != 0);
    CHECK(log_warn("/log-test", "print me") != 0);
    CHECK(log_err("/log-test", "print me") != 0);
    CHECK(log_crit("/log-test", "print me") != 0);

    CHECK(logf("/log-test/disabled", LOG_CRIT, "print me") != 0);

    CHECK(logf("/log-test/disabled", LOG_DEBUG, "don't print me") == 0);
    CHECK(logf("/log-test/disabled", LOG_INFO,  "don't print me") == 0);
    CHECK(logf("/log-test/disabled", LOG_WARN,  "don't print me") == 0);
    CHECK(logf("/log-test/disabled", LOG_ERR,   "don't print me") == 0);

    CHECK(log_debug("/log-test/disabled", "don't print me") == 0);
    CHECK(log_info("/log-test/disabled",  "don't print me") == 0);
    CHECK(log_warn("/log-test/disabled",  "don't print me") == 0);
    CHECK(log_err("/log-test/disabled",   "don't print me") == 0);
    
    CHECK(log_crit("/log-test/disabled",  "but print me!!") != 0);

    CHECK(log_multiline("/log-test/multiline", LOG_DEBUG,
                         "print me\n"
                         "and me\n"
                         "and me\n") != 0);
    
    CHECK(log_multiline("/log-test/disabled", LOG_DEBUG,
                        "not me\n"
                        "nor me\n"
                        "nor me\n") == 0);

    return UNIT_TEST_PASSED;
}

class LoggerTest : public Logger {
public:
    LoggerTest() : Logger("LoggerTest", "/log-test/loggertest") {}
    int foo();
};

int
LoggerTest::foo() {
    // test macro call with implicit path
    CHECK(log_debug("debug message %d", 10) != 0);
        
    // and non-macro call
    CHECK(logf(LOG_DEBUG, "debug message %d", 10) != 0);

    // and with a const char* param
    CHECK(log_debug("debug %s %d", "message", 10) != 0);
    CHECK(logf(LOG_DEBUG, "debug %s %d", "message", 10) != 0);

    // test macro calls with explicit path
    CHECK(__log_debug("/log-test/other/path",
                      "debug message %d", 10) != 0);
    CHECK(__log_debug("/log-test/other/path",
                      "debug %s %d", "message", 10) != 0);

    // and non-macro calls
    CHECK(logf("/log-test/other/path", LOG_DEBUG,
               "debug message %d", 10) != 0);
    CHECK(logf("/log-test/other/path", LOG_DEBUG,
               "debug %s %d", "message", 10) != 0);

    return UNIT_TEST_PASSED;
}

int
foo()
{
    // test macro calls with explicit path in a non-logger function
    CHECK(log_debug("/log-test/path", "debug message %d", 10) != 0);
    CHECK(log_debug("/log-test/path", "debug %s %d", "message", 10) != 0);

    // and non-macro calls
    CHECK(logf("/log-test/path", LOG_DEBUG,
               "debug message %d", 10) != 0);
    CHECK(logf("/log-test/path", LOG_DEBUG,
               "debug %s %d", "message", 10) != 0);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(LoggerTest) {
    LoggerTest test;
    CHECK(test.foo() == UNIT_TEST_PASSED);
    CHECK(foo() == UNIT_TEST_PASSED);
    return UNIT_TEST_PASSED;
}

class FormatterTest : public Formatter {
public:
    virtual int format(char* buf, size_t sz) const;
};

int
FormatterTest::format(char* buf, size_t sz) const
{
    int x = 100;
    char* s = "fox";
    double d = 0.5;
    int ret = snprintf(buf, sz, "my own %d %s %g", x, s, d);
    return ret;
}

class BoundsTest : public Formatter {
public:
    BoundsTest(int overflow = 0) : overflow_(overflow) {}
    virtual int format(char* buf, size_t sz) const;
    int overflow_;
};

int
BoundsTest::format(char* buf, size_t sz) const
{
    int n = sz;
    for (int i = 0; i < n; ++i) {
        buf[i] = (i % 10) + '0';
    }
    return n + overflow_;
}

class RecursiveTest : public Formatter {
public:
    RecursiveTest(int level) : level_(level) {}
    virtual int format(char* buf, size_t sz) const;

    int level_;
};

int
RecursiveTest::format(char* buf, size_t sz) const
{
    RecursiveTest r(level_ + 1);
    return snprintf(buf, sz, "%d *%p", level_, &r);
}

class RecursiveBoundsTest : public Formatter {
public:
    virtual int format(char* buf, size_t sz) const;
};

int
RecursiveBoundsTest::format(char* buf, size_t sz) const
{
    BoundsTest b(10);
    return snprintf(buf, sz, "recurse: *%p", &b);
}

class TruncateTest : public Formatter {
public:
    virtual int format(char* buf, size_t sz) const;
};

int
TruncateTest::format(char* buf, size_t sz) const
{
    int n = sz;
    for (int i = 0; i < n; ++i) {
        buf[i] = (i % 10) + '0';
    }
    return n + 10; // pretend it truncated
}

class OverflowTest : public Formatter {
public:
    virtual int format(char* buf, size_t sz) const;
};

int
OverflowTest::format(char* buf, size_t sz) const
{
    int n = sz + 10; // danger!
    for (int i = 0; i < n; ++i) {
        buf[i] = (i % 10) + '0';
    }
    return n;
}

class SomethingVirtual {
public:
    virtual void foo() {
        logf("/foo", LOG_INFO, "foo");
    }

    virtual ~SomethingVirtual() {}
};

class MultiFormatter : public SomethingVirtual,
                       public Formatter,
                       public Logger
{
public:
    MultiFormatter() : Logger("MultiFormatter", "/test/multiformatter") {}
    
    int format(char* buf, size_t sz) const {
        return snprintf(buf, sz, "i'm a multiformatter %p logpath %p",
                        this, &logpath_);
    }
    
};

DECLARE_TEST(FormatterTest) {
    FormatterTest fmt;
    CHECK(logf("/log-test/formatter", LOG_DEBUG,
               "formatter: %p *%p", &fmt, &fmt) != 0);

    CHECK(logf("/log-test/formatter", LOG_DEBUG,
               "%p pointer works at beginning", &fmt) != 0);

    CHECK(logf("/log-test/formatter", LOG_DEBUG,
               "*%p pointer works at beginning too", &fmt) != 0);

    MultiFormatter mft;

    CHECK(logf("/log-test/multi", LOG_DEBUG,
               "multiformatter: address is %p *%p",
               &mft, (Formatter*)&mft) != 0);

    BoundsTest bft;
    CHECK(logf("/log-test/bounds", LOG_DEBUG,
               "bounds test: *%p *%p", &fmt, &bft) != 0);

    BoundsTest bft2(10);
    CHECK(logf("/log-test/bounds2", LOG_DEBUG,
               "bounds test: *%p *%p", &fmt, &bft2) != 0);

    RecursiveTest rft(0);
    CHECK(logf("/log-test/recursive", LOG_DEBUG,
               "recursive test: *%p", &rft) != 0);

    RecursiveBoundsTest rbt;
    CHECK(logf("/log-test/recursive_bounds", LOG_DEBUG,
               "recursive bounds test: *%p", &rbt) != 0);

    TruncateTest tft;
    CHECK(logf("/log-test/truncate", LOG_DEBUG,
               "truncate: *%p", &tft) != 0);

    OverflowTest oft;
    Log::__debug_no_panic_on_overflow = true;
    CHECK(logf("/log-test/overflow", LOG_DEBUG, "overflow: *%p", &oft) == -1);

    return UNIT_TEST_PASSED;
}

class LoggerThread : public Thread, public Logger {
public:
    LoggerThread(int threadid)
        : Thread("LoggerThread", CREATE_JOINABLE),
          Logger("LoggerThread", "/test/logger-thread")
    {
        logpathf("/log-test/thread/%d", threadid);
    }
    
    virtual void run () {
        output_ = false;

        int i = 0;
        while (1) {
            ++i;

            if (stop_) {
                return;
            }

            if (output_) {
                log_info("loops: %d", i / 1000);
                output_ = false;
            }
            
            log_debug("never output");

            ++i;
        }
    }

    volatile bool output_;
    static bool stop_;
};

bool LoggerThread::stop_ = false;

DECLARE_TEST(ReparseTest) {
    LoggerThread t1(1), t2(2), t3(3), t4(4);

    t1.start();
    t2.start();
    t3.start();
    t4.start();

    CHECK(__log_enabled(LOG_INFO, "/log-test/thread"));
    t1.output_ = t2.output_ = t3.output_ = t3.output_ = true;
    sleep(2);

    {
        Log::instance()->parse_debug_file(path2.c_str());
        StringBuffer rules;
        Log::instance()->dump_rules(&rules);
        CHECK_EQUALSTR(rules.c_str(), debug2_sorted);
        CHECK(! __log_enabled(LOG_INFO, "/log-test/thread"));
    }
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    sleep(2);
    
    Log::instance()->parse_debug_file(path1.c_str());
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    while (t1.output_ || t2.output_ || t3.output_ || t4.output_) {
        Thread::yield();
    }
    
    Log::instance()->parse_debug_file(path2.c_str());
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    while (t1.output_ || t2.output_ || t3.output_ || t4.output_) {
        Thread::yield();
    }
    
    Log::instance()->parse_debug_file(path1.c_str());
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    while (t1.output_ || t2.output_ || t3.output_ || t4.output_) {
        Thread::yield();
    }
    
    Log::instance()->parse_debug_file(path2.c_str());
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    while (t1.output_ || t2.output_ || t3.output_ || t4.output_) {
        Thread::yield();
    }
    Log::instance()->parse_debug_file(path1.c_str());
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    while (t1.output_ || t2.output_ || t3.output_ || t4.output_) {
        Thread::yield();
    }
    
    Log::instance()->parse_debug_file(path2.c_str());
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    while (t1.output_ || t2.output_ || t3.output_ || t4.output_) {
        Thread::yield();
    }
    Log::instance()->parse_debug_file(path1.c_str());
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    while (t1.output_ || t2.output_ || t3.output_ || t4.output_) {
        Thread::yield();
    }
    
    Log::instance()->parse_debug_file(path2.c_str());
    t1.output_ = t2.output_ = t3.output_ = t4.output_ = true;
    while (t1.output_ || t2.output_ || t3.output_ || t4.output_) {
        Thread::yield();
    }

    LoggerThread::stop_ = true;
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Fini) {
    CHECK(f1->unlink() == 0);
    CHECK(f2->unlink() == 0);

    log_notice("/test", "flamebox-ignore-cancel logs");

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(LogTest) {
    ADD_TEST(Init);
    ADD_TEST(RulesTest);
    ADD_TEST(LoggerTest);
    ADD_TEST(FormatterTest);
    ADD_TEST(ReparseTest);
    ADD_TEST(Fini);
}

DECLARE_TEST_FILE(LogTest, "LogTest");

