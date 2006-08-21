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
#ifndef _OASYS_XML_DOCUMENT_H_
#define _OASYS_XML_DOCUMENT_H_

#include "../debug/DebugUtils.h"
#include "../util/StringBuffer.h"

namespace oasys {

class XMLObject;

/**
 * An object encapsulation of an XML document, consisting of some
 * amount of unparsed header information (i.e. processing
 * instructions, ENTITY references, etc), then a root tag XMLObject.
 */
class XMLDocument {
public:
    /**
     * Default constructor.
     */
    XMLDocument();

    /**
     * Destructor
     */
    ~XMLDocument();

    /// @{ Accessors
    const std::string header() const { return header_; }
    const XMLObject*  root()   const { return root_; }
    /// @}

    /**
     * Set the root tag. Assumes ownership of the object.
     */
    void set_root(XMLObject* root);

    /**
     * Append some header data
     */
    void add_header(const char* text, size_t len = 0);

    /**
     * Generate formatted XML text and put it into the given buffer.
     *
     * @param indent     The number of spaces to indent for subelements.
     */
    void to_string(StringBuffer* buf, int indent) const;

protected:
    std::string header_;
    XMLObject*  root_;

    NO_ASSIGN(XMLDocument);

};

} // namespace oasys

#endif /* _OASYS_XML_DOCUMENT_H_ */
