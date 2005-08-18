#include <unistd.h>
#include <debug/DebugUtils.h>
#include <thread/Thread.h>

using namespace oasys;

class MyThread : public Thread {
public:
    MyThread() : Thread(DELETE_ON_EXIT | CREATE_JOINABLE | INTERRUPTABLE) {}
protected:
    void run() {
        log_info("/test/thread", "Thread running... sleeping");
        while(1) {
            int c = fgetc(stdin);
            if (should_stop()) {
                log_info("/test/thread", "Thread woke up from fgetc, stopping. c= %d", c);
                return;
            } else {
                log_info("/test/thread", "Thread woke up from fgetc, should_stop not set. c = %d", c);
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

    
    log_info("/test/main", "interrupting test thread...");
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
