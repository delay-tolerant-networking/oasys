#ifndef __SERIALIZEHELPERS_H__
#define __SERIALIZEHELPERS_H__

#include <vector>

#include "Serialize.h"

namespace oasys {

//----------------------------------------------------------------------------
template<typename _type> void
serialize_basic_vector(oasys::SerializeAction* action,
                       std::string             name,
                       std::vector<_type>*     v)
{
    name += ".size";

    size_t size;
    size = v->size();
    action->process(name.c_str(), &size);
    
    if (action->error())
        return;

    v->resize(size);
    
    for (size_t i=0; i<size; ++i)
    {
        action->process(name.c_str(), &((*v)[i]));
        if (action->error())
            return;
    }
}

} // namespace oasys

#endif /* __SERIALIZEHELPERS_H__ */
