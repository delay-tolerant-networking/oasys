#ifdef __win32__
#include <io.h>
#else
// XXX/bowei
#endif

#include "LogSink.h"

namespace oasys {

//----------------------------------------------------------------------------
FileLogSink::FileLogSink(const char* filename)
    : fd_(0)
{
    strcpy(filename_, filename);
}

//----------------------------------------------------------------------------
void
FileLogSink::rotate()
{ 
    // XXX/bowei -- TODO
}

//----------------------------------------------------------------------------
void
FileLogSink::log(const char* str)
{
    // Porting note: We use the raw functions here because the IO
    // functions use logging. Have to start somewhere.
#ifdef __win32__
    _write(fd_, str, strlen(str));
#else
#endif
}


//----------------------------------------------------------------------------
RingBufferLogSink::RingBufferLogSink()
    : cur_buf_(0)
{
    memset(static_buf_, 0, NUM_BUFFERS * BUF_LEN + 16);

    // Guards for easy finding upon a core dump
    memcpy(static_buf_, "|RNGBUF|", 8);
    memcpy(static_buf_ + NUM_BUFFERS * BUF_LEN + 8, "|RNGBUF|", 8);

    buf_ = static_buf_ + 8;
}

//----------------------------------------------------------------------------
void
RingBufferLogSink::log(const char* str)
{
    cur_buf_  = (cur_buf_ + 1) % NUM_BUFFERS;
    char* buf = at(cur_buf_);
    
    size_t len = strlen(str);
    if (len > BUF_LEN - 4) {
        len = BUF_LEN - 4;
        memcpy(cur_buf_, "...", 4);
    } else {
        cur_buf_[len] = '\0';
    }
    memcpy(cur_buf_, str, len);
}

//----------------------------------------------------------------------------

} // namespace oasys
