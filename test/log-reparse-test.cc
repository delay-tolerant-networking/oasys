
#include <unistd.h>
#include <sys/stat.h>
#include "debug/Log.h"
#include "io/FileIOClient.h"
#include "thread/Thread.h"
#include "thread/Timer.h"
#include "util/StringBuffer.h"

using namespace oasys;

class LoggerThread : public Thread {
    virtual void run () {
        int i;
        while (1) {
            if ((i % 1000) == 0) {
                log_info("/test/1000", "test 1000 (%d)", i / 1000);
            }
            
            log_debug("/test", "test %d", ++i);
        }
    }
};

int
main(int argc, const char** argv)
{
    Log::init(LOG_WARN);
    Log* log = Log::instance();

    // first create two files, one with the test rule enabled, one without
    StringBuffer path1("/tmp/log-reparse-test-%s-1", getenv("USER"));
    StringBuffer path2("/tmp/log-reparse-test-%s-2", getenv("USER"));
    
    FileIOClient* f1 = new FileIOClient();
    FileIOClient* f2 = new FileIOClient();

    f1->logpathf("/log/file1");
    f2->logpathf("/log/file2");

    f1->open(path1.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    f2->open(path2.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    
    const char* debug1 = "/log info\n/test debug\n/test/1000 info\n";
    const char* debug2 = "/log info\n/test/1000 info\n";
    f1->write(debug1, strlen(debug1));
    f2->write(debug2, strlen(debug2));

    f1->close();
    f2->close();

    // set up the reparse signal handler
    TimerSystem::init();
    log->add_reparse_handler(SIGHUP);

    // start up the thread, nothing should output (yet)
    LoggerThread* t = new LoggerThread();
    t->start();

    // parse the first one, now we start getting output
    log->parse_debug_file(path1.c_str());
    sleep(1);

    // reparse the same one using a signal
    kill(getpid(), SIGHUP);
    sleep(1);
        
    // now parse the other one, output should cease
    log->parse_debug_file(path2.c_str());
    sleep(1);
    
    // reparse the same one using a signal
    kill(getpid(), SIGHUP);
    sleep(1);

    // try adding a rule explicitly.. we should get output again
    log_info("/log", "adding explicit debug rule");
    log->add_debug_rule("/test", LOG_DEBUG);
    sleep(1);
    
    // all done!

    f1->unlink();
    f2->unlink();
}
