/*
 *    Copyright 2006-2007 The MITRE Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 *    The US Government will not be charged any license fee and/or royalties
 *    related to this software. Neither name of The MITRE Corporation; nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
