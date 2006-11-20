/*
 *    Copyright 2005-2006 Intel Corporation
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

#ifndef _OASYS_INTTYPES_H_
#define _OASYS_INTTYPES_H_

#include "config.h"

//----------------------------------------------------------------------------
/*
 * Microsoft C/C++ integer types.
 */
#ifdef __win32__

typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;

typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#else
/*
 * Unix integer types.
 */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/*
 * See if we can define u_intxx_t in terms of uintxx_t.
 */
#ifndef HAVE_U_INT32_T

#ifdef HAVE_UINT32_T
#define u_int64_t uint64_t
#define u_int32_t uint32_t
#define u_int16_t uint16_t
#define u_int8_t  uint8_t
#else
#error need appropriate types for u_intxx_t verai
#endif

#endif /* HAVE_U_INT32_T */

#endif

/**
 * On 64 bit platforms, a u_int64_t can't be formatted as %llu since
 * it's not a long long int. This macro adds a cast where needed.
 */
#ifdef __amd64
#define U64FMT(x) static_cast<long long unsigned int>(x)
#else
#define U64FMT(x) x
#endif

#endif /* _OASYS_INTTYPES_H_ */
