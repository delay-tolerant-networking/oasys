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
 * (including NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OASYS_FPCLASSIFY_H_
#define _OASYS_FPCLASSIFY_H_

/**
 * Compatibility header for missing fpclassify. Thanks to Larry Pyatt
 * for the implementation.
 */
#include "config.h"

#ifndef HAVE_FPCLASSIFY

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
