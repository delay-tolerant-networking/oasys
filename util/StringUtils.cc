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

#include "debug/Log.h"
#include "StringUtils.h"

namespace oasys {

int
tokenize(const std::string& str,
         const std::string& sep,
         std::vector<std::string>* tokens)
{
    size_t start, end;

    tokens->clear();

    start = str.find_first_not_of(sep);
    if (start == std::string::npos || start == str.length()) {
        return 0; // nothing to do
    }
    
    while (1) {
        end = str.find_first_of(sep, start);
        if (end == std::string::npos) {
            end = str.length();
        }
        
        tokens->push_back(str.substr(start, end - start));
        
        if (end == str.length()) {
            break; // all done
        }
        
        start = str.find_first_not_of(sep, end);
        if (start == std::string::npos) {
            break; // all done
        }
    }

    return tokens->size();
}

void
StringSet::dump(const char* log) const
{
    logf(log, LOG_DEBUG, "dumping string set...");
    for (iterator i = begin(); i != end(); ++i) {
        logf(log, LOG_DEBUG, "\t%s", i->c_str());
    }
}

void
StringHashSet::dump(const char* log) const
{
    logf(log, LOG_DEBUG, "dumping string set...");
    for (iterator i = begin(); i != end(); ++i) {
        logf(log, LOG_DEBUG, "\t%s", i->c_str());
    }
}

//----------------------------------------------------------------------------
const char*
bool_to_str(bool b)
{
    if (b) 
    {
        return "true";
    }
    
    return "false";
}

//----------------------------------------------------------------------------
const char*
str_if(bool b, const char* true_str, const char* false_str)
{
    if (b) 
    {
        return true_str;
    }
    else
    {
        return false_str;
    }
}

} // namespace oasys
