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

#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "Atomic.h"
#include "Lock.h"
#include "Thread.h"

#ifndef __NO_ATOMIC__

namespace oasys {

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

} // namespace oasys

#else

/**
 * If we know there are no atomic instructions for the architecture,
 * just use a Mutex.
 */
#include "Mutex.h"
namespace oasys {

class SpinLock : public Mutex {
public:
    SpinLock() : Mutex("spinlock", TYPE_RECURSIVE, true) {}
};

} // namespace oasys

#endif /* __NO_ATOMIC__ */

#endif /* _SPINLOCK_H_ */
