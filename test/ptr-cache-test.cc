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
#include "util/PointerCache.h"

using namespace oasys;

struct Name {};

class PtrCacheTest : public PointerCache<Name, int> {
public:
    PtrCacheTest(int* i) : PointerCache<Name, int>(i) {
        set_ptr(i);
    }

    PtrSet& ptr_set() { return pointers_; }

protected:
    void register_ptr(int* ptr) {
        PointerCache<Name, int>::register_ptr(ptr);
        printf("register()");
        num_++;
    }

    void unregister_ptr(int* ptr) {
        PointerCache<Name, int>::unregister_ptr(ptr);
        printf("unregister()");
        num_--;
    }
    
    void resurrect() {
        printf("resurrect()\n");
    }

    bool at_limit(int* i) {
        printf("num_ = %u\n", num_);
        return num_ >= 4;
    }

    void evict() {
        printf("evict()\n");
    }

private:
    static size_t num_;
};

size_t PtrCacheTest::num_ = 0;

DECLARE_TEST(Test1) {
    int a, b, c;
    PtrCacheTest pa(&a), pb(&b), pc(&c);

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(PtrCacheTester) {
    ADD_TEST(Test1);
}

DECLARE_TEST_FILE(PtrCacheTester, "pointer cache test");
