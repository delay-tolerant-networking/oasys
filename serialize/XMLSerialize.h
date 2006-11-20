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

#ifndef _OASYS_XML_SERIALIZE_H_
#define _OASYS_XML_SERIALIZE_H_

#include <vector>
#include <sstream>
#include <sys/time.h>

#include "Serialize.h"
#include "../util/StringBuffer.h"
#include "../xml/XMLDocument.h"
#include "../xml/XMLObject.h"

namespace oasys {

/**
 * XMLMarshal implements common functionality for building up
 * an XML document.
 */
class XMLMarshal : public SerializeAction {
public:
    XMLMarshal(ExpandableBuffer *buf, const char *root_tag);

    // Virtual process functions inherited from SerializeAction
    virtual void end_action();
    virtual void process(const char *name, SerializableObject* object);
    virtual void process(const char *name, u_int64_t *i);
    virtual void process(const char *name, u_int32_t *i);
    virtual void process(const char *name, u_int16_t *i);
    virtual void process(const char *name, u_int8_t *i);
    virtual void process(const char *name, bool *b);
    virtual void process(const char *name, u_char *bp, u_int32_t len);
    virtual void process(const char *name, u_char **bp,
                         u_int32_t *lenp, int flags);
    virtual void process(const char *name, std::string *s);

    /// Accessor to the internal XMLDocument
    const XMLDocument& doc() const { return doc_; }
    
protected:
    StringBuffer buf_;  ///< completed document buffer
    XMLDocument doc_;
    XMLObject *current_node_;
};

/**
 * Interface designed to be implemented by third-party
 * XML parsers
 */
class XMLUnmarshal : public SerializeAction {
public:
    XMLUnmarshal()
        : SerializeAction(Serialize::UNMARSHAL,
          Serialize::CONTEXT_UNKNOWN) {}

    /**
     * Parse the provided string buffer.
     * @return next element tag found or 0 on failure
     */
    virtual const char * parse(const char *xml_doc) = 0;
};

} // namespace oasys

#endif // _OASYS_XML_SERIALIZE_H_
