#include <iostream>
#include <cstdlib>

#include <thread/Thread.h>
#include <thread/MsgQueue.h>

using namespace std;

using namespace oasys;

class Consumer : public Thread {
public:
    Consumer(MsgQueue<int>* q) : q_(q) {}
protected:
    virtual void run() {
        int prev = -1;

        while(true) {
            //logf("/test", LOG_INFO, "consumer dequeue");
            int curr = q_->pop_blocking();

            ASSERT(curr - 1 == prev);
            
            logf("/test", LOG_INFO, "consumer dequeue %d", curr);
            prev = curr;
            // yield();
        }
    }
    
    MsgQueue<int>* q_;
};

class Producer : public Thread {
public:
    Producer(MsgQueue<int>* q) : q_(q) {}
protected:
    virtual void run() {
        int curr = 0;

        while(true) {
            logf("/test", LOG_INFO, "producer enqueue %d", curr);
            q_->push(curr);
            curr++;
        }
    }
    
    MsgQueue<int>* q_;
};

int
main(int argc, char* argv[])
{
    Log::init(LOG_INFO);
    MsgQueue<int> q;

    Consumer c(&q);
    Producer p(&q);

    c.start();
    p.start();
    
    while(true)
        Thread::yield();
}
