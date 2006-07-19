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

#ifndef _OASYS_ATOMIC_MUTEX_H_
#define _OASYS_ATOMIC_MUTEX_H_

/**
 * @file Atomic-mutex.h
 *
 * This file defines an architecture-agnostic implementation of the
 * atomic routines. A singleton global mutex is locked before every
 * operation, ensuring safety, at the potential expense of
 * performance.
 */

#include "../debug/DebugUtils.h"
#include "../util/Singleton.h"

namespace oasys {

class Mutex;

/**
 * Global accessor to the singleton atomic mutex.
 */
Mutex* atomic_mutex();

/**
 * The definition of atomic_t for is just a wrapper around the value,
 * since the mutex is in a singleton.
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
void atomic_add(volatile atomic_t *v, u_int32_t i);

/**
 * Atomic subtraction function.
 *
 * @param i	integer value to subtract
 * @param v	pointer to current value
 */
void atomic_sub(volatile atomic_t* v, u_int32_t i);

/**
 * Atomic increment.
 *
 * @param v	pointer to current value
 */
void atomic_incr(volatile atomic_t* v);

/**
 * Atomic decrement.
 *
 * @param v	pointer to current value
 * 
 */ 
void atomic_decr(volatile atomic_t* v);

/**
 * Atomic decrement and test.
 *
 * @return true if the value zero after the decrement, false
 * otherwise.
 *
 * @param v	pointer to current value
 * 
 */ 
bool atomic_decr_test(volatile atomic_t* v);

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
u_int32_t atomic_cmpxchg32(volatile atomic_t* v, u_int32_t o, u_int32_t n);

/**
 * Atomic increment function that returns the new value.
 *
 * @param v 	pointer to current value
 */
u_int32_t atomic_incr_ret(volatile atomic_t* v);

/**
 * Atomic addition function that returns the new value.
 *
 * @param v 	pointer to current value
 * @param i 	integer to add
 */
u_int32_t atomic_add_ret(volatile atomic_t* v, u_int32_t i);

} // namespace oasys

#endif /* _OASYS_ATOMIC_MUTEX_H_ */
