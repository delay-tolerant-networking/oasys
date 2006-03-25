/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _OASYS_LOGGER_H_
#define _OASYS_LOGGER_H_

#include "DebugUtils.h"
#include "Log.h"

namespace oasys {

/**
 * Many objects will, at constructor time, format a log target and
 * then use it throughout the code implementation -- the Logger class
 * encapsulates this common behavior.
 *
 * It therefore exports a set of functions (set_logpath, logpathf,
 * logpath_appendf) to manipulate the logging path post-construction.
 *
 * For example:
 *
 * @code
 * class LoggerTest : public Logger {
 * public:
 *   LoggerTest() : Logger("/loggertest")
 *   {
 *       logf(LOG_DEBUG, "initializing"); 		 // no path needed
 *       logf("/loggertest" LOG_DEBUG, "intiializing");  // but can be given
 *
 *       log_debug("loggertest initializing");           // macros work
 *       __log_debug("/loggertest", "initializing");     // but this case needs
 *                                                       // to be __log_debug
 *       set_logpath("/path");
 *       logpath_appendf("/a"); // logpath is "/path/a"
 *       logpath_appendf("/b"); // logpath is "/path/b"
 *   }
 * };
 * @endcode
 */

class Logger {
public:
    /**
     * Default constructor, initializes the logpath to the address of
     * the object.
     */
    Logger()
    {
        set_logpath(NULL);
    }

    /**
     * Constructor that initializes the logpath to a constant string.
     */
    Logger(const char* logpath)
    {
        set_logpath(logpath);
    }

    /**
     * Same thing with a std::string
     */
    Logger(const std::string& logpath)
    {
        set_logpath(logpath.c_str());
    }

    /**
     * Format function for logpath_.
     */
    inline void logpathf(const char* fmt, ...) PRINTFLIKE(2, 3);

    /**
     * Format function that appends to the end of the base path
     * instead of overwriting.
     *
     * For example:
     *
     * \code
     *   set_logpath("/path");
     *   logpath_appendf("/a"); // logpath is "/path/a"
     *   logpath_appendf("/b"); // logpath is "/path/b"
     * \endcode
     */
    inline void logpath_appendf(const char* fmt, ...) PRINTFLIKE(2, 3);
    
    /**
     * Assignment function.
     */
    void set_logpath(const char* logpath)
    {
        if (logpath == 0) {
            strncpy(logpath_, "/", sizeof(logpath_));
            baselen_ = 1;
        } else {
            strncpy(logpath_, logpath, sizeof(logpath_));
            baselen_ = strlen(logpath);
        }
    }

    /**
     * Wrapper around the base __log_enabled function that uses the
     * logpath_ instance.
     */
    inline bool log_enabled(log_level_t level) const
    {
        return oasys::__log_enabled(level, logpath_);
    }

    /**
     * Wrapper around vlogf that uses the logpath_ instance.
     */
    inline int vlogf(log_level_t level, const char *fmt, va_list args) const
    {
        return Log::instance()->vlogf(logpath_, level, this, fmt, args);
    }

    /**
     * Wrapper around logf that uses the logpath_ instance.
     */
    inline int logf(log_level_t level, const char *fmt, ...) const
        PRINTFLIKE(3, 4);

    /**
     * Yet another wrapper that just passes the log path straight
     * through, ignoring the logpath_ instance altogether. This means
     * a derived class doesn't need to explicitly qualify ::logf.
     */
    inline int logf(const char* logpath, log_level_t level,
                    const char* fmt, ...) const
        PRINTFLIKE(4, 5);

    /**
     * Wrapper around __logf, used by the log_debug style macros.
     *
     * (See Log.h for a full explanation of the need for __logf)
     */
    inline int __logf(log_level_t level, const char *fmt, ...) const
        PRINTFLIKE(3, 4);


    /**
     * As described in Log.h, the log_debug style macros call
     * log_enabled(level, path) before calling __logf. In the case of
     * the Logger, the path parameter isn't really the path, but is
     * actually the format string, so we actually call log_enabled on
     * the logpath_ instance.
     */
    inline bool __log_enabled(log_level_t level, const char* path) const
    {
        return oasys::__log_enabled(level, logpath_);
    }

    
    /**
     * And finally, one for log_multiline..
     */
    inline int log_multiline(log_level_t level, const char* msg) const
    {
        return Log::instance()->log_multiline(logpath_, level, 
                                              msg, this);
    }

    /**
     * @return current logpath
     */
    const char* logpath() { return logpath_; }

protected:
    char logpath_[LOG_MAX_PATHLEN];
    size_t baselen_;
};

void
Logger::logpathf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logpath_, sizeof(logpath_), fmt, ap);
    va_end(ap);
}

void
Logger::logpath_appendf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(&logpath_[baselen_], sizeof(logpath_) - baselen_, fmt, ap);
    va_end(ap);
}

int
Logger::logf(log_level_t level, const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vlogf(level, fmt, ap);
    va_end(ap);
    return ret;
}

int
Logger::__logf(log_level_t level, const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vlogf(level, fmt, ap);
    va_end(ap);
    return ret;
}

int
Logger::logf(const char* logpath, log_level_t level,
             const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    int ret = Log::instance()->vlogf(logpath, level, this, fmt, ap);
    va_end(ap);
    return ret;
}


} // namespace oasys

#endif /* _OASYS_LOGGER_H_ */
