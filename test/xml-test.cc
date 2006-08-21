/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
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
#include "util/UnitTest.h"
#include "util/StringBuffer.h"
#include "xml/XMLObject.h"

using namespace oasys;
using namespace std;

std::string test_to_string(const XMLObject* obj, int indent = -1)
{
    StringBuffer buf;
    obj->to_string(&buf, indent);
    return std::string(buf.c_str(), buf.length());
}

DECLARE_TEST(To_StringTest) {
    {
        XMLObject obj("test");
        CHECK_EQUALSTR(test_to_string(&obj).c_str(), "<test/>");
    }

    {
        XMLObject obj("test");
        obj.add_attr("a1", "v1");
        CHECK_EQUALSTR(test_to_string(&obj).c_str(), "<test a1=\"v1\"/>");
    }
    
    {
        XMLObject obj("test");
        obj.add_attr("a1", "v1");
        obj.add_attr("a2", "v2");
        CHECK_EQUALSTR(test_to_string(&obj).c_str(), "<test a1=\"v1\" a2=\"v2\"/>");
    }
    
    {
        XMLObject obj("test");
        obj.add_element(new XMLObject("tag"));
        CHECK_EQUALSTR(test_to_string(&obj).c_str(), "<test><tag/></test>");
    }

    {
        XMLObject obj("test");
        obj.add_element(new XMLObject("tag"));
        obj.add_element(new XMLObject("tag2"));
        CHECK_EQUALSTR(test_to_string(&obj).c_str(), "<test><tag/><tag2/></test>");
    }
    
    {
        XMLObject obj("test");
        obj.add_text("Some text here");
        CHECK_EQUALSTR(test_to_string(&obj).c_str(), "<test>Some text here</test>");
    }

    {
        XMLObject obj("test");
        obj.add_proc_inst("xyz", "abc=\"foobar\"");
        CHECK_EQUALSTR(test_to_string(&obj).c_str(), "<test><?xyz abc=\"foobar\"?></test>");
    }

    {
        XMLObject obj("test");
        obj.add_attr("a1", "v1");
        obj.add_element(new XMLObject("tag"));
        obj.add_text("Some text here");
        CHECK_EQUALSTR(test_to_string(&obj).c_str(), "<test a1=\"v1\"><tag/>Some text here</test>");
    }
    
    return UNIT_TEST_PASSED;
}


DECLARE_TESTER(Test) {    
    ADD_TEST(To_StringTest);
}

DECLARE_TEST_FILE(Test, "string tokenize test");
