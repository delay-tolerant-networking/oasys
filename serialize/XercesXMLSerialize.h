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

#ifndef _OASYS_XERCES_XML_SERIALIZE_H_
#define _OASYS_XERCES_XML_SERIALIZE_H_

#include <config.h>
#ifdef XERCES_C_ENABLED

#include "XMLSerialize.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMBuilder.hpp>
#include <xercesc/framework/XMLGrammarPool.hpp>

namespace oasys {

class Mutex;

/**
 * <p>XercesXMLUnmarshal implements the SerializeAction and
 * XMLUnmarshal interface using the Xerces C++ Parser.
 * The class is designed to work in conjunction with
 * oasys serializtion where XML element names map to DTN
 * classes and attributes map to class data members.</p>
 * <p><code>parse</code> takes an xml document string, optionally
 * performs validation, builds an internal DOM tree, and
 * returns the first element tag found.  The calling code
 * instantiates a class based upon the tag name and calls
 * action() to unserialize the object.  Calling <code>parse</code>
 * again returns the next element tag found and the process is
 * repeated.</p>
 */
class XercesXMLUnmarshal : public XMLUnmarshal,
                           public Logger {
public:
    XercesXMLUnmarshal(bool validation, const char *schema=0);
    virtual ~XercesXMLUnmarshal();

    /**
     * Parse, optionally validate, and build a DOM tree from
     * the provided xml document.  Return the first element tag.
     * @param xml_doc source XML document
     * @return the first top-level element tag or 0 on failure
     */
    virtual const char *parse(const char *xml_doc = 0);

    // Virtual functions inherited from SerializeAction
    virtual void process(const char *name, u_int32_t *i);
    virtual void process(const char *name, u_int16_t *i);
    virtual void process(const char *name, u_int8_t *i);
    virtual void process(const char *name, bool *b);
    virtual void process(const char *name, u_char *bp,
                         size_t len);
    virtual void process(const char* name, u_char** bp,
                         size_t* lenp, int flags);
    virtual void process(const char *name, std::string *s);

protected:
    /**
     * Used for iterating over DOM tree elements
     * @return char string of next top-level element in the DOM tree
     */
    virtual const char *next_elem();

    char *root_tag_str;
    xercesc::XMLGrammarPool *pool_;
    xercesc::DOMImplementation *impl_;
    xercesc::DOMBuilder *parser_;
    xercesc::DOMDocument *doc_;
    xercesc::DOMElement *root_elem_;
    xercesc::DOMTreeWalker *walker_;

    // avoid race conditions involving multiple simultaneous
    // constructions of XercesXMLUnmarshal
    static oasys::Mutex* lock_;
};

/**
 * ValidationError is a helper class to catch DOM parser errors.
 */
class ValidationError : public xercesc::DOMErrorHandler {
public:
    ValidationError()
        : set_(false), severity_(-1), message_(0)
    {
    }

    virtual ~ValidationError()
    {
        delete [] message_;
    }

    /**
     * Error handler registered with Xerces
     */
    virtual bool handleError(const xercesc::DOMError& domError);

    bool is_set()
    {
        return set_;
    }

    short get_severity()
    {
        return severity_;
    }

    const char * message()
    {
        return message_;
    }

private:
    bool set_;
    short severity_;
    char *message_;
};

} // namespace oasys

#endif // XERCES_C_ENABLED
#endif //_OASYS_XERCES_XML_SERIALIZE_H_
