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

#ifndef _OASYS_DEBUG_H_
#define _OASYS_DEBUG_H_

#include <stdlib.h>

#include "Log.h"

#define ASSERT(x)                                               \
    do { if (! (x)) {                                           \
        oasys::logf("/assert", oasys::LOG_CRIT,                 \
                    "ASSERTION FAILED (" #x ") at %s:%d\n",     \
                    __FILE__, __LINE__);                        \
        abort();                                                \
    } } while(0)

#define PANIC(fmt, args...)                                             \
    do {                                                                \
       oasys::logf("/panic", oasys::LOG_CRIT, "PANIC at %s:%d: " fmt,   \
                   __FILE__, __LINE__ , ## args);                       \
        abort();                                                        \
    } while(0)

#define NOTREACHED                                      \
    do { oasys::logf("/assert", oasys::LOG_CRIT,        \
                     "NOTREACHED REACHED at %s:%d\n",   \
                     __FILE__, __LINE__);               \
        abort();                                        \
    } while(0);

#define NOTIMPLEMENTED                                  \
    do { oasys::logf("/assert", oasys::LOG_CRIT,        \
                     "%s NOT IMPLEMENTED at %s:%d\n",   \
                     __FUNCTION__, __FILE__, __LINE__); \
        abort();                                        \
    } while(0);

/** @{ 
 * Compile time static checking (better version, from Boost)
 * Take the sizeof() of an undefined template, which will die.
 */
template <int x> struct static_assert_test{};
template <bool>  struct STATIC_ASSERTION_FAILURE;
template <>      struct STATIC_ASSERTION_FAILURE<true>{};

#define STATIC_ASSERT(_x, _what)                   \
    typedef static_assert_test                     \
    <                                              \
	sizeof(STATIC_ASSERTION_FAILURE<(bool)(_x)>) \
    > static_assert_typedef_ ## _what;
/** @} */

#include "../memory/Memory.h"

#endif /* _OASYS_DEBUG_H_ */
