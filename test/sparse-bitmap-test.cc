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

#include "util/SparseBitmap.h"
#include "util/UnitTest.h"

using namespace oasys;
char buf[1024];

DECLARE_TEST(Extend) {
    SparseBitmap<int> b;
    CHECK_EQUAL(b.num_entries(), 0);
    CHECK_EQUAL(b.num_contiguous(), 0);
    CHECK_EQUAL(b.num_set(), 0);
    CHECK(!b.is_set(-1));
    CHECK(!b.is_set(0));
    CHECK(!b.is_set(1));
    CHECK(!b.is_set(-1));
    CHECK(!b.is_set(0));
    CHECK(!b.is_set(1));
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ ]");

    DO(b.set(0));
    CHECK(!b.is_set(-1));
    CHECK(b.is_set(0));
    CHECK(!b.is_set(1));
    CHECK_EQUAL(b.num_contiguous(), 1);
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_set(), 1);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ 0 ]");

    DO(b.set(0, 3));
    CHECK(!b.is_set(-1));
    CHECK(b.is_set(0, 3));
    CHECK(b.is_set(0));
    CHECK(b.is_set(1));
    CHECK(b.is_set(2));
    CHECK(!b.is_set(3));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 3);
    CHECK_EQUAL(b.num_set(), 3);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ 0..2 ]");

    DO(b.set(-10, 21));
    CHECK(b.is_set(-10, 21));
    CHECK(!b.is_set(-11));
    CHECK(b.is_set(-10));
    CHECK(b.is_set(10));
    CHECK(!b.is_set(11));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 21);
    CHECK_EQUAL(b.num_set(), 21);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ -10..10 ]");

    DO(b.set(0));
    CHECK(b.is_set(-10, 21));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 21);
    CHECK_EQUAL(b.num_set(), 21);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ -10..10 ]");

    DO(b.set(-10, 21));
    CHECK(b.is_set(-10, 21));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 21);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ -10..10 ]");
    CHECK_EQUAL(b.num_set(), 21);

    DO(b.clear());
    DO(b.set(1));
    DO(b.set(2));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 2);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ 1..2 ]");
    
    DO(b.set(0));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 3);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ 0..2 ]");
    
    DO(b.set(3, 3));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 6);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ 0..5 ]");
    
    DO(b.set(-3, 3));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 9);
    CHECK(b.is_set(-3, 9));
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ -3..5 ]");
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Sparse) {
    SparseBitmap<int> b;

    DO(b.set(0));
    DO(b.set(2));
    CHECK(b[0]);
    CHECK(!b[1]);
    CHECK(b[2]);
    CHECK_EQUAL(b.num_entries(), 2);
    CHECK_EQUAL(b.num_contiguous(), 1);
    CHECK_EQUAL(b.num_set(), 2);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ 0 2 ]");

    DO(b.set(-2));
    DO(b.set(4));
    CHECK(b[-2]);
    CHECK(!b[-1]);
    CHECK(b[0]);
    CHECK(!b[1]);
    CHECK(b[2]);
    CHECK(!b[3]);
    CHECK(b[4]);
    CHECK_EQUAL(b.num_entries(), 4);
    CHECK_EQUAL(b.num_contiguous(), 1);
    CHECK_EQUAL(b.num_set(), 4);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ -2 0 2 4 ]");

    DO(b.set(-10, 4));
    CHECK_EQUAL(b.num_entries(), 5);
    CHECK_EQUAL(b.num_contiguous(), 4);
    CHECK_EQUAL(b.num_set(), 8);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ -10..-7 -2 0 2 4 ]");
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Clear) {
    SparseBitmap<int> b;

    // erase whole entry
    DO(b.set(0));
    DO(b.clear(0));
    CHECK(!b[0]);
    CHECK_EQUAL(b.num_entries(), 0);
    CHECK_EQUAL(b.num_set(), 0);

    DO(b.set(0, 10));
    DO(b.clear(-1000, 2000));
    CHECK(!b[0]);
    CHECK_EQUAL(b.num_entries(), 0);
    CHECK_EQUAL(b.num_set(), 0);
    DO(b.clear());

    // shrink entry on left
    DO(b.set(0, 10));
    DO(b.clear(0, 5));
    CHECK(!b[0]);
    CHECK(!b[4]);
    CHECK(b[5]);
    CHECK(b[9]);
    CHECK(!b[10]);
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_set(), 5);
    CHECK_EQUAL(b.num_contiguous(), 5);
    DO(b.clear());

    // shrink entry on right
    DO(b.set(0, 10));
    DO(b.clear(5, 5));
    CHECK(b[0]);
    CHECK(b[4]);
    CHECK(!b[5]);
    CHECK(!b[9]);
    CHECK(!b[10]);
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 5);
    CHECK_EQUAL(b.num_set(), 5);
    DO(b.clear());

    // shrink two
    DO(b.set(10, 10));
    DO(b.set(30, 10));
    CHECK_EQUAL(b.num_entries(), 2);
    CHECK_EQUAL(b.num_set(), 20);
    
    DO(b.clear(15, 20));
    CHECK(b.is_set(10));
    CHECK(b.is_set(14));
    CHECK(!b.is_set(15));
    CHECK(!b.is_set(34));
    CHECK(b.is_set(35));
    CHECK(b.is_set(39));
    CHECK(!b.is_set(40));
    CHECK_EQUAL(b.num_entries(), 2);
    CHECK_EQUAL(b.num_set(), 10);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Compact) {
    SparseBitmap<int> b;

    DO(b.set(1));
    DO(b.set(3));
    DO(b.set(5));
    DO(b.set(7));
    DO(b.set(9));

    CHECK_EQUAL(b.num_contiguous(), 1);
    CHECK_EQUAL(b.num_entries(), 5);
    CHECK_EQUAL(b.num_set(), 5);

    DO(b.set(1, 5));
    CHECK_EQUAL(b.num_contiguous(), 5);
    CHECK_EQUAL(b.num_entries(), 3);
    CHECK_EQUAL(b.num_set(), 7);
    
    DO(b.set(1, 9));
    CHECK_EQUAL(b.num_contiguous(), 9);
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_set(), 9);

    DO(b.clear());
    DO(b.set(0));
    DO(b.set(10));
    DO(b.set(0, 10));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 11);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ 0..10 ]");
    
    DO(b.clear());
    DO(b.set(10));
    DO(b.set(5, 5));
    CHECK_EQUAL(b.num_entries(), 1);
    CHECK_EQUAL(b.num_contiguous(), 6);
    DO(b.format(buf, sizeof(buf)));
    CHECK_EQUALSTR(buf, "[ 5..10 ]");
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Iterator) {
    SparseBitmap<int> b;

    SparseBitmap<int>::iterator i, j, k;

    DO(i = b.begin());
    CHECK(i == b.end());
    CHECK(b.begin() == b.end());

    DO(b.set(1));
    DO(i = b.begin());
    CHECK_EQUAL(*i, 1);
    CHECK(i == b.begin());
    CHECK(i != b.end());
    CHECK(b.first() == 1);
    CHECK(b.last() == 1);
    DO(++i);
    CHECK(i == b.end());
    DO(--i);
    CHECK(i != b.end());
    CHECK(i == b.begin());
    CHECK_EQUAL(*i, 1);

    DO(b.set(2));
    DO(b.set(5, 2));
    DO(i = b.begin());
    CHECK_EQUAL(*i++, 1);
    CHECK_EQUAL(*i++, 2);
    CHECK_EQUAL(*i++, 5);
    CHECK_EQUAL(*i++, 6);
    CHECK(i == b.end());
    CHECK_EQUAL(*--i, 6);
    CHECK_EQUAL(*--i, 5);
    CHECK_EQUAL(*--i, 2);
    CHECK_EQUAL(*--i, 1);
    CHECK(i == b.begin());
    CHECK(b.first() == 1);
    CHECK(b.last() == 6);

    CHECK_EQUAL(*i++, 1);
    CHECK_EQUAL(*i, 2);
    CHECK_EQUAL(*++i, 5);
    CHECK_EQUAL(*i, 5);

    DO(i = b.begin());
    DO(i.skip_contiguous());
    CHECK_EQUAL(*i, 2);
    DO(i.skip_contiguous());
    CHECK_EQUAL(*i, 2);
    DO(i++);
    CHECK_EQUAL(*i, 5);
    DO(i.skip_contiguous());
    CHECK_EQUAL(*i, 6);
    DO(i.skip_contiguous());
    CHECK_EQUAL(*i, 6);

    DO(i = b.end() - 1);
    CHECK_EQUAL(*i, 6);

    DO(i = b.begin());
    CHECK_EQUAL(*i, 1);
    DO(i += 1);
    CHECK_EQUAL(*i, 2);
    DO(i += 2);
    CHECK_EQUAL(*i, 6);

    DO(i -= 3);
    CHECK_EQUAL(*i, 1);
    
    DO(i += 2);
    CHECK_EQUAL(*i, 5);
    DO(i -= 2);
    CHECK_EQUAL(*i, 1);

    DO(i += 2);
    DO(i -= 1);
    CHECK_EQUAL(*i, 2);

    DO(b.set(10, 10000000));
    DO(i = b.begin());
    CHECK_EQUAL(*i, 1);
    DO(i += 4);
    CHECK_EQUAL(*i, 10);
    DO(i += (10000000 - 1));
    CHECK_EQUAL(*i, 10000009);
    DO(i++);
    CHECK(i == b.end());
    DO(i--);
    CHECK_EQUAL(*i, 10000009);

    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(Extend);
    ADD_TEST(Sparse);
    ADD_TEST(Clear);
    ADD_TEST(Compact);
    ADD_TEST(Iterator);
}

DECLARE_TEST_FILE(Test, "sparse bitmap test");
