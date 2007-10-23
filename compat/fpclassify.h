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


#ifndef _OASYS_FPCLASSIFY_H_
#define _OASYS_FPCLASSIFY_H_

#ifndef OASYS_CONFIG_STATE
#error "MUST INCLUDE config.h before including this file"
#endif

/**
 * Compatibility header for missing fpclassify. Thanks to Larry Pyatt
 * for the implementation.
 */

#ifdef HAVE_FPCLASSIFY
#include <math.h>
#else

#ifdef __cplusplus
extern "C" {
#endif

#include <ieeefp.h>

// fpclassify returns one of the following:
// FP_NAN : x is "Not a Number". 
// FP_INFINITE : x is either plus or minus infinity. 
// FP_ZERO : x is zero. 
// FP_SUBNORMAL : x is too small to be represented in normalized format. 
// FP_NORMAL : a normal floating-point number. 
//
// fpclass gives more information by returning one of the following:
// FP_SNAN : signaling NaN
// FP_QNAN : quiet NaN
// FP_NINF : negative infinity
// FP_PINF : positive infinity
// FP_NDENORM : negative denormalized non-zero
// FP_PDENORM : positive denormalized non-zero
// FP_NZERO : negative zero
// FP_PZERO : positive zero
// FP_NNORM : negative normalized non-zero
// FP_PNORM : positive normalized non-zero

// So, it should be easy to write fpclassify using fpclass.
// Here goes!

#define FP_NAN       1
#define FP_INFINITE  2
#define FP_ZERO      3
#define FP_SUBNORMAL 4
#define FP_NORMAL    5

int fpclassify(double x);

//These other macros provide a short answer to some standard questions. 
#define isfinite(x) (fpclassify(x) != FP_NAN && fpclassify(x) != FP_INFINITE) 
     
#define isnormal(x) (fpclassify(x) == FP_NORMAL) 
     
#define isnan(x) (fpclassify(x) == FP_NAN) 
     
#define isinf(x) (fpclassify(x) == FP_INFINITE)

#ifdef __cplusplus
}
#endif

#endif /* HAVE_FPCLASSIFY */

#endif /* _OASYS_FPCLASSIFY_H_ */
