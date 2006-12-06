#ifndef __BASE16_H__
#define __BASE16_H__

#include "../debug/DebugUtils.h"

namespace oasys {

class Base16 {
public:
    /*!
     * Encode input into Base16. out should be *2 the size of in. Will
     * truncate buffer if there is not enough space.
     *
     * @return Number of bytes encoded.
     */
    static size_t encode(char* in, size_t in_len, 
                         char* out16, size_t out16_len);
    
    /*!
     * Decode the input from Base16. out should be 1/2 the size of
     * in. Will truncate buffer if there is not enough space.
     *
     * @return Number of bytes decoded.
     */
    static size_t decode(char* in16, size_t in16_len, 
                         char* out, size_t out_len);
};

};

#endif /* __BASE16_H__ */
