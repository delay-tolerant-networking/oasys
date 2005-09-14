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

#ifndef _OASYS_MSG_QUEUE_H_
#define _OASYS_MSG_QUEUE_H_

#include <queue>
#include <unistd.h>
#include <errno.h>
#include <sys/poll.h>

#include "Notifier.h"
#include "SpinLock.h"
#include "../debug/Log.h"
#include "../debug/DebugUtils.h"

namespace oasys {

/**
 * A producer/consumer queue for passing data between threads in the
 * system, using the Notifier functionality to block and wakeup
 * threads.
 */
template<typename _elt_t>
class MsgQueue : public Notifier {
public:    
    /*!
     * Constructor.
     */
    MsgQueue(const char* logpath = NULL, SpinLock* lock = NULL);
        
    /**
     * Destructor.
     */
    ~MsgQueue();

    /**
     * Atomically add msg to the back of the queue and signal a
     * waiting thread.
     */
    void push(_elt_t msg, bool at_back = true);
    
    /**
     * Atomically add msg to the front of the queue, and signal
     * waiting threads.
     */
    void push_front(_elt_t msg)
    {
        push(msg, false);
    }

    /**
     * Block and pop msg from the queue.
     */
    _elt_t pop_blocking();

    /**
     * Try to pop a msg from the queue, but don't block. Return
     * true if there was a message on the queue, false otherwise.
     */
    bool try_pop(_elt_t* eltp);
    
    /**
     * \return Size of the queue.
     */
    size_t size()
    {
        ScopeLock l(lock_);
        return queue_.size();
    }

protected:
    SpinLock*          lock_;
    std::deque<_elt_t> queue_;
};

#include "MsgQueue.tcc"

} // namespace oasys

#endif //_OASYS_MSG_QUEUE_H_
