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

#include "config.h"

#ifdef OASYS_ATOMIC_MUTEX

#include "Atomic-mutex.h"
#include "Mutex.h"

namespace oasys {

/**
 * To implement atomic operations without assembly support at
 * userland, we rely on a single global mutex.
 */
Mutex g_atomic_mutex("/XXX/ATOMIC_MUTEX_UNUSED_LOGGER",
                     Mutex::TYPE_FAST,
                     true /* keep_quiet */);

/**
 * Global accessor to the singleton atomic mutex.
 */
Mutex* atomic_mutex() { return &g_atomic_mutex; }

//----------------------------------------------------------------------
void
atomic_add(volatile atomic_t *v, u_int32_t i)
{
    ScopeLock l(atomic_mutex(), "atomic_add");
    v->value += i;
}

//----------------------------------------------------------------------
void
atomic_sub(volatile atomic_t* v, u_int32_t i)
{
    ScopeLock l(atomic_mutex(), "atomic_sub");
    v->value -= i;
}

//----------------------------------------------------------------------
void
atomic_incr(volatile atomic_t* v)
{
    ScopeLock l(atomic_mutex(), "atomic_incr");
    v->value++;
}

//----------------------------------------------------------------------
void
atomic_decr(volatile atomic_t* v)
{
    ScopeLock l(atomic_mutex(), "atomic_decr");
    v->value--;
}

//----------------------------------------------------------------------
bool
atomic_decr_test(volatile atomic_t* v)
{
    ScopeLock l(atomic_mutex(), "atomic_decr_test");
    v->value--;
    return (v->value == 0);
}

//----------------------------------------------------------------------
u_int32_t
atomic_cmpxchg32(volatile atomic_t* v, u_int32_t o, u_int32_t n)
{
    ScopeLock l(atomic_mutex(), "atomic_cmpxchg32");
    u_int32_t ret = v->value;

    if (v->value == o) {
        v->value = n;
    }

    return ret;
}

//----------------------------------------------------------------------
u_int32_t
atomic_incr_ret(volatile atomic_t* v)
{
    ScopeLock l(atomic_mutex(), "atomic_incr_ret");
    v->value++;
    return v->value;
}

//----------------------------------------------------------------------
u_int32_t
atomic_add_ret(volatile atomic_t* v, u_int32_t i)
{
    ScopeLock l(atomic_mutex(), "atomic_add_ret");
    v->value += i;
    return v->value;
}

} // namespace oasys

#endif /* OASYS_ATOMIC_MUTEX */
