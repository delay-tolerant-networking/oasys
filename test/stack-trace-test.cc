
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "debug/FatalSignals.h"
#include "util/StringBuffer.h"
#include "util/UnitTest.h"

using namespace oasys;

const char* executable;

char expand_stacktrace[1024];

bool
fork_to_die(const char* how) {
    char cmd[1024];
    log_always("/test", "flamebox-ignore ign1 .*got fatal %s - will dump core",
               how);
    log_always("/test", "flamebox-ignore ign2 STACK TRACE");
    log_always("/test", "flamebox-ignore ign3 fatal handler dumping core");
    
    snprintf(cmd, sizeof(cmd),
             "%s %s 2>&1 | %s -o %s",
             executable, how, expand_stacktrace, executable);
    int ok = system(cmd);
    
    log_always("/test", "flamebox-ignore-cancel ign1");
    log_always("/test", "flamebox-ignore-cancel ign2");;
    log_always("/test", "flamebox-ignore-cancel ign3");;

    return (ok == 0);
}

typedef void (*ill_function_t)();

void
ill_function()
{
    char* p = (char*)malloc(1024);
    memset(p, -1, 1024);
    ill_function_t f = (ill_function_t)p;
    f();
}

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
        printf("a: %d\n", a);
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
        ill_function();
    }
    
    if (!strcmp(how, "SIGFPE")) {
        int a = 3;
        int b = 0;
        int x = a / b;
        printf("x: %d\n", x);
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
    if (access("./stack-trace-test", R_OK | X_OK) != 0) {
        fprintf(stderr, "error: must run this test in the same directory "
                "as its executable\n");
        exit(1);
    }
    
    // Cygwin hack
    StringBuffer g("./%s.exe", argv[0]);
    if (access("./stack-trace-test.exe", R_OK | X_OK) == 0) {
        executable = "./stack-trace-test.exe";
    } else {
        executable = "./stack-trace-test";
    }

    char* test_utils_dir = getenv("OASYS_TEST_UTILS_DIR");
    if (!test_utils_dir) {
        test_utils_dir = "../test-utils";
    }

    sprintf(expand_stacktrace, "%s/expand-stacktrace.pl", test_utils_dir);
    if (access(expand_stacktrace, R_OK | X_OK) != 0) {
        fprintf(stderr, "error: %s does not exist\n", expand_stacktrace);
        exit(1);
    }
    
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
    

