#include <cstdio>
#include <sys/time.h>

#include "debug/Log.h"
#include <thread/Atomic.h>
#include <thread/Thread.h>
#include <util/UnitTest.h>

#ifndef __NO_ATOMIC__

using namespace oasys;

class AddRetThread : public Thread {
public:
    AddRetThread(volatile u_int32_t* barrier, volatile u_int32_t* val,
                 volatile u_int32_t* sum, u_int32_t count, u_int32_t amount)
        : Thread("IncrThread", CREATE_JOINABLE),
          barrier_(barrier), val_(val), sum_(sum), count_(count), amount_(amount) {}
    
protected:
    virtual void run() {
        atomic_incr(barrier_);
        while (*barrier_ != 0) {}

        log_debug("/test", "thread %p starting", this);
        for (u_int i = 0; i < count_; ++i) {
            u_int32_t newval;
            if (amount_ == 1) {
                newval = atomic_incr_ret(val_);
            } else {
                newval = atomic_add_ret(val_, amount_);
            }
            atomic_add(sum_, newval);
        }
        log_debug("/test", "thread %p done", this);
    }

    volatile u_int32_t* barrier_;
    volatile u_int32_t* val_;
    volatile u_int32_t* sum_;
    u_int32_t count_;
    u_int32_t amount_;
};

class IncrThread : public Thread {
public:
    IncrThread(volatile u_int32_t* barrier, volatile u_int32_t* val,
               u_int32_t count, u_int32_t limit = 0)
        : Thread("IncrThread", CREATE_JOINABLE),
          barrier_(barrier), val_(val), count_(count), limit_(limit) {}
    
protected:
    virtual void run() {
        atomic_incr(barrier_);
        while (*barrier_ != 0) {}

        log_debug("/test", "thread %p starting", this);
        for (u_int i = 0; i < count_; ++i) {
            atomic_incr(val_);

            while ((limit_ != 0) && (*val_ > limit_)) {
                
            }
        }
        log_debug("/test", "thread %p done", this);
    }

    volatile u_int32_t* barrier_;
    volatile u_int32_t* val_;
    u_int32_t count_;
    u_int32_t limit_;
};

class DecrTestThread : public Thread {
public:
    DecrTestThread(volatile u_int32_t* barrier, volatile u_int32_t* val,
                   u_int32_t count, u_int32_t* nzero, u_int32_t* maxval)
        : Thread("DecrTestThread", CREATE_JOINABLE),
          barrier_(barrier), val_(val), count_(count), nzero_(nzero), maxval_(maxval) {}
    
protected:
    virtual void run() {
        atomic_incr(barrier_);
        while (*barrier_ != 0) {}
        
        log_debug("/test", "thread %p starting", this);

        for (u_int i = 0; i < count_; ++i) {
            while (! atomic_cmpxchg32(val_, 0, 0)) {} // wait
            
            bool zero = atomic_decr_test(val_);
            if (zero) {
                atomic_incr(nzero_);
            }

            u_int32_t cur = *val_;
            if (cur > *maxval_) {
                *maxval_ = cur;
            }
        }
        
        log_debug("/test", "thread %p done", this);
    }

    volatile u_int32_t* barrier_;
    volatile u_int32_t* val_;
    u_int32_t count_;
    u_int32_t* nzero_;
    u_int32_t* maxval_;
};

class CompareAndSwapThread : public Thread {
public:
    CompareAndSwapThread(volatile u_int32_t* barrier, volatile u_int32_t* val,
                         u_int32_t count,
                         volatile u_int32_t* success, volatile u_int32_t* fail)
        : Thread("DecrTestThread", CREATE_JOINABLE),
          barrier_(barrier), val_(val), count_(count), success_(success), fail_(fail) {}
    
protected:
    virtual void run() {
        atomic_incr(barrier_);
        while (*barrier_ != 0) {}
        
        log_debug("/test", "thread %p starting", this);

        for (u_int i = 0; i < count_; ++i) {
            u_int32_t old = atomic_cmpxchg32(val_, 0, (u_int32_t)this);
            if (old == 0) {
                ASSERT(*val_ == (u_int32_t)this);
                old = atomic_cmpxchg32(val_, (u_int32_t)this, 0);
                ASSERT(old == (u_int32_t)this);
                atomic_incr(success_);
            } else {
                atomic_incr(fail_);
            }
        }
        
        log_debug("/test", "thread %p done", this);
    }

    volatile u_int32_t* barrier_;
    volatile u_int32_t* val_;
    u_int32_t count_;
    volatile u_int32_t* success_;
    volatile u_int32_t* fail_;
};

int
atomic_add_ret_test(int nthreads, int count, int amount)
{
    u_int32_t barrier = 0;
    u_int32_t val = 0;
    u_int32_t sum[nthreads];
    u_int64_t expected = 0;
    u_int64_t total_sum = 0;

    memset(sum, 0, sizeof(sum));

    Thread* threads[nthreads];

    for (int i = 0; i < nthreads; ++i) {
        threads[i] = new AddRetThread(&barrier, &val, &sum[i], count, amount);
        threads[i]->start();
    }

    while (barrier != (u_int)nthreads) {}
    barrier = 0;

    for (int i = 0; i < nthreads; ++i) {
        threads[i]->join();
        delete threads[i];
    }

    for (int i = 0; i < nthreads * count; ++i) {
        expected += ((i + 1) * amount);
    }

    for (int i = 0; i < nthreads; ++i) {
        CHECK_GTU(sum[i], 0);
        total_sum += sum[i];
    }

    CHECK_EQUAL(val, nthreads * count * amount);
    CHECK_EQUAL_U64(total_sum, expected);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(AtomicAddRet2_1) {
    return atomic_add_ret_test(2, 10000, 1);
}

DECLARE_TEST(AtomicAddRet10_1) {
    return atomic_add_ret_test(10, 10000, 1);
}

DECLARE_TEST(AtomicAddRet2_10) {
    return atomic_add_ret_test(2, 10000, 10);
}

DECLARE_TEST(AtomicAddRet10_10) {
    return atomic_add_ret_test(10, 5000, 10);
}

int
atomic_incr_test(int nthreads, int count)
{
    u_int32_t barrier = 0;
    u_int32_t val = 0;
    u_int32_t expected = 0;

    Thread* threads[nthreads];

    for (int i = 0; i < nthreads; ++i) {
        threads[i] = new IncrThread(&barrier, &val, count * (i + 1));
        expected += count * (i + 1);
        threads[i]->start();
    }

    while (barrier != (u_int)nthreads) {}
    barrier = 0;

    for (int i = 0; i < nthreads; ++i) {
        threads[i]->join();
        delete threads[i];
    }

    CHECK_EQUAL(val, expected);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(AtomicInc2) {
    return atomic_incr_test(2, 10000000);
}

DECLARE_TEST(AtomicInc10) {
    return atomic_incr_test(10, 10000000);
}

DECLARE_TEST(AtomicDecrTest) {
    u_int32_t barrier = 0;
    u_int32_t val = 0;
    u_int32_t nzeros = 0;
    u_int32_t maxval = 0;

    int count = 10000000;
                
    IncrThread it(&barrier, &val, count, count / 1000);
    DecrTestThread dt(&barrier, &val, count, &nzeros, &maxval);

    it.start();
    dt.start();

    while (barrier != 2) {}
    barrier = 0;

    it.join();
    dt.join();

    CHECK_EQUAL(val, 0);
    CHECK_GTU(nzeros, 0);
    CHECK_GTU(maxval, 0);

    return UNIT_TEST_PASSED;
}

int
compare_and_swap_test(int nthreads, int count) {
    u_int32_t barrier = 0;
    u_int32_t val = 0;
    
    u_int32_t success[nthreads];
    u_int32_t fail[nthreads];
    Thread* threads[nthreads];

    for (int i = 0; i < nthreads; ++i) {
        success[i] = 0;
        fail[i]    = 0;
        threads[i] = new CompareAndSwapThread(&barrier, &val, count,
                                              &success[i], &fail[i]);
        threads[i]->start();
    }

    while (barrier != (u_int)nthreads) {}
    barrier = 0;

    for (int i = 0; i < nthreads; ++i) {
        threads[i]->join();
        delete threads[i];
    }

    u_int32_t total_success = 0, total_fail = 0;
    for (int i = 0; i < nthreads; ++i) {
        CHECK_GTU(success[i], 0);
        CHECK_GTU(fail[i], 0);

        total_success += success[i];
        total_fail    += fail[i];
    }

    CHECK_EQUAL(total_success + total_fail, count * nthreads);
    CHECK_EQUAL(val, 0);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(CompareAndSwapTest2) {
    return compare_and_swap_test(2, 10000000);
}

DECLARE_TEST(CompareAndSwapTest10) {
    return compare_and_swap_test(10, 10000000);
}

DECLARE_TESTER(AtomicTester) {
    ADD_TEST(AtomicAddRet2_1);
    ADD_TEST(AtomicAddRet10_1);
    ADD_TEST(AtomicAddRet2_10);
    ADD_TEST(AtomicAddRet10_10);
    ADD_TEST(AtomicInc2);
    ADD_TEST(AtomicInc10);
    ADD_TEST(AtomicDecrTest);
    ADD_TEST(CompareAndSwapTest2);
    ADD_TEST(CompareAndSwapTest10);
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

DECLARE_TEST_FILE(AtomicTester, "atomic test");
