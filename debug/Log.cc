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
#  include <oasys-config.h>
#endif

#include <sstream>
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
bool Log::shutdown_ = false;
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
    ASSERT(!shutdown_);

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
    shutdown_ = true;
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
                const char* classname, const void* obj) const
{
    // this function can be called with a null buffer if buflen is 0.
    // In this case, this function does nothing except return the
    // number of characters (not including the null terminator) that
    // *would* be written if buflen was big enough.
    ASSERT(buf || (buflen == 0));

    // the length of the log entry prefix (not including the null
    // terminator), assuming buflen is big enough (the return value)
    size_t prefix_len = 0;

    size_t len;
    char *ptr = buf;

    const char* color_begin = "";
    const char* color_end   = "";
    const char* color_level = "";
    
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
    
    // advance the would-be size appropriately
    prefix_len += len;
    // advance ptr (and decrement buflen) only by the number of
    // characters that were written -- not by the number that *would*
    // have been written.  This keeps buflen from wrapping around and
    // prevents undefined behavior by incrementing ptr beyond one past
    // the length of the buffer.  You will see this pattern throughout
    // the remainder of this function.
    if (len > buflen) len = buflen;
    ptr += len;
    buflen -= len;
    
    if (output_flags_ & OUTPUT_TIME) {
        Time t;
        t.get_time();
        len = snprintf(ptr, buflen, "%u.%06u ", t.sec_, t.usec_);
        
        prefix_len += len;
        if (len > buflen) len = buflen;
        ptr += len;
        buflen -= len;
    }

    if (output_flags_ & OUTPUT_PATH)
    {
        if (output_flags_ & OUTPUT_SHORT) {
            len = snprintf(ptr, buflen, "%-19.19s ", path);
        } else {
            len = snprintf(ptr, buflen, "%s ", path);
        }
        
        prefix_len += len;
        if (len > buflen) len = buflen;
        ptr += len;
        buflen -= len;
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
        
        prefix_len += len;
        if (len > buflen) len = buflen;
        ptr += len;
        buflen -= len;
    }
    
    if ((output_flags_ & OUTPUT_OBJ) && (obj != NULL))
    {
        len = snprintf(ptr, buflen, "%p ", obj);
        
        prefix_len += len;
        if (len > buflen) len = buflen;
        ptr += len;
        buflen -= len;
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
        
        prefix_len += len;
        if (len > buflen) len = buflen;
        ptr += len;
        buflen -= len;
    }

    len = snprintf(ptr, buflen, "]%s ", color_end);
    
    prefix_len += len;
    if (len > buflen) len = buflen;
    ptr += len;
    buflen -= len;
    
    return prefix_len;
}


std::string
Log::gen_prefix(const std::string& path, log_level_t level,
                const char* classname, const void* obj) const
{
    std::vector<char> prefix_buf(LOG_MAX_LINELEN, '\0');
    size_t prefix_len;
    while ((prefix_len = this->gen_prefix(
                &prefix_buf[0], prefix_buf.size(), path.c_str(), level,
                classname, obj))
           >= prefix_buf.size())
    {
        prefix_buf.resize(prefix_len + 1, '\0');
    }
    ASSERT(prefix_buf.size() > prefix_len);
    return std::string(&prefix_buf[0]);
}


int
Log::log(const std::string& path, log_level_t level,
         const char* classname, const void* obj,
         const std::string& msg, bool prefixEachLine)
{
    ASSERT(inited_);
    ASSERT(!shutdown_);

    int rval = 0;

    // bail if we're not going to output the line
    if (!log_enabled(level, path.c_str()) &&
        (classname == NULL || !log_enabled(level, classname)))
    {
        return rval;
    }

    // generate the log entry prefix
    std::string prefix(this->gen_prefix(path, level, classname, obj));

    // dump the message to the log file
    if (prefixEachLine)
    {
        std::istringstream is(msg);
        std::string line;

        // lock the log file so that all lines appear next to each
        // other (the lock is reentrant, so this won't cause a
        // deadlock in dumpToFile())
        output_lock_->lock("Log::log");
        while (std::getline(is, line))
        {
            rval = this->dumpToFile(prefix + line + '\n');
        }
        output_lock_->unlock();
    }
    else
    {
        if (msg[msg.size() - 1] != '\n')
            rval = this->dumpToFile(prefix + msg + '\n');
        else
            rval = this->dumpToFile(prefix + msg);
    }

    return rval;
}


int
Log::dumpToFile(const std::string& data)
{
    return this->dumpToFile(data.data(), data.size());
}


int
Log::dumpToFile(const char* data, size_t size)
{
    if (!data || size == 0)
        return 0;

#ifdef CHECK_NON_PRINTABLE
    // make sure there are no special bytes in the log entry string
    for (size_t i = 0; i < size; ++i)
    {
        ASSERT((data[i] == '\n') ||
               ((data[i] >= 32) && (data[i] <= 126)));
    }
#endif

    const int save_errno = errno;
    
    // do the write, making sure to drain the buffer. since stdout was
    // set to nonblocking, the spin lock prevents other threads from
    // jumping in here
    output_lock_->lock("Log::dumpToFile");
    int ret = IO::writeall(logfd_, data, size);
    output_lock_->unlock();
    
    ASSERTF(ret == static_cast<int>(size),
            "unexpected return from IO::writeall (got %d, expected %zu): %s",
            ret, size, strerror(errno));

    errno = save_errno;
    
    return static_cast<int>(size);
}


int
Log::vlogf(const char* path, log_level_t level,
           const char* classname, const void* obj,
           const char* fmt, va_list ap)
{
    ASSERT(inited_);
    ASSERT(!shutdown_);

    if ((path == NULL) || (fmt == NULL))
        return -1;

    char pathbuf[LOG_MAX_PATHLEN];

    /* Make sure that paths that don't start with a slash still show up. */
    if (*path != '/') {
        snprintf(pathbuf, sizeof(pathbuf), "/%s", path);
        path = pathbuf;
    }

    // bail if we're not going to output the line
    if (! log_enabled(level, path) &&
        (classname == NULL || ! log_enabled(level, classname)))
    {
        return 0;
    }
    
    // try to catch crashes due to buffer overflow with some guard
    // bytes at the end
    static const char guard[] = "[guard]";

    // fixed buffer that should be big enough to handle most cases.
    // The extra byte is for a final newline (added if one is not
    // provided by the caller).
    ASSERT(LOG_MAX_LINELEN >= 0);
    char buf_fixed[LOG_MAX_LINELEN + 1 + sizeof(guard)];

    // buf is a pointer to a buffer that will contain the log entry
    char* buf = buf_fixed;

    // the buffer will be incrementally filled; ptr keeps track of
    // where we are in the buffer at the moment
    char* ptr = buf;

    // buflen is the amount of buffer space we have remaining.  it is
    // initialized to LOG_MAX_LINELEN and not LOG_MAX_LINELEN+1 (as in
    // the declaration of buf_fixed above) so that there is room for a
    // trailing newline in case the user didn't include one.
    size_t buflen = LOG_MAX_LINELEN;

    // guard_location is where the buffer overflow guard can be found
    char* guard_location = &buf[sizeof(buf_fixed) - sizeof(guard)];

    // set the guard
    memcpy(guard_location, guard, sizeof(guard));

    // generate the log prefix
    size_t prefix_len = gen_prefix(buf, buflen, path, level, classname, obj);

    if (prefix_len < buflen) {
        // set ptr to the location of the null terminator
        ptr += prefix_len;
        // decrement the amount of space we have left in buf
        buflen -= prefix_len;
    } else {
        // prefix was truncated because buf isn't big enough

        // prevent undefined behavior by not incrementing ptr beyond
        // one past the end of buf
        ptr += buflen;
        // keep buflen from wrapping around, which might result in a
        // buffer overflow in the call to vsnprintf below
        buflen = 0;
    }
    
    // generate the log string
    size_t data_len = log_vsnprintf(ptr, buflen, fmt, ap);

    if (data_len < buflen) {
        ptr += data_len;
        buflen -= data_len;
    } else {
            // not enough room in the buffer for the whole log entry, so
            // truncate the line

            // is there room to copy anything?
            if (LOG_MAX_LINELEN > 0) {
                // yes

                // string that lets the user know that the line was
                // truncated
                static const char* trunc = "... (truncated)";

                // how much of the string to copy
                size_t chars_to_copy = (LOG_MAX_LINELEN > sizeof(trunc)) ?
                    sizeof(trunc) : LOG_MAX_LINELEN;

                // copy the string
                memcpy(&buf[LOG_MAX_LINELEN - chars_to_copy], trunc,
                       chars_to_copy - 1);

                // make sure that ptr is pointing to the end of the
                // log entry buffer
                ptr = &buf[LOG_MAX_LINELEN - 1];

                // make sure that it's null terminated
                *ptr = '\0';

            } else {
                // no room to copy anything
                ptr = buf;
            }
    }

    // make sure there's a trailing newline
    if ((ptr > buf) && (ptr[-1] != '\n')) {
        // it is safe to add this trailing newline since an extra byte
        // was added to the size of buf specifically for this purpose.
        *ptr++ = '\n';
    }
    // make sure the string is null-terminated
    *ptr = '\0';

#ifndef NDEBUG
    // check for buffer overflow
    if (memcmp(guard_location, guard, sizeof(guard)) != 0) {
        if (__debug_no_panic_on_overflow) {
            return -1;
        }
        
        PANIC("logf buffer overflow");
    }
#endif

    return this->dumpToFile(buf, ptr - buf);
};

int
Log::log_multiline(const char* path, log_level_t level, 
                   const char* classname, const void* obj,
                   const char* msg)
{
    ASSERT(inited_);
    ASSERT(!shutdown_);

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
