#include "Time.h"

#if defined(_WIN32)

#include "compat/MSVC.h"

#else

#include <sys/time.h>
#include <time.h>

#endif

namespace oasys {

//----------------------------------------------------------------------------
void
Time::get_time() 
{
#if defined(_WIN32)

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
    usec_ = static_cast<u_int32_t>(ft % 1000000);

    // Get seconds
    ft /= 1000000;
    sec_ = static_cast<u_int32_t>(ft);

#else
    struct timeval tv;

    gettimeofday(&tv, 0);
    sec_  = static_cast<u_int32_t>(tv.tv_sec);
    usec_ = static_cast<u_int32_t>(tv.tv_usec);

#endif

    cleanup();
}

//----------------------------------------------------------------------------
double 
Time::in_seconds()
{
    return static_cast<double>(sec_) + 
        static_cast<double>(usec_)/1000000;
}

//----------------------------------------------------------------------------
u_int32_t
Time::in_microseconds()
{
    return sec_ * 1000000 + usec_;
}
    
//----------------------------------------------------------------------------
u_int32_t
Time::in_milliseconds()
{
    return sec_ * 1000 + usec_/1000;
}

//----------------------------------------------------------------------------
Time
Time::operator+(const Time& t) const
{
    return Time(t.sec_ + sec_, t.usec_ + usec_);
}

//----------------------------------------------------------------------------
Time
Time::operator-(const Time& t) const
{
    // a precondition for this fn to be correct is (*this >= t)
    Time t2(*this);
    if (t2.usec_ < t.usec_) {
        t2.usec_ += 1000000;
        t2.sec_  -= 1;
    }

    t2.sec_  -= t.sec_;
    t2.usec_ -= t.usec_;
    return t2;
}

//----------------------------------------------------------------------------
bool
Time::operator==(const Time& t) const
{
    return (sec_ == t.sec_) && (usec_ == t.usec_);
}

//----------------------------------------------------------------------------
bool
Time::operator<(const Time& t) const
{
    return (sec_ < t.sec_) 
        || ( (sec_ == t.sec_) && (usec_ < t.usec_));
}

//----------------------------------------------------------------------------
bool
Time::operator>(const Time& t) const
{
    return (sec_ > t.sec_) 
        || ( (sec_ == t.sec_) && (usec_ > t.usec_));    
}

//----------------------------------------------------------------------------
bool
Time::operator<=(const Time& t) const
{
    return (*this == t) || (*this < t);
}

//----------------------------------------------------------------------------
bool
Time::operator>=(const Time& t) const
{
    return (*this == t) || (*this > t);
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
