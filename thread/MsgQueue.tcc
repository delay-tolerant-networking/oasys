/*!
 * \file
 *
 * NOTE: This file is included by MsgQueue.h and should _not_ be
 * included in the regular Makefile build because of template
 * instantiation issues. As of this time, g++ does not have a good,
 * intelligent way of managing template instantiations. The other
 * route to go is to use -fno-implicit-templates and manually
 * instantiate template types. That however would require changing a
 * lot of the existing code, so we will just bear the price of
 * redundant instantiations for now.
 */

template<typename _elt_t>
MsgQueue<_elt_t>::MsgQueue(const char* logpath, SpinLock* lock)
    : Notifier(logpath), disable_notify_(false)
{
    logpath_appendf("/msgqueue");
    
    if (lock != NULL) {
        lock_ = lock;
    } else {
        lock_ = new SpinLock();
    }
}

template<typename _elt_t>
MsgQueue<_elt_t>::~MsgQueue()
{
    if (size() != 0)
    {
        log_err("not empty at time of destruction, size=%zu",
                queue_.size());
    }
    
    delete lock_;
    lock_ = 0;
}

template<typename _elt_t> 
void MsgQueue<_elt_t>::push(_elt_t msg, bool at_back)
{
    ScopeLock l(lock_, "MsgQueue::push");

    /*
     * It's important that we first call notify() and then put the
     * message on the queue. This is because notify might need to
     * release the lock and sleep in the rare case where the pipe is
     * full. Therefore, we wait until the notification is safely in
     * the pipe before putting the message on the queue, with the lock
     * held for both calls to ensure atomicity.
     */
    if (!disable_notify_) {
        notify(lock_);
    }
    
    if (at_back)
        queue_.push_back(msg);
    else
        queue_.push_front(msg);
}

template<typename _elt_t> 
_elt_t MsgQueue<_elt_t>::pop_blocking()
{
    ScopeLock l(lock_, "MsgQueue::pop_blocking");

    /*
     * If the queue is empty, wait for new input.
     */
    bool used_wait = false;

    if (queue_.empty()) {
        if (disable_notify_) {
            disable_notify_ = false;
        }
        
        wait(lock_);
        ASSERT(lock_->is_locked_by_me());
        used_wait = true;
    }

    /*
     * Can't be empty now.
     */
    ASSERT(!queue_.empty());

    if (!used_wait && !disable_notify_) {
        drain_pipe(1);
    }
    
    _elt_t elt  = queue_.front();
    queue_.pop_front();
    
    return elt;
}

template<typename _elt_t> 
bool MsgQueue<_elt_t>::try_pop(_elt_t* eltp)
{
    ScopeLock lock(lock_, "MsgQueue::try_pop");

    // nothing in the queue, nothing we can do...
    if (queue_.size() == 0) {
        return false;
    }
    
    // but if there is something in the queue, then return it
    *eltp = queue_.front();
    queue_.pop_front();

    if (!disable_notify_) {
        drain_pipe(1);
    }
        
    return true;
}

template <typename _elt_t>
void MsgQueue<_elt_t>::disable_notify()
{
    ScopeLock lock(lock_, "MsgQueue::disable_notify");

    // the queue must be empty or assertions won't hold
    ASSERT(queue_.size() == 0);
    disable_notify_ = true;
}

