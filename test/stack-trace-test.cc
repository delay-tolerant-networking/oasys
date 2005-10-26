
#include <signal.h>
#include <sys/resource.h>

#include "debug/FatalSignals.h"
#include "util/UnitTest.h"

using namespace oasys;

bool
fork_to_die(const char* how) {
    char cmd[1024];
    log_always("/test", "flamebox-ignore ign1 .*got fatal %s - will dump core",
               how);
    log_always("/test", "flamebox-ignore ign2 STACK TRACE");
    
    snprintf(cmd, sizeof(cmd),
             "test/stack-trace-test %s 2>&1 | "
             "test-utils/expand-stacktrace.pl -o test/stack-trace-test",
             how);
    int ok = system(cmd);
    
    log_always("/test", "flamebox-ignore-cancel ign1");
    log_always("/test", "flamebox-ignore-cancel ign2");;

    return (ok == 0);
}

void
ill_function()
{
    int a = 2;
    int b = 3;
    int c = 4;
    int d = a + b + c;
    d = d * 2;
}

typedef void (*ill_function_t)();

void
die(const char* how)
{
    struct rlimit lim;
    lim.rlim_cur = 0;
    lim.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &lim);
    
    if (!strcmp(how, "SIGSEGV")) {
        int *ptr = 0;
        int a = *ptr;
        (void)a;
    }

    if (!strcmp(how, "SIGBUS")) {
#if defined(__i386__)
        kill(getpid(), SIGBUS);
        exit(1);
#else
        char* p = (char*)malloc(10);
        int* ip = (int*)(p+1);
        int a = *ip;
        (void)a;
#endif
    }

    if (!strcmp(how, "SIGILL")) {
        void* p = malloc(1024);
        ill_function_t f = (ill_function_t)p;
        f();
    }
    
    if (!strcmp(how, "SIGFPE")) {
        int a = 3;
        int b = 0;
        int x = a / b;
        (void)x;
    }

    if (!strcmp(how, "SIGABRT")) {
        kill(getpid(), SIGABRT);
    }
    
    if (!strcmp(how, "SIGQUIT")) {
        kill(getpid(), SIGQUIT);
    }

    if (!strcmp(how, "PANIC")) {
        PANIC("panic");
    }

    fprintf(stdout, "error: shouldn't get here\n");
    exit(1);
}

void a(const char* how);
void b(const char* how);
void c(const char* how);
void d(const char* how);
void e(const char* how);

void a(const char* how) { b(how); }
void b(const char* how) { c(how); }
void c(const char* how) { d(how); }
void d(const char* how) { e(how); }
void e(const char* how) { die(how); }


#define SIGTESTER(sig)                          \
DECLARE_TEST(sig##Test) {                       \
    CHECK(fork_to_die(#sig));                   \
    return UNIT_TEST_PASSED;                    \
}

SIGTESTER(SIGSEGV);
SIGTESTER(SIGBUS);
SIGTESTER(SIGILL);
SIGTESTER(SIGFPE);
SIGTESTER(SIGABRT);
SIGTESTER(SIGQUIT);
SIGTESTER(PANIC);

DECLARE_TESTER(Tester) {
    ADD_TEST(SIGSEGVTest);
    ADD_TEST(SIGBUSTest);
    ADD_TEST(SIGILLTest);
    ADD_TEST(SIGFPETest);
    ADD_TEST(SIGABRTTest);
    ADD_TEST(SIGQUITTest);
    ADD_TEST(PANICTest);
}

int
main(int argc, const char** argv)
{
    // the various test cases above simply re-run the test script
    // passing an argument
    if (argc == 2) {
        oasys::Log::init(oasys::LOG_INFO);
        oasys::FatalSignals::init(argv[0]);
        a(argv[1]);
    }

    Tester t("stack-trace-test");
    return t.run_tests(argc, argv);
}
    

