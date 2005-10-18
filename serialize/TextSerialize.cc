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

#include "TextSerialize.h"
#include "../util/TextCode.h"

namespace oasys {

//----------------------------------------------------------------------------
TextMarshal::TextMarshal(context_t         context,
                         ExpandableBuffer* buf,
                         int               options,
                         const char*       comment, 
                         int               indent_incr)
    : SerializeAction(Serialize::MARSHAL, context, options),
      indent_(0), 
      indent_incr_(indent_incr), 
      buf_(buf, false)
{
    buf_.append("# text marshal start\n");
    if (comment != 0) 
        buf_.append(comment);
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_int32_t* i)
{
    buf_.appendf("%s: %u\n", name, *i);
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_int16_t* i)
{
    buf_.appendf("%s: %u\n", name, (u_int32_t)*i);
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_int8_t* i)
{
    buf_.appendf("%s: %u\n", name, (u_int32_t)*i);
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, bool* b)
{
    buf_.appendf("%s: %s\n", name, (*b) ? "true" : "false" );
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_char* bp, size_t len)
{
    buf_.appendf("%s:\n", name);
    TextCode coder(reinterpret_cast<char*>(bp), len,
                   buf_.expandable_buf(), 40, 
                   (indent_ + 1) * indent_incr_);
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_char** bp, size_t* lenp, int flags)
{
    buf_.appendf("%s:\n", name);
    TextCode coder(reinterpret_cast<char*>(*bp), *lenp,
                   buf_.expandable_buf(), 40, 
                   (indent_ + 1) * indent_incr_);
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, std::string* s)
{
    buf_.appendf("%s:\n", name);
    TextCode coder(reinterpret_cast<const char*>(s->c_str()),
                   strlen(s->c_str()),
                   buf_.expandable_buf(), 
                   40, 
                   (indent_ + 1) * indent_incr_);
}

void 
TextMarshal::process(const char* name, SerializableObject* object)
{
    buf_.appendf("%s:\n", name);
    indent();
    object->serialize(this);
    unindent();
}


//----------------------------------------------------------------------------
void
TextMarshal::add_indent()
{
    for (int i=0; i<indent_; ++i)
        for (int j=0; j<indent_incr_; ++j) 
            buf_.append(' ');
}

//----------------------------------------------------------------------------
TextUnmarshal::TextUnmarshal(context_t context, u_char* buf, 
                             size_t length, int options)
    : SerializeAction(Serialize::UNMARSHAL, context, options)
{}
 
//----------------------------------------------------------------------------   
void 
TextUnmarshal::process(const char* name, u_int32_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_int16_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_int8_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, bool* b)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_char* bp, size_t len)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_char** bp, 
                       size_t* lenp, int flags)
{
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, std::string* s)
{
}

/*
//----------------------------------------------------------------------------
TextMarshalSize(
    context_t context,
    int       options,
    int       indent_incr
    ) : SerializeAction(Serialize::INFO, context, options), 
        indent_incr_(indent_incr)
{}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_int32_t* i)
{
    
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_int16_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_int8_t* i)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, bool* b)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_char* bp, size_t len)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, u_char** bp, 
                         size_t* lenp, int flags)
{
}

//----------------------------------------------------------------------------
void 
TextMarshalSize::process(const char* name, std::string* s)
{
}
*/

} // namespace oasys
