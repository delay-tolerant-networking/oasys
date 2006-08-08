#ifndef __TIME_H__
#define __TIME_H__

#include "../compat/inttypes.h"

namespace oasys {

/*!
 * Structure to handle time in a platform independent way.
 */
struct Time {
    u_int32_t sec_;
    u_int32_t usec_;

    Time(u_int32_t sec  = 0,
         u_int32_t usec = 0) 
        : sec_(sec), usec_(usec) 
    {
        cleanup();
    }
    
    //! Get the time into the structure
    void get_time();
    
    //! @return Time in seconds as a floating point number
    double in_seconds();

    //! @return Time in microseconds
    u_int32_t in_microseconds();
    
    //! @return Time in milliseconds
    u_int32_t in_milliseconds();

    //! @return Milliseconds elapsed between this timer and the
    // current time.
    u_int32_t elapsed_ms();

    //! @{ Standard operators
    Time operator+(const Time& t)  const;
    Time operator-(const Time& t)  const;
    Time& operator+=(const Time& t);
    Time& operator-=(const Time& t);

    bool operator==(const Time& t) const;
    bool operator<(const Time& t)  const;
    bool operator>(const Time& t)  const;
    bool operator>=(const Time& t) const;
    bool operator<=(const Time& t) const;
    //! @}

    // Use default operator=

    //! Cleanup the usec field wrt. sec
    void cleanup();
};

} // namespace oasys

#endif /* __TIME_H__ */
