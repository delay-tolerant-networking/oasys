#ifndef __TIME_H__
#define __TIME_H__

namespace oasys {

/*!
 * Structure to handle time in a platform independent way.
 */
struct Time {
    int sec_;
    int usec_;

    Time(unsigned int sec  = 0,
         unsigned int usec = 0) 
        : sec_(sec), usec_(usec) 
    {
        cleanup();
    }
    
    //! Get the time into the structure
    void get_time();

    //! @{ Standard operators
    Time operator+(const Time& t);
    Time operator-(const Time& t);
    //! @}

    // Use default operator=

private:
    //! Cleanup the usec field wrt. sec
    void cleanup();
};

} // namespace oasys

#endif /* __TIME_H__ */
