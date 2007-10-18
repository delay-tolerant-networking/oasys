/*
 *    Copyright 2004-2006 Intel Corporation
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

#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <algorithm>

#include "DebugUtils.h"
#include "Log.h"
#include "compat/inttypes.h"
#include "io/IO.h"
#include "thread/SpinLock.h"
#include "thread/Timer.h"
#include "util/StringBuffer.h"
#include "util/Glob.h"
#include "util/Time.h"

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
    if (! (x)) 
    {
        fprintf(stderr, "LOGGING ASSERTION FAILED (%s) at %s:%d\n",
                what, file, line);
        oasys_break();
        abort();
    }
}

level2str_t log_levelnames[] =
{
    { "debug",   LOG_DEBUG },
    { "info",    LOG_INFO },
    { "notice",  LOG_NOTICE },
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
bool Log::__debug_no_panic_on_overflow = false;

Log::Log()
    : output_flags_(OUTPUT_PATH | OUTPUT_TIME | OUTPUT_LEVEL),
      logfd_(-1),
      default_threshold_(LOG_DEFAULT_THRESHOLD)
{
    output_lock_ = new SpinLock();
    rule_list_   = &rule_lists_[1];
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

#ifdef NDEBUG
    if (defaultlvl == LOG_DEBUG) {
        fprintf(stderr, "WARNING: default log level debug invalid for "
                "non-debugging build\n");
        defaultlvl = LOG_INFO;
    }
#endif
    
    default_threshold_ = defaultlvl;
    parse_debug_file(debug_path);

    inited_ = true;
    instance_ = this;
}

Log::~Log()
{
    close(logfd_);
    logfd_ = -1;

    delete output_lock_;
}

void
Log::shutdown()
{
    delete instance_;
    instance_ = NULL;
    inited_ = false;
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
    
    // handle double buffering for the rule lists
    RuleList* old_rule_list = rule_list_;
    RuleList* new_rule_list = (rule_list_ == &rule_lists_[0]) ?
                              &rule_lists_[1] : &rule_lists_[0];

    ASSERT(new_rule_list != old_rule_list);
    new_rule_list->clear();

    // handle ~/ in the debug_path
    if ((debug_path[0] == '~') && (debug_path[1] == '/')) {
        char path[256];
        const char* home = getenv("HOME");

        if (home == 0 || *home == 0) {
            home = "/";
        }
        
        if (home[strlen(home) - 1] == '/') {
            // avoid // in expanded path
            snprintf(path, sizeof(path), "%s%s", home, debug_path + 2);
        } else {
            snprintf(path, sizeof(path), "%s%s", home, debug_path + 1);
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
                if (strstr(logpath, "no_path") != 0) {
                    output_flags_ &= ~OUTPUT_PATH;
                }
                if (strstr(logpath, "no_time") != 0) {
                    output_flags_ &= ~OUTPUT_TIME;
                }
                if (strstr(logpath, "no_level") != 0) {
                    output_flags_ &= ~OUTPUT_LEVEL;
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
                if (strstr(logpath, "classname") != 0) {
                    output_flags_ |= OUTPUT_CLASSNAME;
                }
                /*if (strstr(logpath, "walltime") != 0) {
                    output_flags_ |= OUTPUT_WALLTIME;
                }*/
                
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

#ifdef NDEBUG
            if (threshold == LOG_DEBUG) {
                fprintf(stderr, "WARNING: debug level log rule for path %s "
                        "ignored in non-debugging build\n",
                        logpath);
                continue;
            }
#endif
            
            new_rule_list->push_back(Rule(logpath, threshold));
        }
    }
    
    fclose(fp);

    sort_rules(new_rule_list);

    if (inited_) {
        logf("/log", LOG_ALWAYS, "reparsed debug file... found %d rules",
             (int)new_rule_list->size());
    }

    rule_list_ = new_rule_list;
}

bool
Log::rule_compare(const Rule& rule1, const Rule& rule2)
{
    if (rule1.path_.length() > rule2.path_.length())
        return true;

    if ((rule1.path_.length() == rule2.path_.length()) &&
        (rule1.level_ < rule2.level_))
        return true;

    return false;
}

void
Log::sort_rules(RuleList* rule_list)
{
    // Now that it's been parsed, sort the list based on the length
    // and the level (if the lengths are equal)
    std::sort(rule_list->begin(), rule_list->end(), Log::rule_compare);

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
Log::dump_rules(StringBuffer* buf)
{
    ASSERT(inited_);

    RuleList* rule_list = rule_list_;
    RuleList::iterator iter = rule_list->begin();
    for (iter = rule_list->begin(); iter != rule_list->end(); iter++) {
        buf->appendf("%s %s\n", iter->path_.c_str(), level2str(iter->level_));
    }
}

Log::Rule *
Log::find_rule(const char *path)
{
    ASSERT(inited_);
    
    /*
     * The rules are stored in decreasing path lengths, so the first
     * match is the best (i.e. most specific).
     */
    size_t pathlen = strlen(path);

    RuleList::iterator iter;
    Rule* rule;
    RuleList* rule_list = rule_list_;
    for (iter = rule_list->begin(); iter != rule_list->end(); iter++) 
    {
        rule = &(*iter);

        if (rule->path_.length() > pathlen) {
            continue; // can't be a match
        }

        const char* rule_path = rule->path_.data();
        size_t rulelen = rule->path_.length();
        
        size_t minlen = (pathlen < rulelen) ? pathlen : rulelen;
        
        if (strncmp(rule_path, path, minlen) == 0) 
        {
            return rule; // match!
        }

        // XXX/bowei cheap dirty hack to add glob expressions to the
        // logging. I'm sick of seeing three billion logs for refs
        // flying by.
        if (rule_path[0] == '+' &&
            Glob::fixed_glob(rule_path + 1, path))
        {
            return rule;
        }
    }

    return NULL; // no match :-(
}

void
Log::redirect_stdio()
{
    stdio_redirected_ = true;

    ASSERT(logfd_ >= 0);

    int err;
    if ((err = dup2(logfd_, 1)) != 1) {
        logf("/log", LOG_ERR, "error redirecting stdout: %s", strerror(errno));
    }

    if ((err = dup2(logfd_, 2)) != 2) {
        logf("/log", LOG_ERR, "error redirecting stderr: %s", strerror(errno));
    }
}

void
Log::rotate()
{
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

    output_lock_->lock("Log::rotate");

    logf("/log", LOG_NOTICE, "closing log file for rotation");
    close(logfd_);
    
    logfd_ = newfd;
    logf("/log", LOG_NOTICE, "log rotate successfully reopened file");


    if (stdio_redirected_) {
        redirect_stdio();
    }
    
    output_lock_->unlock();
}

static void
rotate_handler(int sig)
{
    (void)sig;
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
    (void)sig;
    Log::instance()->parse_debug_file();
}

void
Log::add_reparse_handler(int sig)
{
    logf("/log", LOG_DEBUG, "adding log reparse signal handler");
    TimerSystem::instance()->add_sighandler(sig, reparse_handler);
}

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

size_t
Log::gen_prefix(char* buf, size_t buflen, 
                const char* path, log_level_t level,
                const char* classname, const void* obj)
{
    size_t len;
    char *ptr = buf;

    char* color_begin = "";
    char* color_end   = "";
    char* color_level = "";
    
    if (output_flags_ & OUTPUT_COLOR) {
        color_begin = "\033[33m";
        color_end   = "\033[0m";
        color_level  = "\033[36m";
    }

    if (prefix_.size() > 0) {
        len = snprintf(ptr, buflen, "%s[%s ", color_begin, prefix_.c_str());
    } else {
        len = snprintf(ptr, buflen, "%s[", color_begin);
    }
    
    buflen -= len;
    ptr += len;
    
    if (output_flags_ & OUTPUT_TIME) {
        Time t;
        t.get_time();
        len = snprintf(ptr, buflen, "%u.%06u ", t.sec_, t.usec_);
        
        buflen -= len;
        ptr += len;
    }

    if (output_flags_ & OUTPUT_PATH)
    {
        if (output_flags_ & OUTPUT_SHORT) {
            len = snprintf(ptr, buflen, "%-19.19s ", path);
        } else {
            len = snprintf(ptr, buflen, "%s ", path);
        }
        buflen -= len;
        ptr += len;
    }
    
    if (output_flags_ & OUTPUT_CLASSNAME)
    {
        if (output_flags_ & OUTPUT_SHORT) {
            len = snprintf(ptr, buflen, "%-19.19s ",
                           classname ? classname : "(No_Class)");
        } else {
            len = snprintf(ptr, buflen, "%s ",
                           classname ? classname : "(No_Class)");
        }
        buflen -= len;
        ptr += len;
    }
    
    if ((output_flags_ & OUTPUT_OBJ) && (obj != NULL))
    {
        len = snprintf(ptr, buflen, "%p ", obj);
        buflen -= len;
        ptr += len;
    }

    if (output_flags_ & OUTPUT_LEVEL)
    {
        if (output_flags_ & OUTPUT_SHORT) {
            len = snprintf(ptr, buflen, "%s%c%s",
                           color_level,
                           toupper(level2str(level)[0]),
                           color_begin);
        } else {
            len = snprintf(ptr, buflen, "%s%s%s",
                           color_level,
                           level2str(level),
                           color_begin);
        }
        buflen -= len;
        ptr += len;
    }

    len = snprintf(ptr, buflen, "]%s ", color_end);
    buflen -= len;
    ptr    += len;
    
    return ptr - buf;
}

int
Log::vlogf(const char* path, log_level_t level,
           const char* classname, const void* obj,
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

    /* Make sure that paths that don't start with a slash still show up. */
    if (*path != '/') {
        snprintf(pathbuf, sizeof pathbuf, "/%s", path);
        path = pathbuf;
    }

    // bail if we're not going to output the line
    if (! log_enabled(level, path) &&
        (classname == NULL || ! log_enabled(level, classname)))
    {
        return 0;
    }
    
    size_t buflen = LOG_MAX_LINELEN - 1; /* Save a character for newline. */
    size_t len;

    // generate the log prefix
    len = gen_prefix(buf, buflen, path, level, classname, obj);
    buflen -= len;
    ptr += len;
    
    // generate the log string
    len = log_vsnprintf(ptr, buflen, fmt, ap);

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

#ifndef NDEBUG
    if (memcmp(&buf[LOG_MAX_LINELEN], guard, sizeof(guard)) != 0) {
        if (__debug_no_panic_on_overflow) {
            return -1;
        }
        
        PANIC("logf buffer overflow");
    }

#endif

#ifdef CHECK_NON_PRINTABLE
    for (u_int i = 0; i < buflen; ++i) {
        ASSERT(buf[i] == '\n' ||
               (buf[i] >= 32 && buf[i] <= 126));
    }
#endif

    int save_errno = errno;
    
    // do the write, making sure to drain the buffer. since stdout was
    // set to nonblocking, the spin lock prevents other threads from
    // jumping in here
    output_lock_->lock("Log::vlogf");
    int ret = IO::writeall(logfd_, buf, buflen);
    output_lock_->unlock();
    
    ASSERTF(ret == (int)buflen,
            "unexpected return from IO::writeall (got %d, expected %zu): %s",
            ret, buflen, strerror(errno));

    errno = save_errno;
    
    return buflen;
};

int
Log::log_multiline(const char* path, log_level_t level, 
                   const char* classname, const void* obj,
                   const char* msg)
{
    ASSERT(inited_);

    char pathbuf[LOG_MAX_PATHLEN];

    /* Make sure that paths that don't start with a slash still show up. */
    if (*path != '/') {
        snprintf(pathbuf, sizeof pathbuf, "/%s", path);
        path = pathbuf;
    }

    // bail if we're not going to output the line.
    if (! log_enabled(level, path))
        return 0;

    // generate the log prefix
    char prefix[LOG_MAX_LINELEN];
    size_t prefix_len = gen_prefix(prefix, sizeof(prefix),
                                   path, level, classname, obj);

    size_t iov_total = 128; // 64 lines of output
    struct iovec  static_iov[iov_total];
    struct iovec* dynamic_iov = NULL;
    struct iovec* iov = static_iov;
    size_t iov_cnt = 0;
    size_t total_len = 0;
    
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
        total_len += prefix_len;
        
        iov[iov_cnt].iov_base = (char*)msg;
        iov[iov_cnt].iov_len  = end - msg + 1; // include newline
        ++iov_cnt;
        total_len += end - msg + 1;

        msg = end + 1;

        if (iov_cnt == iov_total) {
            dynamic_iov = (iovec*)realloc(dynamic_iov,
                                          sizeof(iovec) * iov_total * 2);
            if (iov == static_iov) {
                memcpy(dynamic_iov, static_iov, sizeof(iovec) * iov_total);
            }

            iov = dynamic_iov;
            iov_total = iov_total * 2;
        }
    }
    
    int save_errno = errno;
    
    // do the write, making sure to drain the buffer. since stdout was
    // set to nonblocking, the spin lock prevents other threads from
    // jumping in here
    output_lock_->lock("Log::log_multiline");
    int ret = IO::writevall(logfd_, iov, iov_cnt);
    output_lock_->unlock();
    
    ASSERTF(ret == (int)total_len,
            "unexpected return from IO::writevall (got %d, expected %zu): %s",
            ret, total_len, strerror(errno));

    errno = save_errno;
    
    return ret;
}

} // namespace oasys
