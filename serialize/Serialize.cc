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
#include "Serialize.h"
#include "debug/DebugUtils.h"

namespace oasys {

/**
 * Create a SerializeAction with the specified type code and context
 *
 * @param type serialization action type code
 * @param context serialization context
 * @param options serialization options
 */
SerializeAction::SerializeAction(
    action_t  action, 
    context_t context, 
    int       options
    ) : action_(action), 
        context_(context), 
        options_(options), 
        log_(0), 
        error_(false)
{
}

SerializeAction::~SerializeAction()
{
}

/**
 * Call the virtual serialize() callback which will, in turn, call the
 * various process() callbacks on ourself. If any of the process
 * functions fails due to insufficient buffer space, it will set
 * error_ to true.
 */
int
SerializeAction::action(SerializableObject* object)
{
    error_ = false;

    begin_action();
    object->serialize(this);
    end_action();
    
    if (error_ == true)
        return -1;
    
    return 0;
}


/**
 * By default, do nothing.
 */
void
SerializeAction::begin_action()
{
}

/**
 * By default, do nothing.
 */
void
SerializeAction::end_action()
{
}

Builder Builder::static_builder_;

} // namespace oasys
