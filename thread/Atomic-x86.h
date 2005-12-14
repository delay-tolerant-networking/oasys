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

#ifndef _OASYS_ATOMIC_X86_H_
#define _OASYS_ATOMIC_X86_H_

#include "../debug/DebugUtils.h"

namespace oasys {

/**
 * When we're not on a SMP platform, there's no need to lock the bus.
 */
#ifndef NO_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#endif

/*
 * We have to force gcc to not optimize with an alias.
 */
struct __noalias {
    u_int32_t value;
};

#define __noalias__(x) (*(volatile __noalias *)x)

/**
 * Atomic addition function.
 *
 * @param i	integer value to add
 * @param v	pointer to current value
 * 
 */
static inline void
atomic_add(volatile void *v, u_int32_t i)
{
    __asm__ __volatile__(
        LOCK "addl %1,%0"
        :"=m" (__noalias__(v))
        :"ir" (i), "m" (__noalias__(v)));
}

/**
 * Atomic subtraction function.
 *
 * @param i	integer value to subtract
 * @param v	pointer to current value
 
 */
static inline void
atomic_sub(volatile void *v, u_int32_t i)
{
    __asm__ __volatile__(
        LOCK "subl %1,%0"
        :"=m" (__noalias__(v))
        :"ir" (i), "m" (__noalias__(v)));
}

/**
 * Atomic increment.
 *
 * @param v	pointer to current value
 */
static inline void
atomic_incr(volatile void *v)
{
    __asm__ __volatile__(
        LOCK "incl %0"
        :"=m" (__noalias__(v))
        :"m" (__noalias__(v)));
}

/**
 * Atomic decrement.
 *
 * @param v	pointer to current value
 * 
 */ 
static inline void
atomic_decr(volatile void *v)
{
    __asm__ __volatile__(
        LOCK "decl %0"
        :"=m" (__noalias__(v))
        :"m" (__noalias__(v)));
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
atomic_decr_test(volatile void *v)
{
    unsigned char c;
    
    __asm__ __volatile__(
        LOCK "decl %0; sete %1"
        :"=m" (__noalias__(v)), "=qm" (c)
        :"m" (__noalias__(v)) : "memory");

    return (c != 0);
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
atomic_cmpxchg32(volatile void *v, u_int32_t o, u_int32_t n)
{
    __asm__ __volatile__(
	LOCK "cmpxchgl %1, %2"
	: "+a" (o)
	: "r" (n), "m" (__noalias__(v))
	: "memory");
    
    return o;
}

/**
 * Atomic increment function that returns the old value. Note that the
 * implementation loops until it can successfully do the operation and
 * store the value, so there is an extremely low chance that this will
 * never return.
 *
 * @param v 	pointer to current value
 */
static inline u_int32_t
atomic_incr_ret(volatile void* v)
{
    register u_int32_t o, n;
#if defined(NDEBUG) && NDEBUG == 1
    while (1)
#else
    register int j;
    for (j = 0; j < 1000000; ++j)
#endif
    {
        o = * (volatile u_int32_t*)(v);
        n = o + 1;
        if (atomic_cmpxchg32(v, o, n) == o)
            return n;
    }
    
    NOTREACHED;
    return 0;
}

/**
 * Atomic addition function that returns the old value. Note that the
 * implementation loops until it can successfully do the operation and
 * store the value, so there is an extremely low chance that this will
 * never return.
 *
 * @param v 	pointer to current value
 * @param i 	integer to add
 */
static inline u_int32_t
atomic_add_ret(volatile void* v, u_int32_t i)
{
    register u_int32_t o, n;
    
#if defined(NDEBUG) && NDEBUG == 1
    while (1)
#else
    register int j;
    for (j = 0; j < 1000000; ++j)
#endif
    {
        o = * (volatile u_int32_t*)(v);
        n = o + i;
        if (atomic_cmpxchg32(v, o, n) == o)
            return n;
    }
    
    NOTREACHED;
    return 0;
}

} // namespace oasys

#endif /* _OASYS_ATOMIC_X86_H_ */
