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

#include "debug/Debug.h"
#include "debug/Log.h"
#include "Mutex.h"

namespace oasys {

Mutex::Mutex(const char* name, lock_type_t type, bool keep_quiet)
    : Lock(), type_(type), keep_quiet_(keep_quiet)
{
    // This hackish assignment is here because C99 syntax has diverged
    // from the C++ standard! Woohoo!
    switch(type_)
    {
    case TYPE_FAST: {
        pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
        mutex_ = m;
        break;
    }

    case TYPE_RECURSIVE: {
#ifdef __FreeBSD__
        pthread_mutex_t m;
        pthread_mutex_init(&m, 0);
#else
        pthread_mutex_t m = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

        mutex_ = m;
        break;
    }}

    if (name[0] == '/')
        ++name;
    
    if (strcmp(name, "lock") == 0) 
    {
        logpathf("/mutex/%s(%p)", name, this);
    }
    else
    {
        logpathf("/mutex/%s", name);
    }
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&mutex_);
    logf(LOG_DEBUG, "destroyed");
}

int
Mutex::lock()
{
    int err = pthread_mutex_lock(&mutex_);
    
    logf(LOG_DEBUG,
         "%s %d",
         (err == 0) ? "locked" : "error on locking",
         err);
    
    if (err == 0)
        lock_holder_ = Thread::current();

    return err;
}

int
Mutex::unlock()
{
    int err = pthread_mutex_unlock(&mutex_);
    
    logf(LOG_DEBUG, 
         "%s %d",
         (err == 0) ? "unlocked" : "error on unlocking",
         err);

    if (err == 0)
        lock_holder_ = 0;
    
    return err;
}

int
Mutex::try_lock()
{
    int err = pthread_mutex_trylock(&mutex_);

    if (err == EBUSY) 
    {
        logf(LOG_DEBUG, "try_lock busy");
    }

    logf(LOG_DEBUG, 
         "%s %d",
         (err == 0) ? "locked" : "error on try_lock",
         err);

    if (err == 0)
        lock_holder_ = Thread::current();
    
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
