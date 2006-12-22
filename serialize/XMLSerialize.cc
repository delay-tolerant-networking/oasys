/*
 * License Agreement
 * 
 * NOTICE
 * This software (or technical data) was produced for the U. S.
 * Government under contract W15P7T-05-C-F600, and is
 * subject to the Rights in Data-General Clause 52.227-14 (JUNE 1987)
 * 
 * Copyright (C) 2006. The MITRE Corporation (http://www.mitre.org/).
 * All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * * The US Government will not be charged any license fee and/or
 * royalties related to this software.
 * 
 * * Neither name of The MITRE Corporation; nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "XMLSerialize.h"
#include <io/NetUtils.h>

#include <config.h>
#ifdef XERCES_C_ENABLED
#include <xercesc/util/Base64.hpp>
#include <xercesc/util/XMLString.hpp>
#endif

namespace oasys {

XMLMarshal::XMLMarshal(ExpandableBuffer *buf, const char *root_tag)
    : SerializeAction(Serialize::MARSHAL, Serialize::CONTEXT_UNKNOWN),
      buf_(buf, false)
{
    XMLObject *root_node = new XMLObject(root_tag);
    doc_.set_root(root_node);
    current_node_ = root_node;
}

void
XMLMarshal::end_action()
{
    doc_.to_string( &buf_, -1 );
}

void
XMLMarshal::process(const char *name, SerializableObject* object)
{
    if (! object) return;

    XMLObject *parent_node = current_node_;
    XMLObject *new_node = new XMLObject(name);
    current_node_->add_element(new_node);
    current_node_ = new_node;

    object->serialize(this);

    current_node_ = parent_node;
}

void
XMLMarshal::process(const char *name, u_int64_t *i)
{
    StringBuffer buf;
    buf.appendf("%llu", U64FMT(*i));
    current_node_->add_attr(name, std::string(buf.data()));
}

void
XMLMarshal::process(const char *name, u_int32_t *i)
{
    StringBuffer buf;
    buf.appendf("%u", *i);
    current_node_->add_attr(name, std::string(buf.data()));
}

void
XMLMarshal::process(const char *name, u_int16_t *i)
{
    StringBuffer buf;
    buf.appendf("%hu", *i);
    current_node_->add_attr(name, std::string(buf.data()));
}

void
XMLMarshal::process(const char *name, u_int8_t *i)
{
    StringBuffer buf;
    buf.appendf("%hhu", *i);
    current_node_->add_attr(name, std::string(buf.data()));
}

void
XMLMarshal::process(const char *name, bool *b)
{
    *b ?
        current_node_->add_attr(std::string(name), std::string("true")) :
        current_node_->add_attr(std::string(name), std::string("false"));
}

void
XMLMarshal::process(const char *name, u_char *bp, u_int32_t len)
{
#ifdef XERCES_C_ENABLED
    unsigned int elen;
    XMLByte *estr = xercesc::Base64::encode(bp, len, &elen);
    current_node_->add_attr(std::string(name),
        std::string(reinterpret_cast<char *>(estr), elen));
    xercesc::XMLString::release(&estr);
#else
    (void) name;
    (void) bp;
    (void) len;
    
    signal_error();
#endif
}

void
XMLMarshal::process(const char *name, u_char **bp,
                    u_int32_t *lenp, int flags)
{
    (void) name;

    ASSERT(! (lenp == 0 && ! (flags & Serialize::NULL_TERMINATED)));
    
    size_t len;
    if (flags & Serialize::NULL_TERMINATED) 
    {
        len = strlen(reinterpret_cast<char *>(*bp));
    } 
    else 
    {
        len = *lenp;
    }

#ifdef XERCES_C_ENABLED
    unsigned int elen;
    XMLByte *estr = xercesc::Base64::encode(*bp, len, &elen);
    current_node_->add_attr(std::string(name),
        std::string(reinterpret_cast<char *>(estr), elen));
    xercesc::XMLString::release(&estr);
#else
    signal_error();
#endif
}

void
XMLMarshal::process(const char *name, std::string *s)
{
    current_node_->add_attr(std::string(name), *s);
}

void
XMLMarshal::process(const char* name, const InAddrPtr& a)
{
    const char *addr = intoa(*a.addr());
    current_node_->add_attr(std::string(name), std::string(addr));
}

} // namespace oasys
