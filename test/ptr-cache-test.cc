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
#include "util/LRUList.h"

using namespace oasys;

struct Name {};

class PtrCacheTest : public PointerCache<Name, int> {
public:
    PtrCacheTest(int* i) : 
        PointerCache<Name, int>(),
        orig_ptr_(i)
    {
        set_ptr(i);
    }
    ~PtrCacheTest() { 
        set_ptr(0);
    }

protected:
    void register_ptr(int* ptr) {
        printf("register %p\n", ptr);
        
        ASSERT(all_ptrs_.find(ptr) == all_ptrs_.end());
        all_ptrs_[ptr] = this;

        lru_.push_back(ptr);
    }

    void unregister_ptr(int* ptr) {
        printf("unregister %p\n", ptr);
        lru_.erase(std::find(lru_.begin(), lru_.end(), ptr));

        PtrMap::iterator i = all_ptrs_.find(ptr);
        i->second->ptr_ = 0;
        all_ptrs_.erase(ptr);
    }
    
    void restore_and_update_ptr() {
        printf("restore and update %p\n", orig_ptr_);
        
        if (ptr_ == 0) {
            set_ptr(orig_ptr_);
        } else {
            lru_.move_to_back(std::find(lru_.begin(), lru_.end(), ptr_));
        }
    }

    bool at_limit(int* i) {
        printf("size = %u\n", lru_.size());
        return lru_.size() >= 4;
    }

    void evict() {
        printf("evict()\n");
        unregister_ptr(lru_.front());
    }

public:
    typedef std::map<int*, PtrCacheTest*> PtrMap;

    static PtrMap        all_ptrs_;
    static LRUList<int*> lru_;

    int* orig_ptr_;

};

LRUList<int*>        PtrCacheTest::lru_;
PtrCacheTest::PtrMap PtrCacheTest::all_ptrs_;

DECLARE_TEST(Test1) {
    int a, b, c, d, e;
    PtrCacheTest pa(&a), pb(&b), pc(&c), pd(&d), pe(&e);

    

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(PtrCacheTester) {
    ADD_TEST(Test1);
}

DECLARE_TEST_FILE(PtrCacheTester, "pointer cache test");
