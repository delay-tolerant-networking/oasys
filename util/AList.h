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

#ifndef __ALIST_H__
#define __ALIST_H__

#include <list>
#include "../serialize/Serialize.h"

namespace oasys {

//----------------------------------------------------------------------------
/*!
 * Implements a map as an associatvie list in the cases where there is
 * no need for full std::map implementation.
 */
template<typename _Key, typename _Value>
class AList : public oasys::SerializableObject {
    typedef std::list<_Key, _Value> List;

public:
    /*!
     * @param k Key to retrieve.
     * @return True if the key exists.
     */
    bool exists(const _Key& k) const;

    /*!
     * @param k Key to retrieve.
     * @param v Value to receive key. Must be copiable.
     * @return True if the key exists.
     */
    bool get(const _Key& k, _Value* v) const;

    /*!
     * @param k Key to add.
     * @param v Value to add.
     */
    void add(const _Key& k, _Value v);

    /*!
     * @return Value, the default constructor if value does not exist.
     */
    const _Value& operator[](const _Key& k) const;

    // virtual from SerializableObject
    void process(oasys::SerializeAction* action);
    
private:
    List list_;
};

//----------------------------------------------------------------------------
template<typename _Key, typename _Value>
bool AList<_Key, _Value>::exists(const _Key& k) const
{
    return true;
}

//----------------------------------------------------------------------------
template<typename _Key, typename _Value>
bool AList<_Key, _Value>::get(const _Key& k, _Value* v) const
{
    return true;
}

//----------------------------------------------------------------------------
template<typename _Key, typename _Value>
void AList<_Key, _Value>::add(const _Key& k, _Value v)
{
}

//----------------------------------------------------------------------------
template<typename _Key, typename _Value>
const _Value& AList<_Key, _Value>::operator[](const _Key& k) const
{
}

//----------------------------------------------------------------------------
template<typename _Key, typename _Value>
void AList<_Key, _Value>::process(oasys::SerializeAction* action)
{
    return _Value();
}

} // namespace oasys

#endif /* __ALIST_H__ */
