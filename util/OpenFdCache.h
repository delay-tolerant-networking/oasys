#ifndef __OPENFDCACHE_H__
#define __OPENFDCACHE_H__

#include <map>

#include "../debug/Logger.h"
#include "../thread/SpinLock.h"
#include "../io/IO.h"
#include "../util/LRUList.h"

namespace oasys {

struct OpenFdCacheClose {
    static void close(int fd) {
//        log_debug("/OpenFdCacheClose", "closed %d", fd);
        IO::close(fd);
    }
};

/*!
 * Maintains a cache of open files to get rid of calls to syscall
 * open().
 */ 
template<typename _Key, typename _CloseFcn = OpenFdCacheClose>
class OpenFdCache : Logger {
public:
    struct FdListEnt {
        FdListEnt(const _Key& key,
                  int fd        = -1,
                  int pin_count = 0)
            : key_(key), fd_(fd), pin_count_(pin_count)
        {}
        
        _Key key_;
        int  fd_;
        int  pin_count_;
    };

    typedef LRUList<FdListEnt> FdList;
    typedef std::map<_Key, typename FdList::iterator> FdMap;

    OpenFdCache(const char* logpath, size_t max)
        : Logger("OpenFdCache", "%s/%s", logpath, "cache"),
          max_(max)
    {}

    /*!
     * @return -1 if the fd is not in the cache, otherwise the fd of
     * the open file. Also pin the fd so it can't be closed.
     */
    int get_and_pin(const _Key& key) 
    {
        ScopeLock l(&lock_, "OpenFdCache::get_and_pin");

        typename FdMap::iterator i = open_fds_map_.find(key);
        if (i == open_fds_map_.end()) {
            return -1;
        }
        
        open_fds_.move_to_back(i->second);
        ++(i->second->pin_count_);

        log_debug("Got entry fd=%d pin_count=%d", 
                  i->second->fd_, 
                  i->second->pin_count_);

        return i->second->fd_;
    }

    /*!
     * Unpin the fd referenced by _Key.
     */
    void unpin(const _Key& key) 
    {
        ScopeLock l(&lock_, "OpenFdCache::unpin");

        typename FdMap::iterator i = open_fds_map_.find(key);
        ASSERT(i != open_fds_map_.end());
        
        --(i->second->pin_count_);

        log_debug("Unpin entry fd=%d pin_count=%d", 
                  i->second->fd_, i->second->pin_count_);
    }

    /*!
     * Put an fd in the file cache which may evict unpinned fds. Also
     * pin the fd that was just put into the cache.
     */
    void put_and_pin(const _Key& key, int fd) 
    {
        ScopeLock l(&lock_, "OpenFdCache::put_and_pin");

        typename FdMap::iterator i = open_fds_map_.find(key);
        if (i != open_fds_map_.end()) {
            ++(i->second->pin_count_);
            return;
        }

        while (open_fds_map_.size() + 1> max_) {
            if (evict() == -1)
                break;
        }
        
        // start off with pin count 1
        typename FdList::iterator new_ent = open_fds_.insert(open_fds_.end(),
                                                             FdListEnt(key, fd, 1));
        log_debug("Added entry fd=%d pin_count=%d", 
                  new_ent->fd_, 
                  new_ent->pin_count_);

        open_fds_map_[key] = new_ent;
    }

    /*!
     * Close a file fd and remove it from the cache.
     */
    void close(const _Key& key) 
    {
        typename FdMap::iterator i = open_fds_map_.find(key);

        if (i == open_fds_map_.end())
            return;

        _CloseFcn::close(i->second->fd_);
        log_debug("Closed %d", i->second->fd_);

        open_fds_.erase(i->second);
        open_fds_map_.erase(i);       
    }
    
    /*!
     * Close and release all of the cached fds.
     */
    void close_all() {
        log_debug("There were %u open fds upon close.", open_fds_.size());
        
        for (typename FdList::iterator i = open_fds_.begin(); 
             i != open_fds_.end(); ++i)
        {
            if (i->pin_count_ > 0) {
                log_warn("fd=%d was busy", i->fd_);
            }

            log_debug("Closing fd=%d", i->fd_);
            _CloseFcn::close(i->fd_);
        }

        open_fds_.clear();
        open_fds_map_.clear();
    }

private:
    SpinLock lock_;

    FdList open_fds_;
    FdMap  open_fds_map_;

    size_t max_;

    /*!
     * Search from the beginning of the list and throw out a single,
     * unpinned fd.
     *
     * @return 0 if evict succeed or -1 we are totally pinned and
     * can't do anything.
     */
    int evict()
    {
        bool found = false;
        typename FdList::iterator i;
        for (i = open_fds_.begin(); i != open_fds_.end(); ++i)
        {
            if (i->pin_count_ == 0) {
                found = true;
                break;
            }
        }
        
        if (found) 
        {
            log_debug("Evicting fd=%d", i->fd_);
            _CloseFcn::close(i->fd_);
            open_fds_map_.erase(i->key_);
            open_fds_.erase(i);
        }
        else
        {
            log_warn("All of the fds are busy!");
            return -1;
        }

        return 0;
    }
};

} // namespace oasys

#endif /* __OPENFDCACHE_H__ */
