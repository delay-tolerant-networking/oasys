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

#ifndef _OASYS_MUTEX_H_
#define _OASYS_MUTEX_H_

#include <pthread.h>
#include "Lock.h"
#include "../debug/Logger.h"

namespace oasys {

/// Mutex wrapper class for pthread_mutex_t.
class Mutex : public Lock, public Logger {
    friend class Monitor; // Monitor needs access to mutex_.
    
public:
    /// Different kinds of mutexes offered by Linux, distinguished by
    /// their response to a single thread attempting to acquire the
    /// same lock more than once.
    ///
    /// - FAST: No error checking. The thread will block forever.
    /// - RECURSIVE: Double locking is safe.
    ///
    enum lock_type_t {
        TYPE_FAST = 1,
        TYPE_RECURSIVE
    };

    /// Creates a mutex. By default, we create a TYPE_RECURSIVE.
    Mutex(const char* name = "lock", 
          lock_type_t type = TYPE_RECURSIVE,
          bool keep_quiet = false);
    ~Mutex();

    /// Aquire mutex.
    int lock();

    /// Release mutex.
    int unlock();

    /// Try to acquire a lock. If already locked, fail.
    /// \return 0 on success, -1 on failure.
    int try_lock();

    /// Override to implement keep_quiet_ in a sane way
    int logf(log_level_t level, const char *fmt, ...) PRINTFLIKE(3, 4);

protected:
    pthread_mutex_t mutex_;        ///< the underlying mutex
    lock_type_t     type_;         ///< the mutex type
    bool	    keep_quiet_;   ///< no logging
    unsigned int    lock_count_;   ///< count for recursive locking, needed
                                   ///< for proper management of lock_holder_
};

} // namespace oasys

#endif /* _OASYS_MUTEX_H_ */

