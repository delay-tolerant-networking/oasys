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
#ifndef _XMLOBJECT_H_
#define _XMLOBJECT_H_

#include <string>
#include <vector>
#include "../debug/DebugUtils.h"

namespace oasys {

class StringBuffer;

/**
 * A simple object-based representation of an XML entity.
 *
 * Note that the class assumes memory management responsibility for
 * all child objects, i.e. when the destructor is called, all child
 * objects are recursively destroyed as well (using delete).
 */
class XMLObject {
public:
    /**
     * Type for the attribute list is a vector of strings of the form
     * "attr1" "val1" "attr2" "val2" ...
     */
    typedef std::vector<std::string> Attrs;

    /**
     * Type for the attribute list is a vector of strings of the form
     * "target1" "data1" "target2" "data2" ...
     */
    typedef std::vector<std::string> ProcInsts;

    /**
     * Type for the element list is a vector of XMLObject* pointers.
     */
    typedef std::vector<XMLObject*> Elements;
    
    /**
     * The constructor requires the tag name.
     */
    XMLObject(const std::string& tag);

    /**
     * The destructor recursively deletes all subelements.
     */
    ~XMLObject();

    /// @{ Accessors
    const std::string& tag()      const { return tag_; }
    const Attrs& attrs()          const { return attrs_; }
    const ProcInsts& proc_insts() const { return proc_insts_; }
    const Elements& elements()    const { return elements_; }
    const std::string& text()     const { return text_; }
    ///
    
    /**
     * Append an attribute/value pair.
     */
    void add_attr(const std::string& attr, const std::string& val);
    
    /**
     * Append a processing instruction / value pair.
     */
    void add_proc_inst(const std::string& target,
                       const std::string& data);
    
    /**
     * Append a child element and assume memory management
     * responsibility for it.
     */
    void add_element(XMLObject* child);

    /**
     * Append some text data.
     */
    void add_text(const char* text, size_t len = 0);

    /**
     * Recursively generate formatted XML text and put it into the
     * given buffer.
     *
     * @param indent Indentation control for subelements. If -1, put
     *               all subelements and text on the same line. If
     *               >= 0 then put each tag on a new line indented
     *               with the given number of spaces.
     *
     * @param cur_indent The current cumulative indentation
     */
    void to_string(StringBuffer* buf, int indent, int cur_indent = 0) const;
    
protected:
    std::string tag_;
    Attrs       attrs_;
    ProcInsts   proc_insts_;
    Elements    elements_;
    std::string text_;

    /**
     * We don't support assignment of the class.
     */
    NO_ASSIGN_COPY(XMLObject);
};

} // namespace oasys

#endif /* _XMLOBJECT_H_ */
