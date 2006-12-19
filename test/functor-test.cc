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


#include "util/UnitTest.h"
#include <list>

#include "../util/Functors.h"

class A {
public:
    A(int a, int b) : a_(a), b_(b) {}
    
    int value() const { return a_ + b_; }
    
private:
    int a_;
    int b_;
};

void fcn(const std::list<A>& l, const A& a)
{ 
    (void)a;
    const int target = 6;

    std::list<A>::const_iterator i = 
        std::find_if(l.begin(), l.end(), oasys::eq_functor(target, &A::value));
}

DECLARE_TEST(Test1) {
    std::list<A> l;

    // 3
    l.push_back(A(1, 2));

    // 6
    l.push_back(A(4, 2));

    // 7
    l.push_back(A(2, 5));

    int target = 6;

    std::list<A>::const_iterator i = 
        std::find_if(l.begin(), l.end(), oasys::eq_functor(target, &A::value));
    CHECK(i->value() == 6);
    --i;
    CHECK(i == l.begin());

    target = 8;
    i = std::find_if(l.begin(), l.end(), oasys::lt_functor(target, &A::value));
    CHECK(i == l.end());

    target = 3;
    i = std::find_if(l.begin(), l.end(), oasys::neq_functor(target, &A::value));
    CHECK(i->value() == 6);

    fcn(l, A(2,3));

    return oasys::UNIT_TEST_PASSED;
}

DECLARE_TESTER(Test) {
    ADD_TEST(Test1);
}

DECLARE_TEST_FILE(Test, "functor test");
