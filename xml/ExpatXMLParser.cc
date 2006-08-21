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

#include "config.h"

#ifdef LIBEXPAT_ENABLED

#include "ExpatXMLParser.h"
#include "XMLDocument.h"
#include "XMLObject.h"

namespace oasys {

//----------------------------------------------------------------------
ExpatXMLParser::ExpatXMLParser(const char* logpath)
    : Logger("ExpatXMLParser", logpath)
{
}
    
//----------------------------------------------------------------------
ExpatXMLParser::~ExpatXMLParser()
{
}

//----------------------------------------------------------------------
bool
ExpatXMLParser::parse(XMLDocument* doc, const std::string& data)
{
    XML_Parser p = XML_ParserCreate(NULL);

    // set up the expat handlers
    XML_SetUserData(p, this);
    XML_SetElementHandler(p, start_element, end_element);
    XML_SetCharacterDataHandler(p, character_data);

    // cache the document and null out the object
    doc_ = doc;
    cur_ = NULL;

    if (XML_Parse(p, data.c_str(), data.length(), true) != XML_STATUS_OK) {
        log_err("parse error at line %d:\n%s",
                XML_GetCurrentLineNumber(p),
                XML_ErrorString(XML_GetErrorCode(p)));
        return false;
    }

    return true;
}

//----------------------------------------------------------------------
void XMLCALL
ExpatXMLParser::start_element(void* data,
                              const char* element,
                              const char** attr)
{
    ExpatXMLParser* this2 = (ExpatXMLParser*)data;

    XMLObject* new_object = new XMLObject(element);
    if (this2->cur_ == NULL) {
        this2->doc_->set_root(new_object);
    } else {
        this2->cur_->add_element(new_object);
    }

    this2->cur_ = new_object;
    while (attr[0] != NULL) {
        ASSERT(attr[1] != NULL);
        this2->cur_->add_attr(attr[0], attr[1]);
        attr += 2;
    }
}

//----------------------------------------------------------------------
void XMLCALL
ExpatXMLParser::end_element(void* data,
                            const char* element)
{
    ExpatXMLParser* this2 = (ExpatXMLParser*)data;
    ASSERT(this2->cur_->tag() == element);
    this2->cur_ = this2->cur_->parent();
}

//----------------------------------------------------------------------
void XMLCALL
ExpatXMLParser::character_data(void* data,
                               const XML_Char* s,
                               int len)
{
    ExpatXMLParser* this2 = (ExpatXMLParser*)data;
    ASSERT(this2->cur_ != NULL);
    this2->cur_->add_text(s, len);
}

} // namespace oasys

#endif /* LIBEXPAT_ENABLED */
