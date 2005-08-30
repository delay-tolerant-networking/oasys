#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <io/IO.h>
#include <debug/DebugUtils.h>
#include <thread/Thread.h>

using namespace oasys;

class MyThread : public Thread {
public:
    MyThread() : Thread(DELETE_ON_EXIT | CREATE_JOINABLE | INTERRUPTABLE) {}
protected:
    void run() {
        int  fds[2];
        char buf[256];

        log_info("/test/thread", "starting");

        if (pipe(fds) < 0) {
            log_info("/test/thread", "can't create pipe! %s", strerror(errno));
            return;
        }

        for (int i=0; ; ++i) {
            if (should_stop()) {
                log_info("/test/thread", "stopping");
                return;
            } 

            // read on the first, timeout on the second time
            if (i == 0) {
                log_info("/test/thread", "sleeping like the dead - interrupt");
                int err = IO::read(fds[0], buf, 256, "/test/thread", false);
                ASSERT(err == IOERROR && errno == EINTR);
                log_info("/test/thread", "interrupted out of read");
            } else if (i == 1) {
                log_info("/test/thread", "sleeping with timeout - don't interrupt");
                int err = IO::timeout_read(fds[0], buf, 256, 3000, 
                                           "/test/thread", true);
                ASSERT(err == IOTIMEOUT);
                log_info("/test/thread", "timed out of sleeping");
            }
        }

    }
};
    
int
main()
{
    Log::init(LOG_DEBUG);

    log_info("/test/main", "starting test thread...");
    MyThread* t = new MyThread();
    t->start();
    sleep(1);

    log_info("/test/main", "should interrupt");
    t->interrupt();
    sleep(1);

    log_info("/test/main", "shouldn't interrupt");
    t->interrupt();
    sleep(1);

    log_info("/test/main", "interrupting and stopping test thread...");
    t->set_should_stop();
    t->interrupt();

    log_info("/test/main", "waiting for test thread...");
    t->join();

    ASSERT(t->is_stopped());
    log_info("/test/main", "test complete...");
}
