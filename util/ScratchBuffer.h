#ifndef __SCRATCH_BUFFER__
#define __SCRATCH_BUFFER__

#include <cstdlib>

namespace oasys {
/**
 * This is a class which caches a reusable scratch buffer, obviating
 * the need to constantly malloc, the free a buffer in order to
 * serialize into/out of.
 */
class ScratchBuffer {
public:
    ScratchBuffer(size_t size = 0);
    ~ScratchBuffer();
    
    char*  buf(size_t size);
    char*  buf()  { return buf_; }
    size_t size() { return size_; }
    
private:
    char*  buf_;
    size_t size_;
};

}; // namespace oasys

#endif //__SCRATCH_BUFFER_H__
