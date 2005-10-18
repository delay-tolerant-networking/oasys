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
                         const char*       comment)
    : SerializeAction(Serialize::MARSHAL, context, options),
      indent_(0), 
      buf_(buf, false)
{
    buf_.append("# -- text marshal start --\n");
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
    buf_.appendf("%s: TextCode\n", name);
    TextCode coder(reinterpret_cast<char*>(bp), len,
                   buf_.expandable_buf(), 40, indent_ + 1);
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, u_char** bp, size_t* lenp, int flags)
{
    buf_.appendf("%s: TextCode\n", name);
    TextCode coder(reinterpret_cast<char*>(*bp), *lenp,
                   buf_.expandable_buf(), 40, indent_ + 1);
}

//----------------------------------------------------------------------------
void 
TextMarshal::process(const char* name, std::string* s)
{
    buf_.appendf("%s: TextCode\n", name);
    TextCode coder(reinterpret_cast<const char*>(s->c_str()),
                   strlen(s->c_str()),
                   buf_.expandable_buf(), 
                   40, indent_ + 1);
}

void 
TextMarshal::process(const char* name, SerializableObject* object)
{
    buf_.appendf("%s: SerializableObject\n", name);
    indent();
    object->serialize(this);
    unindent();
}


//----------------------------------------------------------------------------
void
TextMarshal::add_indent()
{
    for (int i=0; i<indent_; ++i)
        buf_.append('\t');
}

//----------------------------------------------------------------------------
TextUnmarshal::TextUnmarshal(context_t context, u_char* buf, 
                             size_t length, int options)
    : SerializeAction(Serialize::UNMARSHAL, context, options),
      buf_(reinterpret_cast<char*>(buf)), 
      length_(length), 
      cur_(reinterpret_cast<char*>(buf))
{}
 
//----------------------------------------------------------------------------   
void 
TextUnmarshal::process(const char* name, u_int32_t* i)
{
    if (error_) 
        return;

    u_int32_t num;
    int err = get_num(name, &num);
    
    if (err != 0) 
        return;

    *i = num;
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_int16_t* i)
{
    if (error_) 
        return;

    u_int32_t num;
    int err = get_num(name, &num);
    
    if (err != 0) 
        return;

    *i = static_cast<u_int16_t>(num);
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_int8_t* i)
{
    if (error_) 
        return;

    u_int32_t num;
    int err = get_num(name, &num);
    
    if (err != 0) 
        return;

    *i = static_cast<u_int8_t>(num);
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, bool* b)
{
    if (error_) 
        return;
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_char* bp, size_t len)
{
    if (error_) 
        return;
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, u_char** bp, 
                       size_t* lenp, int flags)
{
    if (error_) 
        return;
}

//----------------------------------------------------------------------------
void 
TextUnmarshal::process(const char* name, std::string* s)
{
    if (error_) 
        return;
}

bool 
TextUnmarshal::is_within_buf(size_t offset)
{
    return cur_ + offset < buf_ + length_;
}

int  
TextUnmarshal::get_line(char** end)
{
  again:    
    size_t offset = 0;
    while (is_within_buf(offset) && cur_[offset] != '\n') {
        ++offset;
    }

    if (!is_within_buf(offset)) {
        return -1;
    }

    // comment, skip to next line
    if (*cur_ == '#') {
        cur_ = &cur_[offset] + 1;
        goto again;
    }
    
    *end = &cur_[offset];

    return 0;
}

int
TextUnmarshal::get_num(const char* field_name, u_int32_t* num)
{
    // expecting {\t}* field_name: num\n
    char* eol;
    if (get_line(&eol) != 0) {
        error_ = true;
        return -1;
    }

    ASSERT(*eol == '\n');
    
    char* field_name_ptr = 0;
    while (is_within_buf(0) && *cur_ != ':') {
        if (*cur_ != '\t' && *cur_ != ' ' && field_name_ptr == 0)
            field_name_ptr = cur_;
        ++cur_;
    }
    
    if (*cur_ != ':' || cur_ > eol) {
        error_ = true;
        return -1;
    }

    if (memcmp(field_name_ptr, field_name, strlen(field_name)) != 0) {
        error_ = true;
        return -1;
    }

    cur_ += 2;
    if (!is_within_buf(0)) {
        error_ = true;
        return -1;
    }

    *num = strtoul(cur_, &eol, 0);
    ASSERT(*eol == '\n');
    
    cur_ = eol + 1;
    
    return 0;
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
