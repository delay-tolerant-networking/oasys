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

#include "debug/DebugUtils.h"
#include "TclListSerialize.h"

namespace oasys {

TclListSerialize::TclListSerialize(Tcl_Interp* interp,
                                   Tcl_Obj*    list_obj,
                                   context_t   context,
                                   int         options)
    : SerializeAction(Serialize::MARSHAL, context, options),
      interp_(interp), list_obj_(list_obj)
{
}

TclListSerialize::~TclListSerialize()
{
}

void
TclListSerialize::process(const char* name, u_int32_t* i)
{
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewIntObj(*i));
}

void
TclListSerialize::process(const char* name, u_int16_t* i)
{
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewIntObj(*i));
}

void
TclListSerialize::process(const char* name, u_int8_t* i)
{
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewIntObj(*i));
}

void
TclListSerialize::process(const char* name, bool* b)
{
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewBooleanObj(*b));
}

void
TclListSerialize::process(const char* name, u_char* bp, size_t len)
{
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewByteArrayObj(bp, len));
}

void
TclListSerialize::process(const char* name, std::string* s)
{
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));
    Tcl_ListObjAppendElement(interp_, list_obj_,
                             Tcl_NewStringObj(s->data(), s->length()));
}

void
TclListSerialize::process(const char* name, u_char** bp,
                          size_t* lenp, int flags)
{
    if (flags & Serialize::NULL_TERMINATED) {
        Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));
        Tcl_ListObjAppendElement(interp_, list_obj_,
                                 Tcl_NewStringObj((char*)*bp, -1));
    } else {
        Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));
        Tcl_ListObjAppendElement(interp_, list_obj_,
                                 Tcl_NewStringObj((char*)*bp, *lenp));
    }
}

void
TclListSerialize::process(const char* name, SerializableObject* object)
{
    Tcl_Obj* old_list_obj = list_obj_;
    Tcl_Obj* new_list_obj = Tcl_NewListObj(0, NULL);

    list_obj_ = new_list_obj;
    object->serialize(this);
    list_obj_ = old_list_obj;
    
    Tcl_ListObjAppendElement(interp_, list_obj_, Tcl_NewStringObj(name, -1));

    int length = 0;
    int ok = Tcl_ListObjLength(interp_, new_list_obj, &length);
    ASSERT(ok == TCL_OK);
    
    if ((length != 2) || (options_ & KEEP_SINGLETON_SUBLISTS)) {
        Tcl_ListObjAppendElement(interp_, list_obj_, new_list_obj);
    } else {
        Tcl_Obj* obj;
        int ok = Tcl_ListObjIndex(interp_, new_list_obj, 1, &obj);
        ASSERT(ok == TCL_OK);
        
        Tcl_ListObjAppendElement(interp_, list_obj_, obj);
        Tcl_IncrRefCount(new_list_obj);
        Tcl_DecrRefCount(new_list_obj); // free it
    }
}


} // namespace oasys
