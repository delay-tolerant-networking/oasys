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
#include "OptParser.h"

namespace oasys {

OptParser::~OptParser()
{
    for (u_int i = 0; i < allopts_.size(); ++i)
    {
        delete allopts_[i];
    }
    allopts_.clear();
}

void
OptParser::addopt(Opt* opt)
{
    allopts_.push_back(opt);
}

bool
OptParser::parse_opt(const char* opt_str, size_t len, bool* invalid_value)
{
    Opt* opt;
    const char* val_str;
    size_t opt_len, val_len;

    if (invalid_value) {
        *invalid_value = false;
    }

    opt_len = strcspn(opt_str, "= \t\r\n");
    if (opt_len == 0 || opt_len > len) {
        return false;
    }

    if (opt_str[opt_len] != '=') {
        val_str = NULL;
        val_len = 0;
    } else {
        val_str = opt_str + opt_len + 1;
        val_len = strcspn(val_str, " \t\r\n");
        if (val_len == 0 || (val_len + opt_len + 1) != len) {
            return false;
        }
    }
    
    int nopts = allopts_.size(); 
    for (int i = 0; i < nopts; ++i)
    {
        opt = allopts_[i];
        
        if (strncmp(opt_str, opt->longopt_, opt_len) == 0)
        {
            if (opt->needval_ && (val_str == NULL)) {
                if (invalid_value) {
                    *invalid_value = true;
                }
                return false; // missing value
            }

            if (opt->set(val_str, val_len) != 0) {
                if (invalid_value) {
                    *invalid_value = true;
                }
                return false; // error in set
            }
            
            return true; // all set
        }
    }

    return false; // no matching option
}

bool
OptParser::parse(const char* args, const char** invalidp)
{
    const char* opt;
    size_t opt_len;

    opt = args;
    while (1) {
        opt_len = strcspn(opt, " \t\r\n");
        if (opt_len == 0) {
            return true; // all done
        }

        if (parse_opt(opt, opt_len) == false) {
            *invalidp = opt;
            return false;
        }

        // skip past the arg and all other whitespace
        opt = opt + opt_len;
        opt += strspn(opt, " \t\r\n");
    }
}

bool
OptParser::parse(int argc, const char* const argv[], const char** invalidp)
{
    for (int i = 0; i < argc; ++i) {
        if (parse_opt(argv[i], strlen(argv[i])) == false) {
            *invalidp = argv[i];
            return false;
        }
    }

    return true;
}


int
OptParser::parse_and_shift(int argc, const char* argv[], const char** invalidp)
{
    int last_slot = 0;
    int valid_count = 0;
    bool invalid_value;

    for (int i = 0; i < argc; ++i) {
        
        if (parse_opt(argv[i], strlen(argv[i]), &invalid_value) == true) {
            ++valid_count;
            
        } else if (invalid_value) {
            if (invalidp)
                *invalidp = argv[0];
            
            return -1;
            
        } else {
            argv[last_slot] = argv[i];
            last_slot++;
        }
    }
    
    return valid_count;
}

bool
OptParser::parse(const std::vector<std::string>& args,
                 const char** invalidp)
{
    std::vector<std::string>::const_iterator iter;
    for (iter = args.begin(); iter != args.end(); ++iter) {
        if (parse_opt(iter->c_str(), iter->length()) == false) {
            *invalidp = iter->c_str();
            return false;
        }
    }

    return true;
}


} // namespace oasys
