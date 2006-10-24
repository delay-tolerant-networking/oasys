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


#ifndef _OASYS_ATOMIC_ARM_H_
#define _OASYS_ATOMIC_ARM_H_

#include "../compat/inttypes.h"

namespace oasys {

/**
 * The definition of atomic_t for arm requires a second word used for
 * the lock, since the only instruction we have to rely on is SWP.
 *
 * As such, all atomic operations call atomic_lock(), then execute the
 * operation, then atomic_unlock().
 */
struct atomic_t {
    atomic_t(u_int32_t v = 0)
        : value(v), lock(0) {}

    volatile u_int32_t value;
    volatile u_int32_t lock;
};

/**
 * Atomic lock function.
 */
static inline void
atomic_lock(volatile atomic_t* v)
{
    u_int32_t tmp;
    
    __asm__ __volatile__("@ atomic_lock\n"
        "1:	mov	%0, #1\n"	/* move 1 into r0 */
        "	swp     %0, %0, [%1]\n"	/* swap into lock location */
        "	teq	%0, #0\n"	/* test if we got it  */
        "	bne	1b"		/* jump if not */
        : "=&r" (tmp)
        : "r" (&v->lock)
        : "cc", "memory");
}

/**
 * Atomic try_lock function.
 */
static inline bool
atomic_try_lock(volatile atomic_t* v)
{
    u_int32_t tmp;
    
    __asm__ __volatile__("@ atomic_lock\n"
        "1:	mov	%0, #1\n"	/* move 1 into r0 */
        "	swp     %0, %0, [%1]\n"	/* swap into lock location */
        : "=&r" (tmp)
        : "r" (&v->lock)
        : "cc", "memory");

    return (tmp == 0);
}

static inline void
atomic_unlock(volatile atomic_t* v)
{
    v->lock = 0;
}

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
    u_int32_t ret;
    
    atomic_lock(v);
    v->value += i;
    ret = v->value;
    atomic_unlock(v);
    
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
    u_int32_t ret;
    
    atomic_lock(v);
    v->value -= i;
    ret = v->value;
    atomic_unlock(v);
    
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
    u_int32_t ret;
    
    atomic_lock(v);
    ret = v->value;
    if (v->value == o) {
        v->value = n;
    }
    atomic_unlock(v);
    
    return ret;
}

} // namespace oasys

#endif /* _OASYS_ATOMIC_ARM_H_ */
