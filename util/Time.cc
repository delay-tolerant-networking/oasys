#include "Time.h"

#ifndef __win32__
#include <sys/time.h>
#include <time.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace oasys {

//----------------------------------------------------------------------------
void
Time::get_time() 
{
#ifndef __win32__

    struct timeval tv;

    gettimeofday(&tv, 0);
    sec_  = (unsigned int) tv.tv_sec;
    usec_ = (unsigned int) tv.tv_usec;

#else

    SYSTEMTIME systime;
    FILETIME   filetime;

    GetSystemTime(&systime);
    SystemTimeToFileTime(&systime, &filetime);
    
    // FileTime is in 100-nanosecond intervals since UTC January 1, 1601
    // 116444736000000000 0.1 usec difference
    // 11644473600 sec difference
    __int64 ft;
    ft = filetime.dwLowDateTime;
    ft |= ((__int64)filetime.dwHighDateTime)<<32;

    ft -= 116444736000000000;

    // Get microseconds
    ft /= 10; 
    usec_ = ft % 1000000;

    // Get seconds
    ft /= 1000000;
    sec_ = ft;

#endif

    cleanup();
}

//----------------------------------------------------------------------------
Time
Time::operator+(const Time& t)
{
    return Time(t.sec_ + sec_, t.usec_ + usec_);
}

//----------------------------------------------------------------------------
Time
Time::operator-(const Time& t)
{
    return Time(sec_ - t.sec_, usec_ - t.usec_);
}
 
//----------------------------------------------------------------------------
void
Time::cleanup()
{
    if (usec_ > 1000000) 
    {
        sec_ += usec_ / 1000000;
        usec_ /= 1000000;
    }
}

} // namespace oasys

#if 0

#include <cstdio>

int 
main()
{
    oasys::Time t;

    t.get_time();

    printf("%d %d\n", t.sec_, t.usec_);

    return 0;
} 

#endif
