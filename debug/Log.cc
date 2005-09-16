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

#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <algorithm>

#include "config.h"
#include "DebugUtils.h"
#include "Log.h"
#include "compat/inttypes.h"
#include "thread/SpinLock.h"
#include "thread/Timer.h"
#include "io/IO.h"

/**
 * Namespace for the oasys library of system support classes.
 */
namespace oasys {

// we can't use the ASSERT from DebugUtils.h since that one calls logf :-)
#undef ASSERT
#define ASSERT(x) __log_assert(x, #x, __FILE__, __LINE__)

void
__log_assert(bool x, const char* what, const char* file, int line)
{
    if (! (x)) {
        fprintf(stderr, "LOGGING ASSERTION FAILED (%s) at %s:%d\n",
                what, file, line);
        abort();
    }
}

level2str_t log_levelnames[] =
{
    { "debug",   LOG_DEBUG },
    { "info",    LOG_INFO },
    { "warning", LOG_WARN },
    { "warn",    LOG_WARN },
    { "error",   LOG_ERR },
    { "err",     LOG_ERR },
    { "critical",LOG_CRIT },
    { "crit",    LOG_CRIT },
    { "always",  LOG_ALWAYS },
    { NULL,      LOG_INVALID }
};

Log* Log::instance_ = NULL;
bool Log::inited_   = false;

Log::Log()
    : output_flags_(OUTPUT_TIME),
      logfd_(-1),
      default_threshold_(LOG_DEFAULT_THRESHOLD)
{
    lock_      = new SpinLock();
    rule_list_ = new RuleList();
}

void
Log::init(const char* logfile, log_level_t defaultlvl,
          const char* prefix, const char* debug_path)
{
    Log* log = new Log();
    log->do_init(logfile, defaultlvl, prefix, debug_path);
}

void
Log::do_init(const char* logfile, log_level_t defaultlvl,
             const char* prefix, const char *debug_path)
{
    ASSERT(instance_ == NULL);
    ASSERT(!inited_);

    instance_ = this;

    // Open the output file descriptor
    logfile_.assign(logfile);
    if (logfile_.compare("-") == 0) {
        logfd_ = 1; // stdout
    } else {
        logfd_ = open(logfile_.c_str(), O_CREAT | O_WRONLY | O_APPEND,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        
        if (logfd_ < 0) {
            fprintf(stderr, "fatal error opening log file '%s': %s\n",
                    logfile_.c_str(), strerror(errno));
            exit(1);
        }
    }

    if (prefix)
        prefix_.assign(prefix);

    ASSERT(lock_);
    ScopeLock lock(lock_);

    default_threshold_ = defaultlvl;
    parse_debug_file(debug_path);
    inited_ = true;
}

void
Log::parse_debug_file(const char* debug_path)
{
    // if the debug path isn't specified, we're either reparsing or
    // the user doesn't want a debug file
    if (debug_path == 0)
        debug_path = debug_path_.c_str();

    // ok, now we can see if there's just nothing to do...
    if (debug_path[0] == '\0')
        return;
    
    // note that we do as much as we can without taking the lock to
    // avoid holding up other parts of the system. the lock is only
    // taken to swap the old and new lists
    RuleList* old_rule_list;
    RuleList* new_rule_list = new RuleList();

    // handle ~/ in the debug_path
    if ((debug_path[0] == '~') && (debug_path[1] == '/')) {
        char path[256];
        const char* home = getenv("HOME");
        if (home && *home) {
            if (home[strlen(home) - 1] == '/') {
                // avoid // in expanded path
                snprintf(path, sizeof(path), "%s%s", home, debug_path + 2);
            } else {
                snprintf(path, sizeof(path), "%s%s", home, debug_path + 1);
            }
        } else {
            fprintf(stderr, "Can't expand ~ to HOME in debug_path %s\n",
                    debug_path);
            return;
        }
        debug_path_.assign(path);
        debug_path = debug_path_.c_str();
    } else {
        debug_path_.assign(debug_path);
    }

    // check if we can open the file
    FILE *fp = fopen(debug_path, "r");
    if (fp == NULL) {
        return;
    }

    char buf[1024];
    int linenum = 0;
    
    while (!feof(fp)) {
        if (fgets(buf, sizeof(buf), fp) > 0) {
	    char *line = buf;
	    char *logpath;
	    char *level;
	    char *rest;

	    ++linenum;

            logpath = line;

            // skip leading whitespace
            while (*logpath && isspace(*logpath)) ++logpath;
            if (! *logpath) {
                // blank line
                continue;
            }

            // skip comment lines
            if (logpath[0] == '#')
                continue;

            // printing options
            if (logpath[0] == '%') {
                if (strstr(logpath, "no_time") != 0) {
                    output_flags_ &= ~OUTPUT_TIME;
                }
                if (strstr(logpath, "brief") != 0) {
                    output_flags_ |= OUTPUT_SHORT;
                }
                if (strstr(logpath, "color") != 0) {
                    output_flags_ |= OUTPUT_COLOR;
                }
                if (strstr(logpath, "object") != 0) {
                    output_flags_ |= OUTPUT_OBJ;
                }
                
                continue;
            }

            // find the end of path and null terminate
            level = logpath;
            while (*level && !isspace(*level)) ++level;
            *level = '\0';
            ++level;

            // skip any other whitespace
            while (level && isspace(*level)) ++level;
            if (!level) {
 parse_err:
                fprintf(stderr, "Error in log configuration %s line %d\n",
                        debug_path, linenum);
                continue;
            }

            // null terminate the level
            rest = level;
            while (rest && !isspace(*rest)) ++rest;
            if (rest) *rest = '\0';

            log_level_t threshold = str2level(level);
            if (threshold == LOG_INVALID) {
                goto parse_err;
            }

            new_rule_list->push_back(Rule(logpath, threshold));
        }
    }
    
    fclose(fp);

    sort_rules(new_rule_list);

    lock_->lock();
    old_rule_list = rule_list_;
    rule_list_ = new_rule_list;
    if (inited_) {
        logf("/log", LOG_ALWAYS, "reparsed debug file... found %d rules",
             (int)new_rule_list->size());
    }
    lock_->unlock();

    delete old_rule_list;
}

void
Log::sort_rules(RuleList* rule_list)
{
    // Now that it's been parsed, sort the list based on the length
    std::sort(rule_list->begin(), rule_list->end(), RuleCompare());

#ifndef NDEBUG
    // Sanity assertion
    if (rule_list->size() > 0) {
        RuleList::iterator itr;
        Rule* prev = 0;
        for (itr = rule_list->begin(); itr != rule_list->end(); ++itr) {
            if (prev != 0) {
                ASSERT(prev->path_.length() >= itr->path_.length());
                prev = &(*itr);
            }
        }
    }
#endif // NDEBUG
}

void
Log::print_rules()
{
    ASSERT(inited_);
    ScopeLock l(lock_);
    
    RuleList::iterator iter = rule_list_->begin();
    printf("Logging rules:\n");
    for (iter = rule_list_->begin(); iter != rule_list_->end(); iter++) {
        printf("\t%s\n", iter->path_.c_str());
    }
}

Log::Rule *
Log::find_rule(const char *path)
{
    ASSERT(inited_);
    ScopeLock l(lock_);
    
    /*
     * The rules are stored in decreasing path lengths, so the first
     * match is the best (i.e. most specific).
     */
    size_t pathlen = strlen(path);

    RuleList::iterator iter;
    Rule* rule;
    for (iter = rule_list_->begin(); iter != rule_list_->end(); iter++) {
        rule = &(*iter);

        if (rule->path_.length() > pathlen) {
            continue; // can't be a match
        }

        const char* rule_path = rule->path_.data();
        size_t rulelen = rule->path_.length();
        
        size_t minlen = (pathlen < rulelen) ? pathlen : rulelen;
        
        if (strncmp(rule_path, path, minlen) == 0) {
            return rule; // match!
	}
    }

    return NULL; // no match :-(
}

void
Log::add_debug_rule(const char* path, log_level_t threshold)
{
    ASSERT(inited_);
    ScopeLock l(lock_);
    ASSERT(path);
    rule_list_->push_back(Rule(path, threshold));
    sort_rules(rule_list_);
}

void
Log::rotate()
{
    ScopeLock l(lock_);

    if (logfd_ == 1) {
        logf("/log", LOG_WARN, "can't rotate when using stdout for logging");
        return;
    }
    
    int newfd = open(logfile_.c_str(), O_CREAT | O_WRONLY | O_APPEND,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (newfd < 0) {
        logf("/log", LOG_ERR, "error re-opening log file for rotate: %s",
             strerror(errno));
        logf("/log", LOG_ERR, "keeping old log file open");
        return;
    }

    logf("/log", LOG_INFO, "closing log file for rotation");
    close(logfd_);
    
    logfd_ = newfd;
    logf("/log", LOG_INFO, "reopened log file after log rotate");
}

static void
rotate_handler(int sig)
{
    Log::instance()->rotate();
}

void
Log::add_rotate_handler(int sig)
{
    logf("/log", LOG_DEBUG, "adding log rotate signal handler");
    TimerSystem::instance()->add_sighandler(sig, rotate_handler);
}

static RETSIGTYPE
reparse_handler(int sig)
{
    Log::instance()->parse_debug_file();
}

void
Log::add_reparse_handler(int sig)
{
    logf("/log", LOG_DEBUG, "adding log reparse signal handler");
    TimerSystem::instance()->add_sighandler(sig, reparse_handler);
}

/**
 * Simple class to incremement a counter in the constructor and then
 * decrement when the class goes out of scope.
 */
class ScopedIncr {
public:
    ScopedIncr(int* v) : val_(v)
    {
#ifdef __NO_ATOMIC__
        (*val_)++;
#else
        atomic_incr(val_);
#endif
    }
    ~ScopedIncr()
    {
#ifdef __NO_ATOMIC__
        (*val_)--;
#else
        atomic_decr(val_);
#endif
    }
protected:
    int* val_;
};

log_level_t
Log::log_level(const char *path)
{
    Rule *r = find_rule(path);

    if (r) {
	return r->level_;
    } else {
	return default_threshold_;
    }
}

void
Log::getlogtime(struct timeval* tv)
{
    // by default, we just use the current time of day
    ::gettimeofday(tv, 0);
}

size_t
Log::gen_prefix(char* buf, size_t buflen, 
                const char* path, log_level_t level, const void* obj)
{
    size_t len;
    char *ptr = buf;

    char* pretty_begin = "";
    char* pretty_end   = "";
    char* pretty_type  = "";
    
    char obj_address[16];
    obj_address[0] = '\0';

    if ( (output_flags_ & OUTPUT_OBJ) && obj != 0) {
        snprintf(obj_address, 16, "%p ", obj);
    }

    if(output_flags_ & OUTPUT_COLOR) {
        pretty_begin = "\033[33m";
        pretty_end   = "\033[0m";
        pretty_type  = "\033[36m";
    }

    if (prefix_.size() > 0) {
        len = snprintf(ptr, buflen, "%s[%s ", 
                       pretty_begin, prefix_.c_str());
        buflen -= len;
        ptr += len;
    } else {
        len = snprintf(ptr, buflen, "%s[", pretty_begin);
        buflen -= len;
        ptr += len;        
    }
    
    if (output_flags_ & OUTPUT_TIME) {
        struct timeval tv;
        getlogtime(&tv);
        len = snprintf(ptr, buflen, 
                       "%ld.%06ld ",
                       (long)tv.tv_sec, (long)tv.tv_usec);
        buflen -= len;
        ptr += len;
    }

    if (output_flags_ & OUTPUT_SHORT) {
        len = snprintf(ptr, buflen, "%.19s ", path);
        buflen -= len;
        ptr    += len;

        for (int j = len; j<20; ++j) {
            --buflen;
            *ptr++ = ' ';
        }
        
        len = snprintf(ptr, buflen, "%s%s%c%s]%s ",
                       pretty_type,
                       obj_address,
                       toupper(level2str(level)[0]),
                       pretty_begin,
                       pretty_end);
        buflen -= len;
        ptr    += len;
    } else {
        len = snprintf(ptr, buflen, 
                       "%s %s%s%s%s]%s ",
                       path, 
                       pretty_type,
                       obj_address,
                       level2str(level),
                       pretty_begin,
                       pretty_end);
        buflen -= len;
        ptr += len;        
    }
    
    return ptr - buf;
}

int
Log::vlogf(const char* path, log_level_t level, const void* obj,
           const char* fmt, va_list ap)
{
    ASSERT(inited_);

    char pathbuf[LOG_MAX_PATHLEN];

    // try to catch crashes due to buffer overflow with some guard
    // bytes at the end
    static const char guard[] = "[guard]";
    char buf[LOG_MAX_LINELEN + sizeof(guard)];
    memcpy(&buf[LOG_MAX_LINELEN], guard, sizeof(guard));
    char* ptr = buf;
    
    // try to catch recursive calls that can result from an
    // assert/panic from somewhere within the logging code
    static int threads_in_vlogf = 0;
    ScopedIncr incr(&threads_in_vlogf);
    
    if (threads_in_vlogf > 10) {
        fprintf(stderr, "fatal error: vlogf called recursively:\n");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n"); // since logf doesn't include newlines
        abort();
    }
    
    // throw a lock around the whole function
    ScopeLock l(lock_);

    /* Make sure that paths that don't start with a slash still show up. */
    if (*path != '/') {
	snprintf(pathbuf, sizeof pathbuf, "/%s", path);
	path = pathbuf;
    }

    // bail if we're not going to output the line.
    if (! __log_enabled(level, path))
        return 0;
    
    size_t buflen = LOG_MAX_LINELEN - 1; /* Save a character for newline. */
    size_t len;

    // generate the log prefix
    len = gen_prefix(buf, buflen, path, level, obj);
    buflen -= len;
    ptr += len;
    
    // generate the log string
    len = vsnprintf(ptr, buflen, fmt, ap);

    if (len >= buflen) {
        // handle truncated lines -- note that the last char in the
        // buffer is free since we reserved it for the newline
        const char* trunc = "... (truncated)\n";
        len = strlen(trunc);
        strncpy(&buf[LOG_MAX_LINELEN - len - 1], trunc, len + 1);
        buflen = LOG_MAX_LINELEN - 1;
        buf[LOG_MAX_LINELEN - 1] = '\0';

    } else {
        // make sure there's a trailing newline
        buflen -= len;
        ptr += len;
        ASSERT(ptr <= (buf + LOG_MAX_LINELEN - 2));
        
        if (ptr[-1] != '\n') {
            *ptr++ = '\n';
            *ptr = 0;
        }
        
        buflen = ptr - buf;
    }

    if (memcmp(&buf[LOG_MAX_LINELEN], guard, sizeof(guard)) != 0) {
        PANIC("logf buffer overflow");
    }

    // do the write, making sure to drain the buffer. since stdout was
    // set to nonblocking, the spin lock prevents other threads from
    // jumping in here
    int ret = IO::writeall(logfd_, buf, buflen);
    ASSERT(ret == (int)buflen);

    return buflen;
};

int
Log::log_multiline(const char* path, log_level_t level, const char* msg,
                   const void* obj)
{
    ASSERT(inited_);

    char pathbuf[LOG_MAX_PATHLEN];

    // throw a lock around the whole function
    ScopeLock l(lock_);

    /* Make sure that paths that don't start with a slash still show up. */
    if (*path != '/') {
	snprintf(pathbuf, sizeof pathbuf, "/%s", path);
	path = pathbuf;
    }

    // bail if we're not going to output the line.
    if (! __log_enabled(level, path))
        return 0;

    // generate the log prefix
    char prefix[LOG_MAX_LINELEN];
    size_t prefix_len = gen_prefix(prefix, sizeof(prefix), path, level, obj);

    size_t iov_total = 128; // 64 lines of output
    struct iovec  static_iov[iov_total];
    struct iovec* dynamic_iov = NULL;
    struct iovec* iov = static_iov;
    size_t iov_cnt = 0;
    
    // scan the string, generating an entry for the prefix and the
    // message line for each one.
    const char* end = msg;
    while (*msg != '\0') {
        end = strchr(msg, '\n');
        if (end == NULL) {
            PANIC("multiline log message must end in trailing newline");
        }
        iov[iov_cnt].iov_base = prefix;
        iov[iov_cnt].iov_len  = prefix_len;
        ++iov_cnt;
        
        iov[iov_cnt].iov_base = (char*)msg;
        iov[iov_cnt].iov_len  = end - msg + 1; // include newline
        ++iov_cnt;

        msg = end + 1;

        if (iov_cnt == iov_total) {
            PANIC("XXX/demmer implement dynamic_iov for > 64 lines");
            (void)dynamic_iov;
        }
    }
    
    // do the write, making sure to drain the buffer. since stdout was
    // set to nonblocking, the spin lock prevents other threads from
    // jumping in here
    int ret = IO::writevall(logfd_, iov, iov_cnt);
    return ret;
}

} // namespace oasys
