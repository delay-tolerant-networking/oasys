/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OASYS_LOCK_H_
#define _OASYS_LOCK_H_

#include "Atomic.h"
#include "Thread.h"

namespace oasys {

/**
 * Abstract Lock base class.
 */
class Lock {
public:
    /**
     * Default Lock constructor.
     */
    Lock() : lock_count_(0),
             lock_holder_(0),
             lock_holder_name_(0), 
             scope_lock_count_(0) 
    {
    }

    /**
     * Lock destructor. Asserts that the lock is not locked by another
     * thread or by a scope lock.
     */
    virtual ~Lock()
    {
        if (is_locked()) {
            ASSERT(is_locked_by_me());
            ASSERT(scope_lock_count_ == 0);
        }
    }
    
    /**
     * Acquire the lock.
     *
     * @return 0 on success, -1 on error
     */
    virtual int lock(const char* lock_user) = 0;

    /**
     * Release the lock.
     *
     * @return 0 on success, -1 on error
     */
    virtual int unlock() = 0;

    /**
     * Try to acquire the lock.
     *
     * @return 0 on success, 1 if already locked, -1 on error.
     */
    virtual int try_lock(const char* lock_user) = 0;

    /**
     * Check for whether the lock is locked or not.
     *
     * @return true if locked, false otherwise
     */
    bool is_locked()
    {
        return (lock_count_.value != 0);
    }

    /**
     * Check for whether the lock is locked or not by the calling
     * thread.
     *
     * @return true if locked by the current thread, false otherwise
     */
    bool is_locked_by_me()
    {
        return is_locked() &&
            pthread_equal(lock_holder_, Thread::current());
    }

protected:
    friend class ScopeLock;

    /**
     * Stores a count of the number of locks currently held, needed
     * for recursive locking. Note that 0 means the lock is currently
     * not locked.
     */
    atomic_t lock_count_;
    
    /**
     * Stores the pthread thread id of the current lock holder. It is
     * the responsibility of the derived class to set this in lock()
     * and unset it in unlock(), since the accessors is_locked() and
     * is_locked_by_me() depend on it.
     */
    pthread_t lock_holder_;

    /**
     * Lock holder name for debugging purposes. Identifies call site
     * from which lock has been held.
     */ 
    const char* lock_holder_name_;

    /**
     * Stores a count of the number of ScopeLocks holding the lock.
     * This is checked in the Lock destructor to avoid strange crashes
     * if you delete the lock object and then the ScopeLock destructor
     * tries to unlock it.
     */
    int scope_lock_count_;
};

/**
 * Scope based lock created from a Lock. Holds the lock until the
 * object is destructed. Example of use:
 *
 * \code
 * {
 *     Mutex m;
 *     ScopeLock lock(&m);
 *     // protected code
 *     ...
 * }
 * \endcode
 */
class ScopeLock {
public:
    ScopeLock(Lock* l, const char* lock_user) 
        : lock_(l)
    {
        int ret = lock_->lock(lock_user);
        ASSERT(ret == 0);
        lock_->scope_lock_count_++;
    }
    
    void unlock() {
        lock_->scope_lock_count_--;
        lock_->unlock();
        lock_ = 0;
    }
    
    ~ScopeLock()
    {
        if (lock_) {
            unlock();
        }
    }
    
protected:
    Lock* lock_;
};

} // namespace oasys

#endif /* LOCK_h */
