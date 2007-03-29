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
#  include <config.h>
#endif

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
        log_debug("update: bucket already full, nothing to update");
        last_update_ = now;
        return;
    }

    u_int32_t elapsed = (now - last_update_).in_milliseconds();
    u_int32_t new_tokens = (rate_ * elapsed) / 1000;

    if (new_tokens != 0) {
        if ((tokens_ + new_tokens) > depth_) {
            new_tokens = depth_ - tokens_;
        }

        log_debug("update: filling %u/%u spent tokens after %u milliseconds",
                  new_tokens, depth_ - tokens_, elapsed);
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
TokenBucket::drain(u_int32_t length)
{
    update();

    if (length <= tokens_) {
        log_debug("drain: draining %u/%u tokens from bucket",
                  length, tokens_);
        tokens_ -= length;
        return true;
    } else {
        log_debug("drain: not enough tokens (%u) to drain %u from bucket",
                  tokens_, length);
        return false;
    }
}

//----------------------------------------------------------------------
u_int32_t
TokenBucket::time_to_fill()
{
    update();
    
    u_int32_t t = ((depth_ - tokens_) * 1000) / rate_;

    log_debug("time_to_fill: %u tokens will be full in %u msecs",
              (depth_ - tokens_), t);
    return t;
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
