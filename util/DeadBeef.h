#ifndef __DEADBEEF_H__
#define __DEADBEEF_H__

namespace oasys {

inline void 
fill_with_the_beef(void* buf, size_t len) {
    char beef[]     = { 0xd, 0xe, 0xa, 0xd, 0xb, 0xe, 0xe, 0xf };
    size_t beef_len = 8;
    char* p = reinterpret_cast<char*>(buf);
    
    for (size_t i=0; i<len; ++i)
        *p++ = beef[i%beef_len];
}

} // namespace oasys
#endif /* __DEADBEEF_H__ */
