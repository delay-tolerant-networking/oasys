/*
 *    Copyright 2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

    /**
     * Return the amount of time (in millseconds) until the bucket
     * will be full again.
     */
    u_int32_t time_to_fill();

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
