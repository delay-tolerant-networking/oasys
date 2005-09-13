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

#include <errno.h>
#include <unistd.h>
#include <sys/poll.h>
#include "Notifier.h"
#include "SpinLock.h"
#include "io/IO.h"

namespace oasys {

Notifier::Notifier(const char* logpath)
{
    if (logpath) {
        set_logpath(logpath);
    } else {
        logpathf("/notifier/%p", this);
    }
    
    if (pipe(pipe_) != 0) {
        log_crit("can't create pipe for notifier");
        exit(1);
    }

    log_debug("created pipe, fds: %d %d", pipe_[0], pipe_[1]);
    
    for (int n = 0; n < 2; ++n) {
        if (IO::set_nonblocking(pipe_[n], true, logpath_) != 0) {
            log_crit("error setting fd %d to nonblocking: %s",
                     pipe_[n], strerror(errno));
            exit(1);
        }
    }

    waiter_ = false;
}

Notifier::~Notifier()
{
    close(pipe_[0]);
    close(pipe_[1]);
}

void
Notifier::drain_pipe(size_t bytes)
{
    int    ret;
    char   buf[256];
    size_t bytes_drained = 0;

    while (true)
    {
        ret = IO::read(read_fd(), buf, 
                       std::min(sizeof(buf), bytes - bytes_drained));
        if (ret <= 0) {
            if (ret == -1 && errno == EAGAIN) {
                return;
            } else {
                log_crit("drain_pipe: unexpected error return from read: %s",
                         strerror(errno));
                exit(1);
            }
        }
        
        bytes_drained += ret;
        log_debug("drain_pipe: drained %u/%u byte(s) from pipe", 
                  bytes_drained, bytes);
        
        if (bytes != 0 && bytes_drained == bytes) {
            return;
        }
        
        // More bytes were requested from the pipe than there are
        // bytes in the pipe. This means that the bytes requested is
        // bogus. This probably is the result of a race condition.
        if (ret < static_cast<int>(sizeof(buf))) {
            log_warn("drain_pipe: only possible to drain %u bytes out of %u! "
                     "race condition?", bytes_drained, bytes);
            return;
        }
    }
}

bool
Notifier::wait(SpinLock* lock, int timeout)
{
    if (waiter_) {
        PANIC("Notifier doesn't support multiple waiting threads");
    }
    waiter_ = true;

    if (lock)
        lock->unlock();

    int ret = IO::poll(read_fd(), POLLIN, 0, timeout, 0, logpath_);
    if (ret < 0 && ret != IOTIMEOUT) {
        PANIC("fatal: error return from notifier poll: %s",
              strerror(errno));
    }
    
    if (lock) {
        lock->lock();
    }

    waiter_ = false;
    
    if (ret == IOTIMEOUT) {
        return false; // timeout
    } else {
        drain_pipe();
        return true;
    }
}

void
Notifier::notify()
{
    char b = 0;
  retry:
    int ret = ::write(write_fd(), &b, 1);
    
    if (ret == -1) {
        if (errno == EAGAIN) {
            // If the pipe is full, that probably means the consumer
            // is just slow, but keep trying because otherwise we will
            // break the semantics of the notifier.
            log_warn("pipe appears to be full");
            goto retry;
        } else {
            log_err("unexpected error writing to pipe: %s", strerror(errno));
        }
    } else if (ret == 0) {
        log_err("unexpected eof writing to pipe");
    } else {
        ASSERT(ret == 1);
    }
}

} // namespace oasys
