/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2005 Intel Corporation. All rights reserved. 
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

#ifndef _OASYS_ATOMIC_PPC_H_
#define _OASYS_ATOMIC_PPC_H_

#include "../debug/DebugUtils.h"

namespace oasys {

/**
 * The definition of atomic_t for x86 is just a wrapper around the
 * value, since we have enough synchronization support in the
 * architecture.
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
static inline u_int32_t
atomic_add_ret(volatile atomic_t* v, u_int32_t i)
{
    register u_int32_t ret;

    __asm__ __volatile__(
        "1:	lwarx %0, 0, %2\n"       /* load old value */
        "	add %0, %3, %0\n"        /* calculate new value */
        "	stwcx. %0, 0, %2\n"      /* attempt to store */
        "	bne- 1b\n"               /* spin if failed */
        : "=&r" (ret), "=m" (v->value)
        : "r" (v), "r" (i), "m" (v->value)
        : "cc", "memory");

    return ret;
}

/**
 * Atomic subtraction function.
 *
 * @param i	integer value to subtract
 * @param v	pointer to current value
 */
static inline u_int32_t
atomic_sub_ret(volatile atomic_t* v, u_int32_t i)
{
    register u_int32_t ret;

    __asm__ __volatile__(
        "1:	lwarx %0, 0, %2\n"       /* load old value */
        "	subfc %0, %3, %0\n"      /* calculate new value */
        "	stwcx. %0, 0, %2\n"      /* attempt to store */
        "	bne- 1b\n"               /* spin if failed */
        : "=&r" (ret), "=m" (v->value)
        : "r" (v), "r" (i), "m" (v->value)
        : "cc", "memory");

    return ret;
}

/// @{
/// Wrapper variants around the basic add/sub functions above

static inline void
atomic_add(volatile atomic_t* v, u_int32_t i)
{
    atomic_add_ret(v, i);
}

static inline void
atomic_sub(volatile atomic_t* v, u_int32_t i)
{
    atomic_sub_ret(v, i);
}

static inline void
atomic_incr(volatile atomic_t* v)
{
    atomic_add(v, 1);
}

static inline void
atomic_decr(volatile atomic_t* v)
{
    atomic_sub(v, 1);
}

static inline u_int32_t
atomic_incr_ret(volatile atomic_t* v)
{
    return atomic_add_ret(v, 1);
}

static inline u_int32_t
atomic_decr_ret(volatile atomic_t* v)
{
    return atomic_sub_ret(v, 1);
}

static inline bool
atomic_decr_test(volatile atomic_t* v)
{
    return (atomic_sub_ret(v, 1) == 0);
}

/// @}

/**
 * Atomic compare and set. Stores the new value iff the current value
 * is the expected old value.
 *
 * @param v 	pointer to current value
 * @param o 	old value to compare against
 * @param n 	new value to store
 *
 * @return 	zero if the compare failed, non-zero otherwise
 */
static inline u_int32_t
atomic_cmpxchg32(volatile atomic_t* v, u_int32_t o, u_int32_t n)
{
    register u_int32_t ret;

    __asm __volatile (
        "1:	lwarx %0, 0, %2\n"       /* load old value */
        "	cmplw %3, %0\n"          /* compare */
        "	bne 2f\n"                /* exit if not equal */
        "	stwcx. %4, 0, %2\n"      /* attempt to store */
        "	bne- 1b\n"               /* spin if failed */
        "	b 3f\n"                  /* we've succeeded */
        "	2:\n"
        "	stwcx. %0, 0, %2\n"      /* clear reservation (74xx) */
        "	3:\n"
        : "=&r" (ret), "=m" (v->value)
        : "r" (v), "r" (o), "r" (n), "m" (v->value)
        : "cc", "memory");

    return (ret);
}

} // namespace oasys

#endif /* _OASYS_ATOMIC_PPC_H_ */
