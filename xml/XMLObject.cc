/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2006 Intel Corporation. All rights reserved. 
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

#include "XMLObject.h"
#include "util/StringBuffer.h"

namespace oasys {

//----------------------------------------------------------------------
XMLObject::XMLObject(const std::string& tag)
    : tag_(tag)
{
}

//----------------------------------------------------------------------
XMLObject::~XMLObject()
{
    Elements::iterator i;
    for (i = elements_.begin(); i != elements_.end(); ++i) {
        delete *i;
    }
}

//----------------------------------------------------------------------
void
XMLObject::add_attr(const std::string& attr, const std::string& val)
{
    attrs_.push_back(attr);
    attrs_.push_back(val);
}

//----------------------------------------------------------------------
void
XMLObject::add_proc_inst(const std::string& target,
                         const std::string& data)
{
    proc_insts_.push_back(target);
    proc_insts_.push_back(data);
}
    
//----------------------------------------------------------------------
void
XMLObject::add_element(XMLObject* child)
{
    elements_.push_back(child);
}

//----------------------------------------------------------------------
void
XMLObject::add_text(const char* text, size_t len)
{
    if (len == 0) {
        len = strlen(text);
    }
    
    text_.append(text, len);
}

//----------------------------------------------------------------------
void
XMLObject::to_string(StringBuffer* buf, int indent, int cur_indent) const
{
    static const char* space = "                                        "
                               "                                        ";
    
    buf->appendf("%.*s<%s", cur_indent, space, tag_.c_str());
    for (unsigned int i = 0; i < attrs_.size(); i += 2)
    {
        buf->appendf(" %s=\"%s\"", attrs_[i].c_str(), attrs_[i+1].c_str());
    }

    // shorthand for attribute-only tags
    if (proc_insts_.empty() && elements_.empty() && text_.size() == 0)
    {
        buf->appendf("/>");
        return;
    }
    else
    {
        buf->appendf(">%s", (indent == -1) ? "" : "\n");

    }
    
    for (unsigned int i = 0; i < proc_insts_.size(); i += 2)
    {
        buf->appendf("<?%s %s?>%s",
                     proc_insts_[i].c_str(), proc_insts_[i+1].c_str(),
                     (indent == -1) ? "" : "\n");
    }
    
    for (unsigned int i = 0; i < elements_.size(); ++i)
    {
        elements_[i]->to_string(buf, indent, (indent > 0) ? cur_indent + indent : 0);
    }

    buf->append(text_);

    buf->appendf("%.*s</%s>", cur_indent, space, tag_.c_str());
}

} // namespace oasys
