/*
 *    Copyright 2006 Intel Corporation
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

#ifndef __CACHE_H__
#define __CACHE_H__

#include <map>

#include "../debug/InlineFormatter.h"
#include "../debug/Logger.h"
#include "../thread/Atomic.h"
#include "../thread/SpinLock.h"
#include "../util/LRUList.h"


namespace oasys {

class CacheHelper {
};

/*!
 * Generic cache implementation.
 *
 * CacheHelper has the following signature:
 *
 * bool over_limit(const _Key& key, const _Val& value)
 * 
 * Return true if the cache limits have been exceeded when adding this
 * new value to the cache.
 *
 * void put(const _Key& key, const _Val& value)
 *
 * Update statistics on the cache for over_limit computation.
 *
 * void cleanup(const _Key& key, const _Val& value)
 *
 * Element has been removed from the cache, cleanup the object and
 * update stored statistics.
 *
 */ 
template<typename _Key, typename _Val, typename _CacheHelper>
class Cache : Logger {
public:
    /*!
     * LRU list entry.
     */
    struct LRUListEnt {
        LRUListEnt(const _Key& key,
                   const _Val& val,
                   int pin_count = 0)
            : key_(key), val_(val), pin_count_(pin_count)
        {}

        _Key     key_;
        _Val     val_;
	int      pin_count_;
        SpinLock lock_;
    };

    typedef LRUList<LRUListEnt> CacheList;
    typedef std::map<_Key, typename CacheList::iterator> CacheMap;

    /*!
     * Handle to an element to an entry in the cache. Valid as long as
     * the pin count is > 0.
     *
     * XXX/bowei -- this is kind of bogus for multithreading
     * performance...FIXME later.
     */
    class Handle {
        friend class Cache;

    public:
        Handle() : cache_(0) {}
        
        // Copyable and assignable

        int pin() {
            return cache_->pin(*this);
        }

        int unpin() {
            int count = cache_->unpin(*this);

            // Pin count == 0, invalid this handle.
            if (count == 0)
            {
                cache_ = 0;
            }
            return count;
        }
        
        bool valid() { return cache_ != 0; }
        
    private:
        Cache* cache_;
        typename CacheList::iterator itr_;

        Handle(Cache* cache, typename CacheList::iterator itr)
            : cache_(cache), itr_(itr) {}
    };

    /*!
     * Helper class for external refs which manage how many references
     * are on an object.
     */
    class ExternalHandle {
    public:
	ExternalHandle(Handle cache_handle)
	    : cache_handle_(cache_handle) 
	{
	    cache_handle_.pin();
	}

	ExternalHandle(const ExternalHandle& other) 
	    : cache_handle_(other.cache_handle_)
	{
	    cache_handle_.pin();
	}

	ExternalHandle& operator=(const ExternalHandle& other)
	{
	    cache_handle_ = other.cache_handle_;
	    cache_handle_.pin();

	    return *this;
	}

	~ExternalHandle() { cache_handle_.unpin(); }

    private:
	Handle cache_handle_;
    };
    
    /*!
     * Constructor.
     */
    Cache(const char*        logpath, 
          const _CacheHelper helper)
        : Logger("Cache", logpath),
          helper_(helper) 
    {}
    
    /*!
     * Get an item from the cache and optionally pin it.
     *
     * @param handle Returns a handle to the Cache element on the
     * list. This handle is useful for a call to cache functions pin()
     * and unpin() because they do not incur an additional search
     * through the std::map for the element.
     *
     * @return whether or not the value is in the cache, and assign
     * *valp to the value.
     */
    bool get(const _Key& key, _Val* valp = 0, bool pin = false,
             Handle* handle = 0)
    {
        ScopeLock l(&lock_, "Cache::get_and_pin");
        
        typename CacheMap::iterator i = cache_map_.find(key);
        if (i == cache_map_.end()) 
        {
            log_debug("get(%s): not in cache",
                      InlineFormatter<_Key>().format(key));
            return false;
        }
        
        cache_list_.move_to_back(i->second);
	ScopeLock ll(&i->second->lock_, "Cache::get_and_pin");
        if (pin) 
        {
	    ++i->second->pin_count_;
        }
        
        log_debug("get(%s): got entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(key), 
                  i->second->pin_count_,
                  cache_map_.size());
        
        if (valp != 0)
        {
            *valp = i->second->val_;
        }

        if (handle != 0) 
        {
            *handle = Handle(this, i->second);
        }
        
        return true;
    }

    /*!
     * Syntactic sugar.
     */
    bool get_and_pin(const _Key& key, _Val* valp = 0,
                     Handle* handle = 0)
    {
        return get(key, valp, true, handle);
    }

    /*!
     * Pin the val referenced by _Key.
     *
     * @return New pin count after this call.
     */
    u_int32_t pin(const _Key& key) 
    {
        ScopeLock l(&lock_, "Cache::pin");

        typename CacheMap::iterator i = cache_map_.find(key);
        ASSERT(i != cache_map_.end());
        
	ScopeLock ll(&i->second->lock_, "Cache::pin");
	++i->second->pin_count_;
        log_debug("pin(%s): pinned entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(key),
                  i->second->pin_count_,
                  cache_map_.size());

        return count;
    }

    /*!
     * Pin based on the iterator Handle.
     *
     * @return New pin count after this call.
     */
    int pin(Handle handle)
    {
	ScopeLock l(&handle.itr_->lock_, "Cache::pin");
        int count = ++handle.itr_->pin_count_;
        log_debug("pin(%s): pinned entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(handle.itr_->key_),
                  count,
                  cache_map_.size());

        return count;
    }

    /*!
     * Unpin the val referenced by _Key.
     *
     * @return New pin count after this call.
     */
    int unpin(const _Key& key) 
    {
        ScopeLock l(&lock_, "Cache::unpin");

        typename CacheMap::iterator i = cache_map_.find(key);
        ASSERT(i != cache_map_.end());

	ScopeLock ll(&i->second->lock_, "Cache::pin");
        ASSERT(i->second->pin_count_ > 0);

        int count = --i->second->pin_count_;
        log_debug("unpin(%s): unpinned entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(i->second->key_),
                  i->second->pin_count_,
                  cache_map_.size());
        return count;
    }

    /*!
     * Unpin based on the iterator Handle.
     *
     * @return New pin count after this call.
     */
    int unpin(Handle handle)
    {
        ScopeLock l(&handle.itr_->lock_, "Cache::unpin");

        int count = --handle.itr_->pin_count_;
        log_debug("unpin(%s): unpinned entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(handle.itr_->key_),
                  handle.itr_->pin_count_,
                  cache_map_.size());
        return count;
    }
    
    /*!
     * Put an val in the file cache which may evict unpinned vals.
     * Also pin the val that was just put into the cache.
     * 
     * @return true if the item successfully was put in the cache,
     * false if there was a collision on the key.
     */
    bool put_and_pin(const _Key& key, const _Val& val,
                     Handle* handle = 0) 
    {
        ScopeLock l(&lock_, "Cache::put_and_pin");

        typename CacheMap::iterator i = cache_map_.find(key);
        if (i != cache_map_.end()) {
            log_debug("put_and_pin(%s): key already in cache",
                      InlineFormatter<_Key>().format(key));
            return false;
        }

        while (helper_.over_limit(key, val))
        {
            if (cache_map_.size() == 0)
            {
                PANIC("Putting object into cache of size greater than "
                      "entire cache limits!");
            }
            
            if (! evict_last()) 
            {
                break;
            }
        }
        
        // start off with pin count 1
        typename CacheList::iterator new_ent =
            cache_list_.insert(cache_list_.end(),
                               LRUListEnt(key, val, 1));

        if (handle) 
        {
            *handle = Handle(this, new_ent);
        }

        log_debug("put_and_pin(%s): added entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(key),
                  new_ent->pin_count_,
                  cache_map_.size());

        cache_map_[key] = new_ent;
        helper_.put(key, val);

        return true;
    }

    /*!
     * Forcibly evict key.
     */
    void evict(const _Key& key) 
    {
        ScopeLock l(&lock_, "Cache::evict");
        
        typename CacheMap::iterator i = cache_map_.find(key);

        if (i == cache_map_.end()) 
        {
            return;
        }

        if (i->second->pin_count_ > 0)
        {
            log_warn("evict(%s): entry still busy, count = %d",
                     InlineFormatter<_Key>().format(key),
                     i->second->pin_count_);
        }

        ASSERT(key == i->second->key_);
        helper_.cleanup(key, i->second->val_);
        log_debug("evict(%s): evicted entry size=%zu",
                  InlineFormatter<_Key>().format(key),
                  cache_map_.size());
        
        cache_list_.erase(i->second);
        cache_map_.erase(i);       
    }
    
    /*!
     * Evict all of the cached vals.
     */
    void evict_all() 
    {
        ScopeLock l(&lock_, "Cache::evict_all");
        
        log_debug("evict_all(): %zu open vals", cache_list_.size());
        
        for (typename CacheList::iterator i = cache_list_.begin(); 
             i != cache_list_.end(); ++i)
        {
            if (i->pin_count_ > 0) {
                log_warn("evict_all(): evicting %s with pin count %d",
                         InlineFormatter<_Key>().format(i->key_),
                         i->pin_count_);
            } else {
                log_debug("evict_all(): evicting %s",
                          InlineFormatter<_Key>().format(i->key_));
            }
            helper_.cleanup(i->key_, i->val_);
        }

        cache_list_.clear();
        cache_map_.clear();
    }

    /*!
     * @return Helper object.
     */
    _CacheHelper* get_helper() { return &helper_; }

    /*!
     * Helper class to unpin an element at the end of a scope.
     */
    class ScopedUnpin {
    public:
        ScopedUnpin(Cache* cache, const _Key& key)
            : cache_(cache), key_(key) {}

        ~ScopedUnpin()
        {
            cache_->unpin(key_);
        }

    private:
        Cache* cache_;
        _Key   key_;
    };

protected:
    SpinLock lock_;

private:
    CacheList    cache_list_;
    CacheMap     cache_map_;
    _CacheHelper helper_;

    /*!
     * Search from the beginning of the list and throw out a single,
     * unpinned val.
     *
     * @return whether or not evict succeeded
     */
    bool evict_last()
    {
        bool found = false;
        typename CacheList::iterator i;
        for (i = cache_list_.begin(); i != cache_list_.end(); ++i)
        {
            if (i->pin_count_ == 0) {
                found = true;
                break;
            }
        }
        
        if (found) 
        {
            log_debug("evict_last(): evicting %s size=%zu",
                      InlineFormatter<_Key>().format(i->key_),
                      cache_map_.size());
            helper_.cleanup(i->key_, i->val_);
            cache_map_.erase(i->key_);
            cache_list_.erase(i);
        }
        else
        {
            log_warn("evict_last(): all entries are busy! size=%zu",
                     cache_map_.size());
            return false;
        }
        
        return true;
    }
};

} // namespace oasys

#endif /* __CACHE_H__ */
