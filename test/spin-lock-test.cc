#include <cstdio>
#include <sys/time.h>

#include "debug/Log.h"
#include <thread/SpinLock.h>
#include <thread/Thread.h>

#ifndef __NO_ATOMIC__

using namespace oasys;

volatile int nspins = 10000000;
volatile int total  = 0;
volatile int count1 = 0;
volatile int count2 = 0;

class Thread1 : public Thread {
public:
    Thread1(SpinLock* l) : Thread(CREATE_JOINABLE), lock_(l) {}
    
protected:
    virtual void run() {
        for (int i = 0; i < nspins; ++i) {
            if (lock_)
                lock_->lock();
            ++count1;
            ++total;
            if (lock_)
                lock_->unlock();
        }
    }

    SpinLock* lock_;
};

class Thread2 : public Thread {
public:
    Thread2(SpinLock* l) : Thread(CREATE_JOINABLE), lock_(l) {}
    
protected:
    virtual void run() {
        for (int i = 0; i < nspins; ++i) {
            if (lock_)
                lock_->lock();
            
            ++count2;
            ++total;
            if (lock_)
                lock_->unlock();
        }
    }

    SpinLock* lock_;
};

void
test(const char* what, SpinLock* lock1, SpinLock* lock2)
{
    log_info("/test", "***** testing with %s lock...", what);
    Thread* t1 = new Thread1(lock1);
    Thread* t2 = new Thread2(lock2);

    t1->start();
    t2->start();

    while (count1 != nspins && count2 != nspins) {
        log_info("/test", "count1:     %d", count1);
        log_info("/test", "count2:     %d", count2);
        sleep(1);
    }

    t1->join();
    t2->join();

    log_info("/log", "total spins: %d",  SpinLock::total_spins_);
    log_info("/log", "total yields: %d", SpinLock::total_yields_);

    log_info("/test", "count1:     %d", count1);
    log_info("/test", "count2:     %d", count2);
    log_info("/test", "total:      %d", total);
    log_info("/test", "count sum:  %d", count1 + count2);

    log_info("/test", "***** testing with %s lock... done.\n\n", what);
    delete t1;
    delete t2;

    count1 = 0;
    count2 = 0;
    total = 0;
    SpinLock::total_spins_ = 0;
    SpinLock::total_yields_ = 0;
}

int
main()
{
    Log::init(LOG_DEBUG);
    SpinLock lock;
    SpinLock lock2;

    test("shared",   &lock, &lock);
    test("separate", &lock, &lock2);
    test("no", 0, 0);
}

#else // __NO_ATOMIC__

int
main()
{
    fprintf(stderr, "spin lock test is meaningless without an atomic.h");
}

#endif // __NO_ATOMIC__

