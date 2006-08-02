/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2006 Intel Corporation. All rights reserved. 
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

#ifndef _OASYS_ONOFFNOTIFIER_H_
#define _OASYS_ONOFFNOTIFIER_H_

#include "SpinLock.h"
#include "../debug/Log.h"
#include "../debug/Logger.h"

namespace oasys {

class Lock;

/**
 * OnOffNotifier is a binary state synchronization object. It has two
 * states, signalled and not signalled. When a OnOffNotifier becomes
 * active, threads that was blocked waiting for the OnOffNotifier becomes
 * unblocked. Also, any subsequent calls to wait on an active OnOffNotifier
 * returns immediately.
 *
 * When inactive, threads block waiting for the OnOffNotifier.
 */
class OnOffNotifier : public Logger {
public:
    /**
     * Constructor that takes the logging path and an optional boolean
     * to suppress all logging.
     */
    OnOffNotifier(const char* logpath = 0, bool keep_quiet = true);

    /**
     * Destructor
     */
    ~OnOffNotifier();

    /**
     * Block the calling thread, pending a switch to the active
     * state. 
     *
     * @param lock If a lock is passed in, wait() will unlock the lock
     * before the thread blocks and re-take it when the end
     * unblocks.
     *
     * @param timeout Timeout in milliseconds.
     *
     * Returns true if the thread was notified, false if a timeout
     * occurred.
     */
    bool wait(Lock* lock = 0, int timeout = -1);

    /**
     * Switch to active state. Please note that there is not
     * guarrantee that when signal_OnOffNotifier returns, all of the
     * threads waiting on the OnOffNotifier have been released.
     */
    void signal();
    
    /**
     * Clear the signaled state.
     */
    void clear();
    
    /**
     * @return the read side of the pipe
     */ 
    int read_fd() { return pipe_[0]; }
    
protected:
    bool waiter_; // for debugging only
    bool quiet_;  // no logging

    SpinLock notifier_lock_;
    bool     active_;
    int      pipe_[2];
};

} // namespace oasys

#endif /* _OASYS_ONOFFNOTIFIER_H_ */
