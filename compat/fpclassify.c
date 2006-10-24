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


#include "fpclassify.h"

#ifndef HAVE_FPCLASSIFY

int fpclassify(double x)
{
  switch (fpclass(x)) 
    {
    case FP_SNAN:
    case FP_QNAN: 
      return FP_NAN;
      break;
    case FP_NINF:
    case FP_PINF:
      return FP_INFINITE;
      break;
    case FP_NDENORM:
    case FP_PDENORM:
      return FP_SUBNORMAL;
      break;
    case FP_NZERO:
    case FP_PZERO:
      return FP_ZERO;
      break;
    default:
      return FP_NORMAL;
    }
}

#endif /* HAVE_FPCLASSIFY */
