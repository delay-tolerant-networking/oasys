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
 * To support logging on a class basis instead of on a hierarchical
 * basis, the Logger also takes the name of the class in the
 * constructor. Since, by convention, paths start with '/' and classes
 * cannot, this allows independent logging control by class name or by
 * logging path.
 *
 * For example:
 *
 * @code
 * class LoggerTest : public Logger {
 * public:
 *   LoggerTest() : Logger("LoggerTest", "/logger/test")
 *   {
 *       logf(LOG_DEBUG, "initializing"); 		 // no path needed
 *       logf("/logger/test" LOG_DEBUG, "intiializing"); // but can be given
 *
 *       log_debug("loggertest initializing");           // macros work
 *       __log_debug("/logger/test", "initializing");    // but this case needs
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
     * Constructor that initializes the logpath with a printf style
     * format string.
     */
    inline Logger(const char* classname, const char* fmt, ...) PRINTFLIKE(3, 4);

    /**
     * Constructor that initializes to a constant std::string.
     */
    Logger(const char* classname, const std::string& logpath)
        : classname_(classname)
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
     *
     * Also, all Logger instances store the class name of the
     * implementation, and logging can be enabled/disabled on that
     * target as well, so we check the class name as well.
     */
    inline bool log_enabled(log_level_t level) const
    {
        return oasys::__log_enabled(level, logpath_) ||
            oasys::__log_enabled(level, classname_);
    }

    /**
     * As described in Log.h, the log_debug style macros call
     * log_enabled(level, path) before calling __logf. In the case of
     * the Logger, the path parameter isn't really the path, but is
     * actually the format string, so we actually call log_enabled on
     * the logpath_ instance.
     *
     * Also, all Logger instances store the class name of the
     * implementation, and logging can be enabled/disabled on that
     * target as well, so we check it.
     */
    inline bool __log_enabled(log_level_t level, const char* path) const
    {
        return oasys::__log_enabled(level, logpath_) ||
            oasys::__log_enabled(level, classname_);
            
    }

    
    /**
     * Wrapper around vlogf that uses the logpath_ instance.
     */
    inline int vlogf(log_level_t level, const char *fmt, va_list args) const
    {
        return Log::instance()->vlogf(logpath_, level, classname_, this,
                                      fmt, args);
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
     * And finally, one for log_multiline..
     */
    inline int log_multiline(log_level_t level, const char* msg) const
    {
        return Log::instance()->log_multiline(logpath_, level, 
                                              classname_, this, msg);
    }

    /**
     * @return current logpath
     */
    const char* logpath() { return logpath_; }

protected:
    const char* classname_;
    char logpath_[LOG_MAX_PATHLEN];
    size_t baselen_;
};

//----------------------------------------------------------------------
Logger::Logger(const char* classname, const char* fmt, ...)
    : classname_(classname)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logpath_, sizeof(logpath_), fmt, ap);
    va_end(ap);
    baselen_ = strlen(logpath_);
}

//----------------------------------------------------------------------
void
Logger::logpathf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logpath_, sizeof(logpath_), fmt, ap);
    va_end(ap);
}

//----------------------------------------------------------------------
void
Logger::logpath_appendf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(&logpath_[baselen_], sizeof(logpath_) - baselen_, fmt, ap);
    va_end(ap);
}

//----------------------------------------------------------------------
int
Logger::logf(log_level_t level, const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vlogf(level, fmt, ap);
    va_end(ap);
    return ret;
}

//----------------------------------------------------------------------
int
Logger::__logf(log_level_t level, const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    int ret = vlogf(level, fmt, ap);
    va_end(ap);
    return ret;
}

//----------------------------------------------------------------------
int
Logger::logf(const char* logpath, log_level_t level,
             const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    int ret = Log::instance()->vlogf(logpath, level, classname_, this,
                                     fmt, ap);
    va_end(ap);
    return ret;
}


} // namespace oasys

#endif /* _OASYS_LOGGER_H_ */
