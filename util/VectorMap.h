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
