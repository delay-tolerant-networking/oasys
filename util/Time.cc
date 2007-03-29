/*
 *    Copyright 2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

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

//----------------------------------------------------------------------
u_int32_t
Time::elapsed_ms()
{
    Time t;
    t.get_time();
    t -= *this;
    return t.in_milliseconds();
}

//----------------------------------------------------------------------------
Time
Time::operator+(const Time& t) const
{
    return Time(t.sec_ + sec_, t.usec_ + usec_);
}

//----------------------------------------------------------------------------
Time&
Time::operator+=(const Time& t)
{
    sec_  += t.sec_;
    usec_ += t.usec_;
    cleanup();
    return *this;
}

//----------------------------------------------------------------------------
Time&
Time::operator-=(const Time& t)
{
    // a precondition for this fn to be correct is (*this >= t)
    if (usec_ < t.usec_) {
        usec_ += 1000000;
        sec_  -= 1;
    }

    sec_  -= t.sec_;
    usec_ -= t.usec_;
    return *this;
}

//----------------------------------------------------------------------------
Time
Time::operator-(const Time& t) const
{
    // a precondition for this fn to be correct is (*this >= t)
    Time t2(*this);
    t2 -= t;
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
