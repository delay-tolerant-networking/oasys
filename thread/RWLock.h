#ifndef __RWLOCK_H__
#define __RWLOCK_H__

#include <cstdio>

#include "Lock.h"
#include "SpinLock.h"
#include "Mutex.h"

namespace oasys {

/*!
 * Lock with shared and exclusive semantics.
 */
class RWLock {
public:
    RWLock(Lock* lock)
        : lock_(lock),
          rcount_(0),
          wcount_(0)
    {}

    /*! 
     * Acquire a shared lock. Any number of readers are permitted
     * inside a shared lock.
     */
    void shared_lock() {
        lock_->lock();
        while (wcount_ > 0) {
            lock_->unlock();
            Thread::spin_yield();
            lock_->lock();
        }
        ++rcount_;
        lock_->unlock();
    }

    //! Drop the shared lock.
    void shared_unlock() {
        lock_->lock();
        --rcount_;
        lock_->unlock();
    }

    /*! 
     * Acquire the write lock. Only one writer is permitted to hold
     * the write lock. No readers are allowed inside when the write
     * lock is held.
     */
    void exclusive_lock() {
        lock_->lock();
        while (wcount_ > 0 && rcount_ > 0) {
            lock_->unlock();
            Thread::spin_yield();
            lock_->lock();
        }
        ++wcount_;

        if (wcount_ != 1) {
            fprintf(stderr, "more than 1 writer (%d writers) entered lock!!", 
                    wcount_);
            exit(-1);
        }

        lock_->unlock();
    }

    //! Drop the write lock.
    void exclusive_unlock() {
        lock_->lock();

        --wcount_;
        if (wcount_ != 0) {
            fprintf(stderr, "more than 1 writer (%d writers) entered lock!!", 
                    wcount_);
            exit(-1);
        }

        lock_->unlock();        
    }
    
private:
    Lock* lock_;

    int rcount_;
    int wcount_;
};

#define SCOPE_LOCK_DEFUN(_name, _fcn)                   \
class ScopeLock_ ## _name {                             \
    ScopeLock_ ## _name (RWLock*     rw_lock,           \
                         const char* lock_user)         \
        : rw_lock_(rw_lock)                             \
    {                                                   \
        do_lock(lock_user);                             \
    }                                                   \
                                                        \
    ScopeLock_ ## _name (ScopePtr<RWLock> rw_lock,      \
                         const char*      lock_user)    \
        : rw_lock_(rw_lock.ptr())                       \
    {                                                   \
        do_lock(lock_user);                             \
    }                                                   \
                                                        \
    ScopeLock_ ## _name (auto_ptr<RWLock> rw_lock,      \
                         const char*      lock_user)    \
        : rw_lock_(rw_lock.get())                       \
    {                                                   \
        do_lock(lock_user);                             \
    }                                                   \
                                                        \
    ~ScopeLock_ ## _name ()                             \
    {                                                   \
        if (rw_lock_) {                                 \
            do_unlock();                                \
        }                                               \
    }                                                   \
                                                        \
private:                                                \
    RWLock* rw_lock_;                                   \
                                                        \
    void do_lock(const char* lock_user) {               \
        rw_lock_->_fcn ## _lock();                      \
    }                                                   \
                                                        \
    void do_unlock() {                                  \
        rw_lock_->_fcn ## _unlock();                    \
    }                                                   \
};

/*! @{ 
 * Define ScopeLock_Shared and ScopeLock_Exclusive.
 */
SCOPE_LOCK_DEFUN(Shared,    shared);
SCOPE_LOCK_DEFUN(Exclusive, exclusive);
//! @}
#undef SCOPE_LOCK_DEFUN

} // namespace oasys

#endif /* __RWLOCK_H__ */
