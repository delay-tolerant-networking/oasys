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

/*
 * Wrapper class for getopt calls.
 */
class Opt;
class Options {
public:
    /**
     * Register a new option binding.
     */
    static void addopt(Opt* opt);

    /**
     * Parse argv, processing all registered options. Returns the
     * index of the first non-option argument in argv
     */
    static int getopt(const char* progname, int argc, char* const argv[]);

    /**
     * Prints a nicely formatted usage string to stderr.
     */
    static void usage(const char* progname);
    
protected:
    typedef std::vector<Opt*> List;
    
    static Opt* opts_[];	// indexed by option character
    static List allopts_;	// list of all options
};

class Opt {
    friend class Options;
    
protected:
    Opt(char shortopt, const char* longopt,
        void* valp, bool* setp, bool hasval,
        const char* valdesc, const char* desc);
    virtual ~Opt();
    
    virtual int set(char* val) = 0;
    
    char shortopt_;
    const char* longopt_;
    void* valp_;
    bool* setp_;
    bool hasval_;
    const char* valdesc_;
    const char* desc_;
    Opt*  next_;
};

class BoolOpt : public Opt {
public:
    BoolOpt(char shortopt, const char* longopt,
            bool* valp, const char* desc);
    BoolOpt(char shortopt, const char* longopt,
            bool* valp, bool* setp, const char* desc);

protected:
    int set(char* val);
};

class IntOpt : public Opt {
public:
    IntOpt(char shortopt, const char* longopt,
           int* valp,
           const char* valdesc, const char* desc);

    IntOpt(char shortopt, const char* longopt,
           int* valp, bool* setp,
           const char* valdesc, const char* desc);

protected:
    int set(char* val);
};

class StringOpt : public Opt {
public:
    StringOpt(char shortopt, const char* longopt,
              std::string* valp,
              const char* valdesc, const char* desc);
    
    StringOpt(char shortopt, const char* longopt,
              std::string* valp, bool* setp,
              const char* valdesc, const char* desc);

protected:
    int set(char* val);
};

} // namespace oasys

#endif /* _OPTIONS_H_ */
