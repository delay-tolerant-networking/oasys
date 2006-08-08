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
#ifndef _OASYS_TOKEN_BUCKET_H_
#define _OASYS_TOKEN_BUCKET_H_

#include "Time.h"
#include "../debug/Logger.h"

namespace oasys {

/**
 * A basic token bucket implementation.
 */
class TokenBucket : public Logger {
public:
    /**
     * Constructor that takes the initial depth and rate parameters.
     */
    TokenBucket(const char* logpath,
                u_int32_t   depth,   /* in tokens */
                u_int32_t   rate     /* in seconds */);

    /**
     * Try to drain the specified amount from the bucket. Note that
     * this function will first try to fill() the bucket, so in fact,
     * this is the only function (besides the constructor) that needs
     * to be called in order for the bucket to work properly.
     *
     * @return true if the bucket was drained the given amount, false
     * if there's not enough tokens in the bucket
     */
    bool drain(u_int32_t length);

    /**
     * Update the number of tokens in the bucket without draining any.
     * Since drain() will also update the number of tokens, there's
     * usually no reason to call this function explicitly.
     */
    void update();

    /// @{ Accessors
    u_int32_t depth()  const { return depth_; }
    u_int32_t rate()   const { return rate_; }
    u_int32_t tokens() const { return tokens_; }
    /// @}

    /// @{ Setters
    void set_depth(u_int32_t depth) { depth_ = depth; update(); }
    void set_rate(u_int32_t rate)   { rate_  = rate;  update(); }
    /// @}

    /**
     * Empty the bucket.
     */
    void empty();
    
protected:
    u_int32_t depth_;
    u_int32_t rate_;
    u_int32_t tokens_;
    Time      last_update_;
};

} // namespace oasys

#endif /* _OASYS_TOKEN_BUCKET_H_ */
