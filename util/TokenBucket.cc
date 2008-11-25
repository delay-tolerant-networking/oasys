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

#ifdef HAVE_CONFIG_H
#  include <oasys-config.h>
#endif

#include "TokenBucket.h"

namespace oasys {

//----------------------------------------------------------------------
TokenBucket::TokenBucket(const char* logpath,
                         u_int64_t   depth,   /* in bits */
                         u_int64_t   rate     /* in seconds */)
    : Logger("TokenBucket", "%s", logpath),
      depth_(depth),
      rate_(rate),
      tokens_(depth) // initialize full
{
    log_debug("initialized token bucket with depth %llu and rate %llu",
              U64FMT(depth_), U64FMT(rate_));
    last_update_.get_time();
}

//----------------------------------------------------------------------
void
TokenBucket::update()
{
    Time now;
    now.get_time();

    if (tokens_ == (int64_t)depth_) {
        log_debug("update: bucket already full, nothing to update");
        last_update_ = now;
        return;
    }

    u_int32_t elapsed = (now - last_update_).in_milliseconds();
    u_int64_t new_tokens = (rate_ * elapsed) / 1000;

    if (new_tokens != 0) {
        if ((tokens_ + new_tokens) > depth_) {
            new_tokens = depth_ - tokens_;
        }

        log_debug("update: filling %llu/%lld spent tokens after %u milliseconds",
                  U64FMT(new_tokens), I64FMT(depth_ - tokens_), elapsed);
        tokens_ += new_tokens;
        last_update_ = now;
        
    } else {
        // there's a chance that, for a slow rate, that the elapsed
        // time isn't enough to fill even a single token. in this
        // case, we leave last_update_ to where it was before,
        // otherwise we might starve the bucket.
        log_debug("update: %u milliseconds elapsed not enough to fill any tokens",
                  elapsed);
    }
}

//----------------------------------------------------------------------
bool
TokenBucket::drain(u_int64_t length, bool only_if_enough)
{
    update();

    bool enough = (tokens_ < 0) ? false : (length <= (u_int64_t)tokens_);

    log_debug("drain: draining %llu/%lld tokens from bucket",
              U64FMT(length), I64FMT(tokens_));

    if (enough || !only_if_enough) {
        tokens_ -= length;
    }

    if (only_if_enough) {
        ASSERT(tokens_ >= 0);
    }
    
    return enough;
}

//----------------------------------------------------------------------
bool
TokenBucket::try_to_drain(u_int64_t length)
{
    return drain(length, true);
}

//----------------------------------------------------------------------
Time
TokenBucket::time_to_level(int64_t n)
{
    update();

    int64_t need = 0;
    if (n > tokens_) {
        need = n - tokens_;
    }

    Time t(need / rate_,
           ((need * 1000000) / rate_) % 1000000);
    
    log_debug("time_to_level(%lld): "
              "%lld more tokens will arrive in %u.%u "
              "(tokens %lld rate %llu)",
              U64FMT(n), U64FMT(need), t.sec_, t.usec_,
              I64FMT(tokens_), U64FMT(rate_));
    return t;
}

//----------------------------------------------------------------------
Time
TokenBucket::time_to_fill()
{
    return time_to_level(depth_);
}

//----------------------------------------------------------------------
void
TokenBucket::empty()
{
    tokens_      = 0;
    last_update_.get_time();

    log_debug("empty: clearing bucket");
}

} // namespace oasys
