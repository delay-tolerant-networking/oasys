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

#include "SpinLock.h"
#include "../debug/StackTrace.h"

namespace oasys {

atomic_t SpinLock::total_spins_(0);
atomic_t SpinLock::total_yields_(0);

int
SpinLock::lock(const char* lock_user)
{
    if (is_locked_by_me()) {
        lock_count_.value++;
        return 0;
    }

    atomic_incr(&lock_waiters_);
    
    int nspins = 0;
    while (atomic_cmpxchg32(&lock_count_, 0, 1) != 0)
    {
        Thread::spin_yield();
        
#ifndef NDEBUG
        atomic_incr(&total_spins_);
        if (++nspins > 1000000) {
            fprintf(stderr,
                    "warning: spin lock held by %s reached spin limit\n",
                    lock_holder_name_);
            StackTrace::print_current_trace(false);
            nspins = 0;
        }
#endif
    }

    atomic_decr(&lock_waiters_);

    ASSERT(lock_count_.value == 1);

    lock_holder_      = Thread::current();
    lock_holder_name_ = lock_user;

    return 0;
};

int
SpinLock::unlock()
{
    ASSERT(is_locked_by_me());

    if (lock_count_.value > 1) {
        lock_count_.value--;
        return 0;
    }

    lock_holder_      = 0;
    lock_holder_name_ = 0;
    lock_count_.value = 0;
    
    if (lock_waiters_.value != 0) {
#ifndef NDEBUG
        atomic_incr(&total_yields_);
#endif
        Thread::spin_yield();
    }


    return 0;
};
 
int
SpinLock::try_lock(const char* lock_user)
{
    if (is_locked_by_me()) {
        lock_count_.value++;
        return 0;
    }

    int got_lock = atomic_cmpxchg32(&lock_count_, 0, 1);
    
    if (got_lock) {
        ASSERT(lock_holder_ == 0);

        lock_holder_      = Thread::current();
        lock_holder_name_ = lock_user;
        
        return 0; // success
        
    } else {
        return 1; // already locked
    }
};

} // namespace oasys
