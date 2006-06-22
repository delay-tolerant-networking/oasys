#include <cstdlib>

#include "../debug/DebugUtils.h"
#include "Random.h"

namespace oasys {

ByteGenerator::ByteGenerator(unsigned int seed)
    : cur_(seed) 
{ 
    next(); 
}

void 
ByteGenerator::fill_bytes(void* buf, size_t size)
{
    char* p = static_cast<char*>(buf);
    
    for(size_t i=0; i<size; ++i) {
        *p = cur_ % 256;
        ++p;
        next();
    }
}

void ByteGenerator::next() 
{
    cur_ = (A * cur_ + C) % M;
}

PermutationArray::PermutationArray(size_t size)
    : size_(size)
{
    array_.reserve(size_);
    for(unsigned int i=0; i<size_; ++i) {
        array_[i] = i;
    }

    for(unsigned int i=0; i<size_ - 1; ++i) {
        size_t temp;
        unsigned int pos = Random::rand(size_-i-1)+i+1;
        
        temp = array_[i];
        array_[i]   = array_[pos];
        array_[pos] = temp;
    }
}

unsigned int
PermutationArray::map(unsigned int i) {
    ASSERT(i<size_); 
    return array_[i];
}

}; // namespace oasys

#if 0
#include <iostream>

using namespace std;
using namespace oasys;

int
main()
{
    const size_t SIZE = 20;
    PermutationArray ar(SIZE);

    for(int i=0;i<SIZE;++i) {
        cout << i << "->" << ar.map(i) << endl;
    }
}
#endif
