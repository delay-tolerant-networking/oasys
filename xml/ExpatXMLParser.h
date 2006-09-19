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
#ifndef _OASYS_EXPAT_XML_PARSER_H_
#define _OASYS_EXPAT_XML_PARSER_H_

#include "config.h"

#ifdef LIBEXPAT_ENABLED

#include <expat.h>
#include "XMLParser.h"
#include "debug/Logger.h"

namespace oasys {

class XMLDocument;
class XMLObject;

class ExpatXMLParser : public XMLParser, public Logger {
public:
    /// Constructor
    ExpatXMLParser(const char* logpath);
    
    /// Destructor
    virtual ~ExpatXMLParser();

    /// Virtual from XMLParser
    bool parse(XMLDocument* doc, const std::string& data);

private:
    /// @{ Expat callbacks
    static void XMLCALL start_element(void* data,
                                      const char* element,
                                      const char** attr);
    
    static void XMLCALL end_element(void* data,
                                    const char* element);
    
    static void XMLCALL character_data(void* data,
                                       const XML_Char* s,
                                       int len);
    /// @}

    XMLDocument* doc_;	///< The XMLDocument being worked on
    XMLObject*   cur_;	///< The current XMLObject
};

} // namespace oasys

#endif /* LIBEXPAT_ENABLED */
#endif /* _OASYS_EXPAT_XML_PARSER_H_ */