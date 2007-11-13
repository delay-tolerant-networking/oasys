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
                u_int64_t   depth,   /* in tokens */
                u_int64_t   rate     /* in tokens per second */);

    /**
     * Drains the specified amount from the bucket. If only_if_enough
     * is set, this will only drain the amount if the bucket has
     * capacity, otherwise this may make the bucket contain a negative
     * number of tokens
     *
     * Note that this function will first try to fill() the bucket, so
     * in fact, this is the only function (besides the constructor)
     * that needs to be called in order for the bucket to work
     * properly.
     *
     * @return true if there were enough tokens in the bucket
     */
    bool drain(u_int64_t length, bool only_if_enough = false);

    /**
     * Syntactic sugar for drain() with only_if_enough set to true.
     */
    bool try_to_drain(u_int64_t length);

    /**
     * Update the number of tokens in the bucket without draining any.
     * Since drain() will also update the number of tokens, there's
     * usually no reason to call this function explicitly.
     */
    void update();

    /**
     * Return the amount of time until the bucket will be full again.
     */
    Time time_to_fill();

    /**
     * Return the amount of time (in millseconds) until the bucket
     * has at least n tokens in it.
     */
    Time time_to_level(int64_t n);

    /// @{ Accessors
    u_int64_t depth()  const { return depth_; }
    u_int64_t rate()   const { return rate_; }
    int64_t   tokens() const { return tokens_; }
    /// @}

    /// @{ Setters
    void set_depth(u_int64_t depth) { depth_ = depth; update(); }
    void set_rate(u_int64_t rate)   { rate_  = rate;  update(); }
    /// @}

    /**
     * Empty the bucket.
     */
    void empty();
    
protected:
    u_int64_t depth_;
    u_int64_t rate_;
    int64_t   tokens_;
    Time      last_update_;
};

} // namespace oasys

#endif /* _OASYS_TOKEN_BUCKET_H_ */
