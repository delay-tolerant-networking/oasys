#include <errno.h>
#include <unistd.h>
#include <sys/poll.h>
#include "Notifier.h"
#include "SpinLock.h"
#include "io/IO.h"

Notifier::Notifier()
{
    logpathf("/notifier/%p", this);
    
    if (pipe(pipe_) != 0) {
        log_crit("can't create pipe for notifier");
        exit(1);
    }

    log_debug("created pipe, fds: %d %d", pipe_[0], pipe_[1]);
    
    for (int n = 0; n < 2; ++n) {
        if (IO::set_nonblocking(pipe_[n], true) != 0) {
            log_crit("error setting fd %d to nonblocking: %s",
                     pipe_[n], strerror(errno));
            exit(1);
        }
    }
}

Notifier::~Notifier()
{
    close(pipe_[0]);
    close(pipe_[1]);
}

void
Notifier::drain_pipe()
{
    int ret;
    char buf[256];

    while (1) {
        ret = IO::read(read_fd(), buf, sizeof(buf), logpath_);
        if (ret <= 0) {
            if ((ret == -1) && (errno == EAGAIN)) { // all done
                log_debug("read from pipe would have blocked");
                return;
            } else {
                log_crit("unexpected error return from read: %s",
                         strerror(errno));
                exit(1);
            }
        }
        
        log_debug("drained %d byte(s) from pipe", ret);
        
        /*
         * In the (likely) case that we didn't get a full buf's worth
         * of data, there's nothing else in the pipe and we can
         * return. Otherwise, loop again and call read until the pipe
         * is empty.
         */
        if (ret < (int)sizeof(buf))
            return;
    }
}

void
Notifier::wait()
{
    int ret = IO::poll(read_fd(), POLLIN, -1, logpath_);
    if (ret <= 0) {
        log_crit("unexpected error return from poll: %s", strerror(errno));
        exit(1);
    }

    ASSERT(ret == 1);
}

void
Notifier::notify()
{
    char b = 0;
    int ret = ::write(write_fd(), &b, 1);

    if (ret == -1) {
        if (errno == EAGAIN) {
            // if the pipe is full, that probably means the
            // consumer is just slow, so we log it, but keep
            // going. really, this shouldn't happen though
            log_warn("pipe appears to be full");
        } else {
            log_err("unexpected error writing to pipe: %s", strerror(errno));
        }
    } else if (ret == 0) {
        log_err("unexpected eof writing to pipe");
    } else {
        ASSERT(ret == 1);
    }
}
