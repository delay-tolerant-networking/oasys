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

#ifndef _OASYS_DEBUG_UTILS_H_
#define _OASYS_DEBUG_UTILS_H_

#include <cstdio>
#include <cstdlib>

#include "FatalSignals.h"

#define ASSERT(x)                                                       \
    do { if (! (x)) {                                                   \
        fprintf(stderr, "ASSERTION FAILED (" #x ") at %s:%d\n",         \
                __FILE__, __LINE__);                                    \
        ::oasys::Breaker::break_here();                                 \
        ::oasys::FatalSignals::die();                                   \
    } } while(0);

#ifdef __GNUC__

#define ASSERTF(x, fmt, args...)                                        \
    do { if (! (x)) {                                                   \
        fprintf(stderr,                                                 \
                "ASSERTION FAILED (" #x ") at %s:%d: " fmt "\n",        \
                __FILE__, __LINE__, ## args);                           \
        ::oasys::Breaker::break_here();                                 \
        ::oasys::FatalSignals::die();                                   \
    } } while(0);

#define PANIC(fmt, args...)                                             \
    do {                                                                \
        fprintf(stderr, "PANIC at %s:%d: " fmt "\n",                    \
                __FILE__, __LINE__ , ## args);                          \
        ::oasys::Breaker::break_here();                                 \
        ::oasys::FatalSignals::die();                                   \
    } while(0);

#endif // __GNUC__

#ifdef __win32__

#define ASSERTF(x, fmt, ...)                                            \
    do { if (! (x)) {                                                   \
        fprintf(stderr,                                                 \
                "ASSERTION FAILED (" #x ") at %s:%d: " fmt "\n",        \
                __FILE__, __LINE__, __VA_ARGS__ );                      \
        ::oasys::Breaker::break_here();                                 \
        ::oasys::FatalSignals::die();                                   \
    } } while(0);

#define PANIC(fmt, ...)                                                 \
    do {                                                                \
        fprintf(stderr, "PANIC at %s:%d: " fmt "\n",                    \
                __FILE__, __LINE__ , __VA_ARGS__);                      \
        ::oasys::Breaker::break_here();                                 \
        ::oasys::FatalSignals::die();                                   \
    } while(0);

#endif __win32__

#define NOTREACHED                                                      \
    do {                                                                \
        fprintf(stderr, "NOTREACHED REACHED at %s:%d\n",                \
                __FILE__, __LINE__);                                    \
        ::oasys::Breaker::break_here();                                 \
        ::oasys::FatalSignals::die();                                   \
    } while(0);

#define NOTIMPLEMENTED                                                  \
    do {                                                                \
        fprintf(stderr, "%s NOT IMPLEMENTED at %s:%d\n",                \
                __FUNCTION__, __FILE__, __LINE__);                      \
        ::oasys::Breaker::break_here();                                 \
        ::oasys::FatalSignals::die();                                   \
    } while(0);

/** @{ 
 * Compile time static checking (better version, from Boost)
 * Take the sizeof() of an undefined template, which will die.
 */
template <int x> struct static_assert_test{};
template <bool>  struct STATIC_ASSERTION_FAILURE;
template <>      struct STATIC_ASSERTION_FAILURE<true>{};

#ifdef STATIC_ASSERT
#undef STATIC_ASSERT
#endif

#define STATIC_ASSERT(_x, _what)                   \
    typedef static_assert_test                     \
    <                                              \
	sizeof(STATIC_ASSERTION_FAILURE<(bool)(_x)>) \
    > static_assert_typedef_ ## _what;
/** @} */

namespace oasys {
class Breaker {
public:
    /** 
     * This is a convenience method for setting a breakpoint here, in
     * case the stack is completely corrupted by the time the program
     * hits abort(). (Seems completely bogus, but it happens.)
     */
    static void break_here();
};

/*!
 * Make sure to null out after deleting an object. USE THIS.
 */
#define delete_z(_obj)                          \
    do { delete _obj; _obj = 0; } while (0)

/*!
 * @{ Macros to define but not declare copy and assignment operator to
 * avoid accidently usage of such constructs. Put this at the top of
 * the class declaration.
 */
#define NO_COPY(_Classname)                                     \
    private: _Classname(const _Classname& other)

#define NO_ASSIGN(_Classname)                                   \
    private: _Classname& operator=(const _Classname& other)

#define NO_ASSIGN_COPY(_Classname)              \
   NO_COPY(_Classname);                         \
   NO_ASSIGN(_Classname)
//! @}

} // namespace oasys

#include "../memory/Memory.h"

#endif /* _OASYS_DEBUG_UTILS_H_ */
