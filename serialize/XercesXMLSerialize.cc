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

#include <config.h>
#ifdef XERCES_C_ENABLED

#include <sys/stat.h>
#include <errno.h>
#include <string>
#include "XercesXMLSerialize.h"
#include "../thread/Mutex.h"
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/Base64.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include <xercesc/validators/schema/SchemaGrammar.hpp>
#include <xercesc/internal/XMLGrammarPoolImpl.hpp>

namespace oasys {

XercesXMLUnmarshal::XercesXMLUnmarshal(bool validation,
                                       const char *schema)
    : Logger("XercesXMLUnmarshal", "/XercesXMLUnmarshal"), root_elem_(0)
{    
    XercesXMLUnmarshal::lock_->lock("Constructing XercesXMLUnmarshal");

    if (validation) {
        // check that we have a good schema
        bool error = false;
        struct stat buf;

        if (stat(schema, &buf)) {
            log_warn("failed to open schema_file: %s", std::strerror(errno));
            error = true;
        } else if (! S_ISREG(buf.st_mode)) {
            log_warn("%s: not a regular file", schema);
            error = true;
        }

        if (error) {
            log_warn("disabling server message validation");
            validation = false;
        }
    }

    // initialize the xerces-c library
    xercesc::XMLPlatformUtils::Initialize();

    static const XMLCh LS[] = { xercesc::chLatin_L,
        xercesc::chLatin_S, xercesc::chNull };

     // Get a DOM implementation capable of "Load and Save"
    impl_ = xercesc::DOMImplementationRegistry::getDOMImplementation(LS);

    if (validation) {
        // create a grammar pool
        pool_ = new xercesc::XMLGrammarPoolImpl(
            xercesc::XMLPlatformUtils::fgMemoryManager);

        // create the DOM parser
        parser_ = (reinterpret_cast<xercesc::DOMImplementationLS *>(impl_))->
            createDOMBuilder(xercesc::DOMImplementationLS::MODE_SYNCHRONOUS, 0,
                xercesc::XMLPlatformUtils::fgMemoryManager,
                XercesXMLUnmarshal::pool_);

        // use the SGXML scanner
        parser_->setProperty(xercesc::XMLUni::fgXercesScannerName,
            (void *) xercesc::XMLUni::fgSGXMLScanner);

        // turn on DOM validation
        if (parser_->canSetFeature(xercesc::XMLUni::fgDOMValidation, true))
            parser_->setFeature(xercesc::XMLUni::fgDOMValidation, true);

        // used cached grammar during message validation
        if (parser_->canSetFeature(xercesc::XMLUni::fgXercesUseCachedGrammarInParse, true))
            parser_->setFeature(xercesc::XMLUni::fgXercesUseCachedGrammarInParse, true);

        // check the schema when loading
        if (parser_->canSetFeature(xercesc::XMLUni::fgXercesSchemaFullChecking, true))
            parser_->setFeature(xercesc::XMLUni::fgXercesSchemaFullChecking, true);

        // look to the cache when validating messages with no namespace
        XMLCh empty_str = 0x00;
        parser_->setProperty(xercesc::XMLUni::fgXercesSchemaExternalNoNameSpaceSchemaLocation,
            (void *) &empty_str);

        // load the grammar pool
        XMLCh *w_schema = xercesc::XMLString::transcode(schema);
        parser_->loadGrammar(w_schema, xercesc::Grammar::SchemaGrammarType, true);
        xercesc::XMLString::release(&w_schema);
        pool_->lockPool();
    } else {
        parser_ = (reinterpret_cast<xercesc::DOMImplementationLS *>(impl_))->
            createDOMBuilder(xercesc::DOMImplementationLS::MODE_SYNCHRONOUS, 0);

        // use the WFXML scanner
        parser_->setProperty(xercesc::XMLUni::fgXercesScannerName,
            (void *) xercesc::XMLUni::fgWFXMLScanner);
    }

    XercesXMLUnmarshal::lock_->unlock();
}

XercesXMLUnmarshal::~XercesXMLUnmarshal()
{
    XercesXMLUnmarshal::lock_->lock("Deconstructing XercesXMLUnmarshal");

    xercesc::XMLString::release(&root_tag_str);
    parser_->release();
    xercesc::XMLPlatformUtils::Terminate();

    XercesXMLUnmarshal::lock_->unlock();
}

// Parse, optionally validate, and build a DOM tree
// from the supplied xml document
const char *
XercesXMLUnmarshal::parse(const char *xml_doc)
{
    if (root_elem_) return next_elem();

    if (! xml_doc) {
        log_warn("parser received empty xml document");
        signal_error();
        return 0;
    }

    // load an error handler
    ValidationError error_handler;
    parser_->setErrorHandler(&error_handler);

    // parse the given xml document
    xercesc::MemBufInputSource message(
        reinterpret_cast<const XMLByte * const>(xml_doc),
        strlen(xml_doc), "message");
    xercesc::Wrapper4InputSource xml_source(&message, false);
    parser_->resetDocumentPool();
    doc_ = parser_->parse(xml_source);

    // was the message valid?
    if (error_handler.is_set()) {
        log_warn("message dropped\n\t%s \n\toffending message was: %s",
            error_handler.message(), xml_doc);
        signal_error();
        return 0;
    }

    // create a walker
    root_elem_ = doc_->getDocumentElement();
    walker_ = doc_->createTreeWalker(root_elem_,
        xercesc::DOMNodeFilter::SHOW_ELEMENT, 0, true);
    const XMLCh *root_tag = root_elem_->getTagName();
    root_tag_str = xercesc::XMLString::transcode(root_tag);
    return root_tag_str;
}

// Return the next element name
const char *
XercesXMLUnmarshal::next_elem()
{
    root_elem_ = reinterpret_cast<xercesc::DOMElement *>(
        walker_->nextNode());

    if (root_elem_) {
        const XMLCh *root_tag = root_elem_->getTagName();
        xercesc::XMLString::release(&root_tag_str);
        char *root_tag_str = xercesc::XMLString::transcode(root_tag);
        return root_tag_str;
    } else {
        walker_->release();
        return 0;
    }
}

void
XercesXMLUnmarshal::process(const char *name, u_int64_t *i)
{
    XMLCh *tag_name = xercesc::XMLString::transcode(name);
    char *value = xercesc::XMLString::transcode(
        root_elem_->getAttribute(tag_name));

    *i = atoll(value);

    xercesc::XMLString::release(&tag_name);
    xercesc::XMLString::release(&value);
}

void
XercesXMLUnmarshal::process(const char *name, u_int32_t *i)
{
    XMLCh *tag_name = xercesc::XMLString::transcode(name);
    char *value = xercesc::XMLString::transcode(
        root_elem_->getAttribute(tag_name));

    *i = atoi(value);

    xercesc::XMLString::release(&tag_name);
    xercesc::XMLString::release(&value);
}

void
XercesXMLUnmarshal::process(const char *name, u_int16_t *i)
{
    XMLCh *tag_name = xercesc::XMLString::transcode(name);
    char *value = xercesc::XMLString::transcode(
        root_elem_->getAttribute(tag_name));

    *i = atoi(value);

    xercesc::XMLString::release(&tag_name);
    xercesc::XMLString::release(&value);
}

void
XercesXMLUnmarshal::process(const char *name, u_int8_t *i)
{
    XMLCh *tag_name = xercesc::XMLString::transcode(name);
    char *value = xercesc::XMLString::transcode(
        root_elem_->getAttribute(tag_name));

    *i = atoi(value);

    xercesc::XMLString::release(&tag_name);
    xercesc::XMLString::release(&value);
}

void
XercesXMLUnmarshal::process(const char *name, bool *b)
{
    XMLCh *tag_name = xercesc::XMLString::transcode(name);
    char *value = xercesc::XMLString::transcode(
        root_elem_->getAttribute(tag_name));

    *b = (strcmp(value, "true") == 0) ? true : false;

    xercesc::XMLString::release(&tag_name);
    xercesc::XMLString::release(&value);
}

void
XercesXMLUnmarshal::process(const char *name, u_char *bp, u_int32_t len)
{
    if (len < 2) return;

    XMLCh *tag_name = xercesc::XMLString::transcode(name);
    std::string value(
        xercesc::XMLString::transcode(
        root_elem_->getAttribute(tag_name)));

    char *sbp = reinterpret_cast<char *>(bp);
    memset(sbp, '\0', len);
    value.copy(sbp, len - 1);

    xercesc::XMLString::release(&tag_name);
}

void
XercesXMLUnmarshal::process(const char* name, u_char** bp,
                            u_int32_t* lenp, int flags)
{
    if (flags & Serialize::ALLOC_MEM) {
        u_int32_t len = *lenp;
        if (flags & Serialize::NULL_TERMINATED)
            len += 1;
        *bp = static_cast<u_char*>(malloc(len));
        if (*bp == 0) {
            signal_error();
            return;
        }
        *lenp = len;
    }

    XMLCh *tag_name = xercesc::XMLString::transcode(name);
    std::string value(
        xercesc::XMLString::transcode(
        root_elem_->getAttribute(tag_name)));

    char *sbp = reinterpret_cast<char *>(*bp);
    if (flags & Serialize::NULL_TERMINATED) {
        memset(sbp, '\0', *lenp);
        value.copy(sbp, *lenp - 1);
    } else {
        value.copy(sbp, *lenp);
    }

    xercesc::XMLString::release(&tag_name);
}

void
XercesXMLUnmarshal::process(const char *name, std::string *s)
{
    XMLCh *tag_name = xercesc::XMLString::transcode(name);
    char *value = xercesc::XMLString::transcode(
        root_elem_->getAttribute(tag_name));

    s->assign(value);

    xercesc::XMLString::release(&tag_name);
    xercesc::XMLString::release(&value);
}

bool
ValidationError::handleError(const xercesc::DOMError &domError)
{
    severity_ = domError.getSeverity();

    delete [] message_;
    message_ = xercesc::XMLString::transcode(domError.getMessage());

    return set_ = true;
}

oasys::Mutex* XercesXMLUnmarshal::lock_ = new oasys::Mutex("XercesXMLUnmarshal");

} // namespace oasys
#endif // XERCES_C_ENABLED
