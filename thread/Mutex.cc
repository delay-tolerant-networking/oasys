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

#include <unistd.h>
#include <errno.h>

#include "config.h"

#ifdef HAVE_SYNCH_H
#include <synch.h>
#endif

#include "debug/DebugUtils.h"
#include "debug/Log.h"
#include "Mutex.h"

namespace oasys {

Mutex::Mutex(const char* name, lock_type_t type, bool keep_quiet)
    : Lock(), type_(type), keep_quiet_(keep_quiet), lock_count_(0)
{
    if (name != NULL && name[0] == '/')
        ++name;

    if (name == NULL)
    {
        set_logpath("");
    }
    else if (strcmp(name, "lock") == 0) 
    {
        logpathf("/mutex/%s(%p)", name, this);
    }
    else
    {
        logpathf("/mutex/%s", name);
    }

    // Set up the type attribute
    pthread_mutexattr_t attrs;
    if (pthread_mutexattr_init(&attrs) != 0) {
        PANIC("fatal error in pthread_mutexattr_init: %s", strerror(errno));
    }

    int mutex_type;
    switch(type_) {
    case TYPE_FAST:
        mutex_type = PTHREAD_MUTEX_NORMAL;
        break;
    case TYPE_RECURSIVE:
        mutex_type = PTHREAD_MUTEX_RECURSIVE;
        break;
    default:
        NOTREACHED; // needed to avoid uninitialized variable warning
        break;
    }

    if (pthread_mutexattr_settype(&attrs, mutex_type) != 0) {
        PANIC("fatal error in pthread_mutexattr_settype: %s", strerror(errno));
    }

    if (pthread_mutex_init(&mutex_, &attrs) != 0) {
        PANIC("fatal error in pthread_mutex_init: %s", strerror(errno));
    }

    if (pthread_mutexattr_destroy(&attrs) != 0) {
        PANIC("fatal error in pthread_mutexattr_destroy: %s", strerror(errno));
    }
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&mutex_);
    if (logpath_[0] != 0)
        log_debug("destroyed");
}

int
Mutex::lock(const char* lock_user)
{
    int err = pthread_mutex_lock(&mutex_);

    if (err != 0) {
        PANIC("error in pthread_mutex_lock: %s", strerror(errno));
    }

    ++lock_count_;
    if (logpath_[0] != 0)
        log_debug("locked (count %d)", lock_count_);
    lock_holder_      = Thread::current();
    lock_holder_name_ = lock_user;
    
    return err;
}

int
Mutex::unlock()
{
    ASSERT(is_locked_by_me());

    if (--lock_count_ == 0) {
        lock_holder_ = 0;
    }
    
    int err = pthread_mutex_unlock(&mutex_);
    
    if (err != 0) {
        PANIC("error in pthread_mutex_unlock: %s", strerror(errno));
    }

    if (logpath_[0] != 0)
        log_debug("unlocked (count %d)", lock_count_);
    
    return err;
}

int
Mutex::try_lock(const char* lock_user)
{
    int err = pthread_mutex_trylock(&mutex_);

    if (err == EBUSY) {
        if (logpath_[0] != 0) {
            log_debug("try_lock busy");
        }
        return EBUSY;
    } else if (err != 0) {
        PANIC("error in pthread_mutex_trylock: %s", strerror(errno));
    }

    ++lock_count_;
    if (logpath_[0] != 0)
        log_debug("try_lock locked (count %d)", lock_count_);
    lock_holder_      = Thread::current();
    lock_holder_name_ = lock_user;
    return err;
}


/**
 * Since the Log class uses a Mutex internally, calling logf from
 * within lock() / unlock() would lead to infinite recursion, hence we
 * use the keep_quiet_ flag to suppress all logging for this instance.
 */
int
Mutex::logf(log_level_t level, const char *fmt, ...)
{
    if (keep_quiet_) return 0;
    
    va_list ap;
    va_start(ap, fmt);
    int ret = vlogf(level, fmt, ap);
    va_end(ap);
    return ret;
}

} // namespace oasys
