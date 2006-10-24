/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "Serialize.h"
#include "debug/DebugUtils.h"

namespace oasys {

//----------------------------------------------------------------------
SerializeAction::SerializeAction(action_t  action, 
                                 context_t context, 
                                 int       options)
    : action_(action), 
      context_(context), 
      options_(options), 
      log_(0), 
      error_(false)
{
}

//----------------------------------------------------------------------
SerializeAction::~SerializeAction()
{
}

//----------------------------------------------------------------------
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


//----------------------------------------------------------------------
void
SerializeAction::begin_action()
{
}

//----------------------------------------------------------------------
void
SerializeAction::end_action()
{
}

//----------------------------------------------------------------------
void
SerializeAction::process(const char* name, unsigned long* i)
{
    STATIC_ASSERT(sizeof(*i) == sizeof(u_int32_t),
                  Sizeof_Unsigned_Long_Equals_UInt32_T);
    
    process(name, (u_int32_t*)i);
}

//----------------------------------------------------------------------
Builder Builder::static_builder_;

} // namespace oasys
