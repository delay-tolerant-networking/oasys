// XXX/demmer add copyright
#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "Atomic.h"
#include "Lock.h"
#include "Thread.h"

/**
 * If we know there are no atomic instructions for the architecture,
 * just use a Mutex.
 */
#ifdef __NO_ATOMIC__
#include "Mutex.h"
class SpinLock : public Mutex {
public:
    SpinLock() : Mutex("spinlock", TYPE_RECURSIVE, true) {}
};
#else

/**
 * A SpinLock is a Lock that busy waits to get a lock. The
 * implementation supports recursive locking.
 */
class SpinLock : public Lock {
public:

public:
    SpinLock() : Lock(), lock_count_(0) {}
    virtual ~SpinLock() {}

    /// @{
    /// Virtual override from Lock
    int lock();
    int unlock();
    int try_lock();
    /// @}
    
private:
    unsigned int lock_count_; ///< count for recursive locking
};

#endif /* __NO_ATOMIC__ */

#endif /* _SPINLOCK_H_ */
