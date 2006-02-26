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
#include "util/PointerHandle.h"
#include "util/LRUList.h"

using namespace oasys;

struct Name {};

class PtrCacheTest : public PointerHandle<int> {
public:
    PtrCacheTest(int* i)
        : orig_ptr_(i)
    {
        restore();
    }

    ~PtrCacheTest() { 
        invalidate();
    }
    
    int* orig_ptr() { return ptr_; }

protected:
    typedef LRUList<PtrCacheTest*> CacheList;
    static CacheList lru_;

    int* orig_ptr_;

    void invalidate() {
        printf("invalidate %p, %p\n", this, ptr_);
        
        if (ptr_ == 0)
            return;

        CacheList::iterator i = std::find(lru_.begin(), lru_.end(), this);
        ASSERT(i != lru_.end());

        lru_.erase(i);
        ptr_ = 0;
    }

    void restore() {
        printf("restore %p\n", this);

        ASSERT(std::find(lru_.begin(), lru_.end(), this) == lru_.end());

        ptr_ = orig_ptr_;
        lru_.push_back(this);

        while (lru_.size() > 3) {
            PtrCacheTest* evict = lru_.front();
            evict->invalidate();
        }
    }

    void update() {
        printf("update %p\n", this);

        CacheList::iterator i = std::find(lru_.begin(), lru_.end(), this);
        ASSERT(i != lru_.end());
        lru_.move_to_back(i);
    }
};

PtrCacheTest::CacheList PtrCacheTest::lru_;

DECLARE_TEST(Test1) {
    int a, b, c, d, e, f, g;
    PtrCacheTest pa(&a), pb(&b), pc(&c), pd(&d), pe(&e);
    PtrCacheTest pf(&f), pg(&g);
    
    CHECK(pa.orig_ptr() == 0);
    CHECK(pb.orig_ptr() == 0);
    CHECK(pc.orig_ptr() == 0);
    CHECK(pd.orig_ptr() == 0);
    CHECK(pe.orig_ptr() == &e);
    CHECK(pf.orig_ptr() == &f);
    CHECK(pg.orig_ptr() == &g);

    CHECK(pa.ptr() == &a);
    CHECK(pa.ptr() == &a);
    CHECK(pc.ptr() == &c);
    CHECK(pd.ptr() == &d);
    CHECK(pb.ptr() == &b);
    CHECK(pb.ptr() == &b);
    CHECK(pd.ptr() == &d);
    CHECK(pe.ptr() == &e);
    CHECK(pf.ptr() == &f);
    CHECK(pg.ptr() == &g);
    CHECK(pc.ptr() == &c);
    CHECK(pe.ptr() == &e);
    

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(PtrCacheTester) {
    ADD_TEST(Test1);
}

DECLARE_TEST_FILE(PtrCacheTester, "pointer cache test");
