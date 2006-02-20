#ifndef __DEBUGDUMPBUF_H__
#define __DEBUGDUMPBUF_H__

#include <stdlib.h>

namespace oasys {

struct DebugDumpBuf {
    static const size_t size_ = 1024 * 8;
    static char buf_[size_];
};

} // namespace oasys

#endif /* __DEBUGDUMPBUF_H__ */
