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

#include "PrettyPrintBuffer.h"
#include "util/StringBuffer.h"

namespace oasys {

const int PrettyPrintBuf::MAX_COL = 40;

//----------------------------------------------------------------------------
PrettyPrintBuf::PrettyPrintBuf(const char* buf, int len)
    : buf_(buf), cur_(0), len_(len)
{
    if(len_ == -1)
    {
        len_ = strlen(buf);
    }
}

//----------------------------------------------------------------------------
std::pair<std::string, bool>
PrettyPrintBuf::next_str()
{
    StringBuffer buf;

    int bound = std::min(cur_ + MAX_COL, len_);
    for(int i = cur_; i<bound; ++i, ++cur_)
    {
        switch(buf_[i])
        { 
        case '\n': buf.append("\\n"); break;
        case '\r': buf.append("\\r"); break;
        case '\t': buf.append("\\t"); break;
        case '\0': buf.append("\\0"); break;

        default:
            buf.append(buf_[i]);
        }
    }

    bool full = (bound == len_) ? true : false;
    return std::pair<std::string, bool>(buf.c_str(), !full);
}

} // namespace oasys
