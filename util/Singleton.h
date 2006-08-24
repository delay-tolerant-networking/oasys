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
 *  Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 *  Neither the name of the Intel Corporation nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
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
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include "../debug/DebugUtils.h"

namespace oasys {

/**
 * Singleton utility template class. Usage:
 *
 * @code
 * // in .h file:
 * class MyClass : public oasys::Singleton<MyClass> {
 * private:
 *     friend class oasys::Singleton<MyClass>;
 *     MyClass();
 * };
 *
 * // in .cc file:
 * MyClass* oasys::Singleton<MyClass>::instance_;
 * @endcode
 */
template<typename _Class, bool _auto_create = true>
class Singleton;

// Autocreation of the singleton
template<typename _Class>
class Singleton<_Class, true> {
public:
    static _Class* instance() {
        // XXX/demmer this has potential race conditions if multiple
        // threads try to access the singleton for the first time
        
        if(instance_ == 0) {
            instance_ = new _Class();
        }
        ASSERT(instance_);

        return instance_;
    }
    
    static _Class* create() {
        if (instance_) {
            PANIC("Singleton create() method called more than once");
        }
        
        instance_ = new _Class();
        return instance_;
    }

    static void set_instance(_Class* instance) {
        if (instance_) {
            PANIC("Singleton set_instance() called with existing object");
        }
        instance_ = instance;
    }
    
protected:
    static _Class* instance_;
};

// No autocreation of the instance
template<typename _Class>
class Singleton<_Class, false> {
public:
    static _Class* instance() 
    {
        // XXX/demmer this has potential race conditions if multiple
        // threads try to access the singleton for the first time
        ASSERT(instance_);
        return instance_;
    }
    
    static _Class* create() 
    {
        if (instance_) 
        {
            PANIC("Singleton create() method called more than once");
        }
        
        instance_ = new _Class();
        return instance_;
    }

    static void set_instance(_Class* instance) 
    {
        if (instance_) 
        {
            PANIC("Singleton set_instance() called with existing object");
        }
        instance_ = instance;
    }
    
protected:
    static _Class* instance_;
};

/**
 * Reference to a Singleton. Usage:
 *
 * @code
 * void myfunc() {
 *     oasys::SingletonRef<MySingletonFoo> foo;
 *     foo->bar();
 * }
 * @endcode
 */
template<typename _Class>
class SingletonRef {
public:
    _Class* operator->() {
	return Singleton<_Class>::instance();
    }
};

} // namespace oasys

#endif // __SINGLETON_H__
