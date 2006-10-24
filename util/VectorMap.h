/*
 *    Copyright 2006 Intel Corporation
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

#ifndef __VECTORMAP_H__
#define __VECTORMAP_H__

#include <vector>

namespace oasys {

template <typename _Type>
class vector_map {
public:
    typedef std::vector<_Type> EntVector;
    
    vector_map() {}
    
    template <typename _Predicate>
    bool exists(_Predicate eq) const {
        EntVector::const_iterator i = std::find_if(ents_.begin(), ents_.end(), eq);
        return i != ents_.end();
    }

    template <typename _Predicate>
    bool insert(_Predicate eq, const _Type& type) {
        EntVector::iterator i = std::find_if(ents_.begin(), ents_.end(), eq);
        if (i == ents_.end()) 
        {
            ents_.push_back(type);
            return false;
        }
        else
        {
            *i = type;
            return true;
        }
    }

    template <typename _Predicate>
    const Type& get(_Predicate eq) const {
        EntVector::const_iterator i = std::find_if(ents_.begin(), ents_.end(), eq);
        return *i;
    }

private:
    EntVector ents_;
}; 

} // namespace oasys

#if 1 

#include <cstdio>

class Equals(int i) 
{
    
}

int
main(int argc, char* argv[])
{
    oasys::vector_map<int> vm;

    
    if (vm.exists(
}

#endif

#endif /* __VECTORMAP_H__ */
