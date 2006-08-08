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

#include "TokenBucket.h"

namespace oasys {

//----------------------------------------------------------------------
TokenBucket::TokenBucket(const char* logpath,
                         u_int32_t   depth,   /* in bits */
                         u_int32_t   rate     /* in seconds */)
    : Logger("TokenBucket", logpath),
      depth_(depth),
      rate_(rate),
      tokens_(depth) // initialize full
{
    log_debug("initialized token bucket with depth %u and rate %u",
              depth_, rate_);
    last_update_.get_time();
}

//----------------------------------------------------------------------
void
TokenBucket::update()
{
    Time now;
    now.get_time();

    if (tokens_ == depth_) {
        log_debug("bucket already full, nothing to update");
        last_update_ = now;
        return;
    }

    u_int32_t elapsed = (now - last_update_).in_milliseconds();
    u_int32_t new_tokens = (rate_ * elapsed) / 1000;

    if (new_tokens != 0) {
        if ((tokens_ + new_tokens) > depth_) {
            new_tokens = depth_ - tokens_;
        }

        log_debug("filling %u/%u tokens after %u milliseconds",
                  new_tokens, depth_, elapsed);
        tokens_ += new_tokens;
        last_update_ = now;
        
    } else {
        // there's a chance that, for a slow rate, that the elapsed
        // time isn't enough to fill even a single token. in this
        // case, we leave last_update_ to where it was before,
        // otherwise we might starve the bucket.
        log_debug("%u milliseconds elapsed not enough to fill any tokens...",
                  elapsed);
    }
}

//----------------------------------------------------------------------
bool
TokenBucket::drain(u_int32_t length)
{
    update();

    if (length <= tokens_) {
        log_debug("draining %u/%u tokens from bucket",
                  length, tokens_);
        tokens_ -= length;
        return true;
    } else {
        log_debug("not enough tokens (%u) to drain %u from bucket",
                  tokens_, length);
        return false;
    }
}

//----------------------------------------------------------------------
void
TokenBucket::empty()
{
    tokens_      = 0;
    last_update_.get_time();
}

} // namespace oasys
