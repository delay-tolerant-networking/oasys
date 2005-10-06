#include <cstdio>
#include <sys/time.h>

#include "debug/Log.h"
#include <thread/SpinLock.h>
#include <thread/Thread.h>
#include <util/UnitTest.h>

#ifndef __NO_ATOMIC__

using namespace oasys;

SpinLock lock;
SpinLock lock2;
volatile int nspins = 10000000;
volatile int total  = 0;
volatile int count1 = 0;
volatile int count2 = 0;

class Thread1 : public Thread {
public:
    Thread1(SpinLock* l) : Thread("Thread1", CREATE_JOINABLE), lock_(l) {}
    
protected:
    virtual void run() {
        for (int i = 0; i < nspins; ++i) {
            if (lock_)
                lock_->lock("Thread1");
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
    Thread2(SpinLock* l) : Thread("Thread2", CREATE_JOINABLE), lock_(l) {}
    
protected:
    virtual void run() {
        for (int i = 0; i < nspins; ++i) {
            if (lock_)
                lock_->lock("Thread2");
            
            ++count2;
            ++total;
            if (lock_)
                lock_->unlock();
        }
    }

    SpinLock* lock_;
};

int
test(const char* what, SpinLock* lock1, SpinLock* lock2)
{
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
    delete t1;
    delete t2;

    count1 = 0;
    count2 = 0;
    total = 0;
    SpinLock::total_spins_ = 0;
    SpinLock::total_yields_ = 0;

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(SharedLock) {
    return test("shared", &lock, &lock);
}

DECLARE_TEST(SeparateLock) {
    return test("shared", &lock, &lock2);
}
    
DECLARE_TEST(NoLock) {
    return test("shared", &lock, &lock2);
}

DECLARE_TESTER(SpinLockTester) {
    ADD_TEST(SharedLock);
    ADD_TEST(SeparateLock);
    ADD_TEST(NoLock);
}

#else // __NO_ATOMIC__

DECLARE_TEST(Bogus) {
    log_warn("spin lock test is meaningless without atomic.h");
    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(SpinLockTester) {
    ADD_TEST(Bogus);
}

#endif // __NO_ATOMIC__

DECLARE_TEST_FILE(SpinLockTester, "spin lock test");
