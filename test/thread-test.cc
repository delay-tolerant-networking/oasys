#include <unistd.h>
#include <debug/DebugUtils.h>
#include <thread/Thread.h>

#define THREADS_TO_CREATE 10

using namespace oasys;

class MyThread : public Thread {
public:
    MyThread() : Thread("MyThread", DELETE_ON_EXIT) {}
protected:
    void run() {}
};

int
main()
{
    Log::init(LOG_DEBUG);

    while(true) {
	for(int i=0; i<THREADS_TO_CREATE; ++i) {
	    MyThread* t = new MyThread();
	    t->start();
	}
	::sleep(1);
    }
}
