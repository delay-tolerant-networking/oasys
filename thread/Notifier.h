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

#ifndef _NOTIFIER_H_
#define _NOTIFIER_H_

#include "../debug/Log.h"

class SpinLock;

/**
 * Thread notification abstraction that wraps an underlying pipe. This
 * can be used as a generic abstraction to pass messages between
 * threads.
 *
 * A call to notify() sends a byte down the pipe, while calling wait()
 * causes the thread to block in poll() waiting for some data to be
 * sent on the pipe.
 *
 * In addition, through read_fd() a thread can get the file descriptor
 * to explicitly block on while awaiting notification. In that case,
 * the calling thread should explicitly call drain_pipe().
 */
class Notifier : public Logger {
public:
    Notifier(const char* logpath = NULL);
    ~Notifier();

    /**
     * Block the calling thread, pending a call to notify(). If a lock
     * is passed in, wait() will unlock the lock before the thread
     * blocks and re-take it when the thread unblocks. If a timeout is
     * specified, only wait for that amount of time.
     *
     * Returns true if the thread was notified, false if a timeout
     * occurred.
     */
    bool wait(SpinLock* lock = NULL, int timeout = -1);

    /**
     * Notify a waiter.
     */
    void notify();

    /**
     * Drain the pipe. Never blocks.
     */
    void drain_pipe();

    /**
     * The read side of the pipe, suitable for an explicit call to
     * poll().
     */
    int read_fd() { return pipe_[0]; }

    /**
     * The write side of the pipe.
     */
    int write_fd() { return pipe_[1]; }
    
protected:
    bool waiter_; // for debugging only
    int pipe_[2];
};

#endif /* _NOTIFIER_H_ */
