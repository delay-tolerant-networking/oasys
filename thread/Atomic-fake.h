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

#ifndef _OASYS_ATOMIC_FAKE_H_
#define _OASYS_ATOMIC_FAKE_H_

/**
 * @file Atomic-fake.h
 *
 * This file defines the atomic routines in a known to be unsafe
 * manner, i.e. by simply performing the operation with no
 * synchronization. The rationale for this implementation is to be
 * used to validate testing frameworks -- i.e. tests should fail if
 * they use this implementation instead of the real atomic
 * implementation.
 */

#include "../debug/DebugUtils.h"
#include "../util/Singleton.h"

namespace oasys {

/**
 * The definition of atomic_t for is just a wrapper around the value.
 */
struct atomic_t {
    atomic_t(u_int32_t v = 0) : value(v) {}

    volatile u_int32_t value;
};

/**
 * Atomic addition function.
 *
 * @param i	integer value to add
 * @param v	pointer to current value
 * 
 */
static inline void
atomic_add(volatile atomic_t *v, u_int32_t i)
{
    v->value += i;
}

/**
 * Atomic subtraction function.
 *
 * @param i	integer value to subtract
 * @param v	pointer to current value
 */
static inline void
atomic_sub(volatile atomic_t* v, u_int32_t i)
{
    v->value -= i;
}

/**
 * Atomic increment.
 *
 * @param v	pointer to current value
 */
static inline void
atomic_incr(volatile atomic_t* v)
{
    v->value++;
}

/**
 * Atomic decrement.
 *
 * @param v	pointer to current value
 * 
 */ 
static inline void
atomic_decr(volatile atomic_t* v)
{
    v->value--;
}

/**
 * Atomic decrement and test.
 *
 * @return true if the value zero after the decrement, false
 * otherwise.
 *
 * @param v	pointer to current value
 * 
 */ 
static inline bool
atomic_decr_test(volatile atomic_t* v)
{
    v->value--;
    return (v->value == 0);
}

/**
 * Atomic compare and swap. Stores the new value iff the current value
 * is the expected old value.
 *
 * @param v 	pointer to current value
 * @param o 	old value to compare against
 * @param n 	new value to store
 *
 * @return 	the value of v before the swap
 */
static inline u_int32_t
atomic_cmpxchg32(volatile atomic_t* v, u_int32_t o, u_int32_t n)
{
    u_int32_t ret = v->value;
    
    if (v->value == o) {
        v->value = n;
    }
    
    return ret; 
}

/**
 * Atomic increment function that returns the new value.
 *
 * @param v 	pointer to current value
 */
static inline u_int32_t
atomic_incr_ret(volatile atomic_t* v)
{
    v->value++;
    return v->value;
}

/**
 * Atomic addition function that returns the new value.
 *
 * @param v 	pointer to current value
 * @param i 	integer to add
 */
static inline u_int32_t
atomic_add_ret(volatile atomic_t* v, u_int32_t i)
{
    v->value += i;
    return v->value;
}

} // namespace oasys

#endif /* _OASYS_ATOMIC_FAKE_H_ */
