
#include "SpinLock.h"
#include "debug/Debug.h"

#ifndef NO_ATOMIC

int
SpinLock::lock()
{
    pthread_t me = Thread::current();
    int nspins;
    
    if (lock_holder_ == me) {
        lock_count_++;
        return 0;
    }
    
    nspins = 0;
    while (atomic_cmpxchg32(&lock_holder_, 0, (unsigned int)me) != 0)
    {
        Thread::spin_yield();
#ifndef NDEBUG
        if (++nspins > 1000000) {
            PANIC("SpinLock reached spin limit");
        }
#endif
    }

    ASSERT(lock_count_ == 0);
    lock_count_ = 1;
    return 0;
};

int
SpinLock::unlock() {
    ASSERT(lock_holder_ == Thread::current());
    ASSERT(lock_count_ > 0);

    lock_count_--;
    if (lock_count_ == 0) {
        lock_holder_ = 0;
    }
    
    return 0;
};
 
int
SpinLock::try_lock()
{
    pthread_t me = Thread::current();
    int lock_holder = atomic_cmpxchg32(&lock_holder_, 0, (unsigned int)me);

    if (lock_holder == 0) {
        ASSERT(lock_count_ == 0);
        lock_count_++;
        return 0; // success
    } else {
        return 1; // already locked
    }
};

#endif
