
#include "debug/Formatter.h"
#include "debug/Log.h"

using namespace oasys;

#define SHOULD(x)    if (!(x)) PANIC("should have printed")
#define SHOULDNOT(x) if ( (x)) PANIC("should not have printed");

class FormatterTest : public Formatter {
public:
    virtual int format(char* buf, size_t sz);
};

int
FormatterTest::format(char* buf, size_t sz)
{
    int x = 100;
    char* s = "fox";
    double d = 0.5;
    int ret = snprintf(buf, sz, "my own %d %s %g", x, s, d);
    return ret;
}

class BoundsTest : public Formatter {
public:
    virtual int format(char* buf, size_t sz);
};

int
BoundsTest::format(char* buf, size_t sz)
{
    int n = sz;
    for (int i = 0; i < n; ++i) {
        buf[i] = (i % 10) + '0';
    }
    return n;
}

class TruncateTest : public Formatter {
public:
    virtual int format(char* buf, size_t sz);
};

int
TruncateTest::format(char* buf, size_t sz)
{
    int n = sz;
    for (int i = 0; i < n; ++i) {
        buf[i] = (i % 10) + '0';
    }
    return n + 10; // pretend it truncated
}

class OverflowTest : public Formatter {
public:
    virtual int format(char* buf, size_t sz);
};

int
OverflowTest::format(char* buf, size_t sz)
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
};

class MultiFormatter : public SomethingVirtual, public Formatter, public Logger {
public:
    int format(char* buf, size_t sz) {
        return snprintf(buf, sz, "i'm a multiformatter %p logpath %p",
                        this, &logpath_);
    }
    
};

class LoggerTest : public Logger {
public:
    LoggerTest() : Logger("/test/loggertest") {}
    void foo();
};

void
LoggerTest::foo()
{
    // test macro call with implicit path
    SHOULD(log_debug("debug message %d", 10));

    // and non-macro call
    SHOULD(logf(LOG_DEBUG, "debug message %d", 10));

    // and with a const char* param
    SHOULD(log_debug("debug %s %d", "message", 10));
    SHOULD(logf(LOG_DEBUG, "debug %s %d", "message", 10));

    // test macro calls with explicit path
    SHOULD(__log_debug("/test/other/path", "debug message %d", 10));
    SHOULD(__log_debug("/test/other/path", "debug %s %d", "message", 10));

    // and non-macro calls
    SHOULD(logf("/test/other/path", LOG_DEBUG, "debug message %d", 10));
    SHOULD(logf("/test/other/path", LOG_DEBUG, "debug %s %d", "message", 10));
}

void
foo()
{
    // test macro calls with explicit path in a non-logger function
    SHOULD(log_debug("/test/path", "debug message %d", 10));
    SHOULD(log_debug("/test/path", "debug %s %d", "message", 10));

    // and non-macro calls
    SHOULD(logf("/test/path", LOG_DEBUG, "debug message %d", 10));
    SHOULD(logf("/test/path", LOG_DEBUG, "debug %s %d", "message", 10));
}

void
shouldprint()
{
    // these should all print
    SHOULD(logf("/test", LOG_DEBUG, "print me"));
    SHOULD(logf("/test", LOG_INFO,  "print me"));
    SHOULD(logf("/test", LOG_WARN,  "print me"));
    SHOULD(logf("/test", LOG_ERR,   "print me"));
    SHOULD(logf("/test", LOG_CRIT,  "print me"));

    SHOULD(log_debug("/test", "print me"));
    SHOULD(log_info("/test",  "print me"));
    SHOULD(log_warn("/test", "print me"));
    SHOULD(log_err("/test", "print me"));
    SHOULD(log_crit("/test", "print me"));
}

void
shouldntprint()
{
    SHOULDNOT(logf("/test/disabled", LOG_DEBUG, "don't print me"));
    SHOULDNOT(logf("/test/disabled", LOG_INFO,  "don't print me"));
    SHOULDNOT(logf("/test/disabled", LOG_WARN,  "don't print me"));
    SHOULDNOT(logf("/test/disabled", LOG_ERR,   "don't print me"));

    SHOULD(logf("/test/disabled", LOG_CRIT, "but print me!!"));

    SHOULDNOT(log_debug("/test/disabled", "don't print me"));
    SHOULDNOT(log_info("/test/disabled",  "don't print me"));
    SHOULDNOT(log_warn("/test/disabled",  "don't print me"));
    SHOULDNOT(log_err("/test/disabled",   "don't print me"));
    
    SHOULD(log_crit("/test/disabled",  "but print me!!"));
}

void
multiline()
{
    SHOULD(log_multiline("/test/multiline", LOG_DEBUG,
                         "print me\n"
                         "and me\n"
                         "and me\n"));
    
    SHOULDNOT(log_multiline("/test/disabled", LOG_DEBUG,
                            "not me\n"
                            "nor me\n"
                            "nor me\n"));
}

int
main(int argc, const char** argv)
{
    Log::init(LOG_ERR);
    
    Log::instance()->add_debug_rule("/test", LOG_DEBUG);
    Log::instance()->add_debug_rule("/test/ignore", LOG_CRIT);
    Log::instance()->add_debug_rule("/test/disabled", LOG_CRIT);

    FormatterTest fmt;
    logf("/test/formatter", LOG_DEBUG, "formatter: %p *%p", &fmt, &fmt);
    logf("/test/formatter", LOG_DEBUG, "%p pointer works at beginning", &fmt);
    logf("/test/formatter", LOG_DEBUG, "*%p pointer works at beginning too", &fmt);

    MultiFormatter mft;
    logf("/test/multi", LOG_DEBUG, "multiformatter: address is %p *%p",
         &mft, (Formatter*)&mft);

    LoggerTest test;
    test.foo();

    foo();
    shouldprint();
    shouldntprint();
    multiline();

    BoundsTest bft;
    logf("/test/bounfs", LOG_DEBUG, "bounds test: *%p *%p", &fmt, &bft);

    TruncateTest tft;
    logf("/test/truncate", LOG_DEBUG, "truncate: *%p", &tft);

    logf("/test/overflow", LOG_DEBUG, "EXPECT A PANIC:");
    OverflowTest oft;
    logf("/test/overflow", LOG_DEBUG, "overflow: *%p", &oft);

    exit(0);
}
