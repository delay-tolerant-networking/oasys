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
#include <list>

#include <debug/Log.h>
#include <memory/Memory.h>

using namespace oasys;

class Foo_1 {};
std::list<Foo_1*> lFoos;

void
Alloc_1()
{
    lFoos.push_back(new Foo_1());
}

void
Alloc_2()
{
    lFoos.push_back(new Foo_1());
}

void Alloc_3()
{
    lFoos.push_back(new Foo_1());

    Alloc_1();
    Alloc_2();
}

class Big { char buf[100000]; };

void
delete_all_foo()
{
    while(lFoos.size() > 0)
    {
	Foo_1* obj = lFoos.front();
	delete obj;

	lFoos.pop_front();
    }
}

int
main(int argc, char* argv[])
{
    Log::init(LOG_DEBUG);

#if ! OASYS_DEBUG_MEMORY_ENABLED
    log_crit("/memory", "test must be run with memory debugging enabled");
#else
    
    DbgMemInfo::init(DbgMemInfo::USE_SIGNAL, "/tmp/dump");

    log_info("/memory", "offset of data=%u\n", 
	     offsetof(dbg_mem_t, block_));

    // Create 11 Foo_1 object in different places
    Alloc_3();
    Alloc_3();
    Alloc_3();
    Alloc_2();
    Alloc_1();

    DbgMemInfo::debug_dump();
    FILE* f = fopen("dump", "w");
    ASSERT(f != 0);

    // Delete all Foo_1 objects
    delete_all_foo();
    DbgMemInfo::debug_dump();

    std::list<int> l;
    l.push_back(2);
    l.push_back(3);
    l.push_back(4);
    l.push_back(5);
    l.push_back(6);
    l.push_back(7);
    l.push_back(8);
    l.push_back(9);
    l.push_back(1);
    DbgMemInfo::debug_dump(); 

    new Big();    

    while(1);

#endif
}
