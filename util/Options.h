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
#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>
#include <vector>

namespace oasys {

/**
 * Base class for options. These can be used either with the Getopt
 * class for parsing argv-style declarations or with the OptParser
 * class for parsing argument strings or arrays of strings.
 */
class Opt {
    friend class Getopt;
    friend class OptParser;
    
protected:
    /**
     * Private constructor.
     */
    Opt(char shortopt, const char* longopt,
        void* valp, bool* setp, bool hasval,
        const char* valdesc, const char* desc);
    virtual ~Opt();

    /**
     * Virtual callback to set the option to the given string value.
     */
    virtual int set(const char* val, size_t len) = 0;
    
    char shortopt_;
    const char* longopt_;
    void* valp_;
    bool* setp_;
    bool hasval_;
    const char* valdesc_;
    const char* desc_;
    Opt*  next_;
};

/**
 * Boolean option class.
 */
class BoolOpt : public Opt {
public:
    /**
     * Basic constructor.
     *
     * @param opt   the option string
     * @param valp  pointer to the value
     * @param desc  descriptive string
     * @param setp  optional pointer to indicate whether or not
                    the option was set
     */
    BoolOpt(const char* opt, bool* valp,
            const char* desc = "", bool* setp = NULL);

    /**
     * Alternative constructor with both short and long options,
     * suitable for getopt calls.
     *
     * @param shortopt  short option character
     * @param longopt   long option string
     * @param valp      pointer to the value
     * @param desc      descriptive string
     * @param setp      optional pointer to indicate whether or not
                        the option was set
     */
    BoolOpt(char shortopt, const char* longopt, bool* valp,
            const char* desc = "", bool* setp = NULL);

protected:
    int set(const char* val, size_t len);
};

/**
 * Integer option class.
 */
class IntOpt : public Opt {
public:
    /**
     * Basic constructor.
     *
     * @param opt     the option string
     * @param valp    pointer to the value
     * @param valdesc short description for the value 
     * @param desc    descriptive string
     * @param setp    optional pointer to indicate whether or not
                      the option was set
     */
    IntOpt(const char* opt, int* valp,
           const char* valdesc = "", const char* desc = "",
           bool* setp = NULL);
    
    /**
     * Alternative constructor with both short and long options,
     * suitable for getopt calls.
     *
     * @param shortopt  short option character
     * @param longopt   long option string
     * @param valp      pointer to the value
     * @param desc      descriptive string
     * @param setp      optional pointer to indicate whether or not
                        the option was set
     */
    IntOpt(char shortopt, const char* longopt, int* valp,
           const char* valdesc = "", const char* desc = "",
           bool* setp = NULL);
    
protected:
    int set(const char* val, size_t len);
};

/**
 * Unsigned integer option class.
 */
class UIntOpt : public Opt {
public:
    /**
     * Basic constructor.
     *
     * @param opt     the option string
     * @param valp    pointer to the value
     * @param valdesc short description for the value 
     * @param desc    descriptive string
     * @param setp    optional pointer to indicate whether or not
                      the option was set
     */
    UIntOpt(const char* opt, u_int* valp,
            const char* valdesc = "", const char* desc = "",
            bool* setp = NULL);
    
    /**
     * Alternative constructor with both short and long options,
     * suitable for getopt calls.
     *
     * @param shortopt  short option character
     * @param longopt   long option string
     * @param valp      pointer to the value
     * @param desc      descriptive string
     * @param setp      optional pointer to indicate whether or not
                        the option was set
     */
    UIntOpt(char shortopt, const char* longopt, u_int* valp,
            const char* valdesc = "", const char* desc = "",
            bool* setp = NULL);
    
protected:
    int set(const char* val, size_t len);
};

/**
 * String option class.
 */
class StringOpt : public Opt {
public:
    /**
     * Basic constructor.
     *
     * @param opt     the option string
     * @param valp    pointer to the value
     * @param valdesc short description for the value 
     * @param desc    descriptive string
     * @param setp    optional pointer to indicate whether or not
                      the option was set
     */
    StringOpt(const char* opt, std::string* valp,
              const char* valdesc = "", const char* desc = "",
              bool* setp = NULL);
    
    /**
     * Alternative constructor with both short and long options,
     * suitable for getopt calls.
     *
     * @param shortopt  short option character
     * @param longopt   long option string
     * @param valp      pointer to the value
     * @param desc      descriptive string
     * @param setp      optional pointer to indicate whether or not
                        the option was set
     */
    StringOpt(char shortopt, const char* longopt, std::string* valp,
              const char* valdesc = "", const char* desc = "",
              bool* setp = NULL);

protected:
    int set(const char* val, size_t len);
};

} // namespace oasys

#endif /* _OPTIONS_H_ */
