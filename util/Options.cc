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
#include <stdio.h>
#include <unistd.h>

#include "Options.h"

namespace oasys {

Opt::Opt(char shortopt, const char* longopt,
         void* valp, bool* setp, bool hasval,
         const char* valdesc, const char* desc)
    
    : shortopt_(shortopt),
      longopt_(longopt),
      valp_(valp),
      setp_(setp),
      hasval_(hasval),
      valdesc_(valdesc),
      desc_(desc),
      next_(0)
{
    if (setp) *setp = false;
}

Opt::~Opt()
{
}

BoolOpt::BoolOpt(const char* opt, bool* valp,
                 const char* desc, bool* setp)
    : Opt(0, opt, valp, setp, false, "", desc)
{
}

BoolOpt::BoolOpt(char shortopt, const char* longopt, bool* valp,
                 const char* desc, bool* setp)
    : Opt(shortopt, longopt, valp, setp, false, "", desc)
{
}

int
BoolOpt::set(const char* val, size_t len)
{
    *((bool*)valp_) = true;

    if (setp_)
        *setp_ = true;

    return 0;
}

IntOpt::IntOpt(const char* opt, int* valp,
               const char* valdesc, const char* desc, bool* setp)
    : Opt(0, opt, valp, setp, true, valdesc, desc)
{
}

IntOpt::IntOpt(char shortopt, const char* longopt, int* valp,
               const char* valdesc, const char* desc, bool* setp)
    : Opt(shortopt, longopt, valp, setp, true, valdesc, desc)
{
}

int
IntOpt::set(const char* val, size_t len)
{
    int newval;
    char* endptr = 0;

    newval = strtol(val, &endptr, 0);
    if (endptr != (val + len))
        return -1;
            
    *((int*)valp_) = newval;
    
    if (setp_)
        *setp_ = true;
    
    return 0;
}

UIntOpt::UIntOpt(const char* opt, u_int* valp,
                 const char* valdesc, const char* desc, bool* setp)
    : Opt(0, opt, valp, setp, true, valdesc, desc)
{
}

UIntOpt::UIntOpt(char shortopt, const char* longopt, u_int* valp,
                 const char* valdesc, const char* desc, bool* setp)
    : Opt(shortopt, longopt, valp, setp, true, valdesc, desc)
{
}

int
UIntOpt::set(const char* val, size_t len)
{
    u_int newval;
    char* endptr = 0;

    newval = strtoul(val, &endptr, 0);
    if (endptr != (val + len))
        return -1;
            
    *((u_int*)valp_) = newval;
    
    if (setp_)
        *setp_ = true;
    
    return 0;
}

StringOpt::StringOpt(const char* opt, std::string* valp,
                     const char* valdesc, const char* desc, bool* setp)
    : Opt(0, opt, valp, setp, true, valdesc, desc)
{
}

StringOpt::StringOpt(char shortopt, const char* longopt, std::string* valp,
                     const char* valdesc, const char* desc, bool* setp)
    : Opt(shortopt, longopt, valp, setp, true, valdesc, desc)
{
}

int
StringOpt::set(const char* val, size_t len)
{
    ((std::string*)valp_)->assign(val, len);

    if (setp_)
        *setp_ = true;
    
    return 0;
}

} // namespace oasys
