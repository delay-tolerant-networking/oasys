#include <unistd.h>
#include <debug/Debug.h>
#include <thread/Thread.h>

using namespace oasys;

class MyThread : public Thread {
public:
    MyThread() : Thread(DELETE_ON_EXIT | CREATE_JOINABLE | INTERRUPTABLE) {}
protected:
    void run() {
        log_info("/test/thread", "Thread running... sleeping");
        while(1) {
            sleep(10000);
            if (should_stop()) {
                log_info("/test/thread", "Thread woke up from sleep, stopping");
                return;
            } else {
                log_info("/test/thread", "Thread woke up from sleep, should_stop not set");
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