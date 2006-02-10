#ifndef __SERIALIZEHELPERS_H__
#define __SERIALIZEHELPERS_H__

#include <vector>

namespace oasys {

template<typename _type> int
serialize_basic_vector(oasys::SerializeAction* action,
                       std::string             name,
                       std::vector<_type>*     v)
{
    name += ".size";

    size_t size;
    size = v->size()
    action->process(name.c_str(), &size);
    v->resize(size);
    
    for (size_t i=0; i<size; ++i)
    {
        action->process(name.c_str(), &((*v)[i]));
    }
}

} // namespace oasys
#endif /* __SERIALIZEHELPERS_H__ */
