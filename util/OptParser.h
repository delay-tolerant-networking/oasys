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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _OASYS_OPTPARSER_H_
#define _OASYS_OPTPARSER_H_

#include <string>
#include <vector>
#include "Options.h"

namespace oasys {

/*
 * Utility class for parsing options from a single string or an array
 * of strings of the form var1=val1 var2=val2 ...
 */
class OptParser {
public:
    /**
     * Register a new option binding.
     */
    void addopt(Opt* opt);

    /**
     * Parse the given argument string, processing all registered
     * opts.
     *
     * Returns true if the argument string was successfully parsed,
     * false otherwise. If non-null, invalidp is set to point to the
     * invalid option string.
     */
    bool parse(const char* args, const char** invalidp = NULL);
    
    /**
     * Parse the given argument vector, processing all registered
     * opts.
     *
     * Returns true if the argument string was successfully parsed,
     * false otherwise. If non-null, invalidp is set to point to the
     * invalid option string.
     */
    bool parse(int argc, const char* const argv[],
               const char** invalidp = NULL);
    
protected:
    bool parse_opt(const char* opt, size_t len);
                  
    typedef std::vector<Opt*> OptList;
    OptList allopts_;
};

} // namespace oasys

#endif /* _OASYS_OPTPARSER_H_ */
