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
#include "../thread/SpinLock.h"
#include "../util/LRUList.h"


namespace oasys {

/*!
 * Generic cache implementation. To simplify the contract, 
 *
 * The _EvictFcn is required to implement the signature:
 *
 *    void (*)(_Key& k, _Val& v);
 */ 
template<typename _Key, typename _Val, typename _EvictFcn>
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

        _Key key_;
        _Val val_;
        int  pin_count_;
    };

    typedef LRUList<LRUListEnt> CacheList;
    typedef std::map<_Key, typename CacheList::iterator> CacheMap;

    /*!
     * Constructor that takes the cache limit.
     */
    Cache(const char* logpath, size_t max)
        : Logger("Cache", logpath), max_(max) {}

    /*!
     * Get an item from the cache and optionally pin it.
     *
     * @return whether or not the value is in the cache, and assign
     * *valp to the value.
     */
    bool get(const _Key& key, _Val* valp, bool pin = false,
             typename CacheList::iterator* iter = NULL)
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
        if (pin) {
            ++(i->second->pin_count_);
        }
        
        log_debug("get(%s): got entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(key), 
                  i->second->pin_count_,
                  cache_map_.size());

        *valp = i->second->val_;

        if (iter != NULL) {
            *iter = i->second;
        }
        
        return true;
    }

    /*!
     * Syntactic sugar.
     */
    bool get_and_pin(const _Key& key, _Val* valp)
    {
        return get(key, valp, true);
    }

    /*!
     * Unpin the val referenced by _Key.
     */
    void unpin(const _Key& key) 
    {
        ScopeLock l(&lock_, "Cache::unpin");

        typename CacheMap::iterator i = cache_map_.find(key);
        ASSERT(i != cache_map_.end());
        
        --(i->second->pin_count_);

        log_debug("unpin(%s): unpinned entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(key),
                  i->second->pin_count_,
                  cache_map_.size());
    }

    /*!
     * Put an val in the file cache which may evict unpinned vals.
     * Also pin the val that was just put into the cache.
     * 
     * @return true if the item successfully was put in the cache,
     * false if there was a collision on the key.
     */
    bool put_and_pin(const _Key& key, const _Val& val,
                     typename CacheList::iterator* iter = NULL) 
    {
        ScopeLock l(&lock_, "Cache::put_and_pin");

        typename CacheMap::iterator i = cache_map_.find(key);
        if (i != cache_map_.end()) {
            log_debug("put_and_pin(%s): key already in cache",
                      InlineFormatter<_Key>().format(key));
            return false;
        }

        while (cache_map_.size() + 1 > max_) 
        {
            if (! evict_last()) {
                break;
            }
        }
        
        // start off with pin count 1
        typename CacheList::iterator new_ent =
            cache_list_.insert(cache_list_.end(),
                               LRUListEnt(key, val, 1));

        if (iter) {
            *iter = new_ent;
        }

        log_debug("put_and_pin(%s): added entry pin_count=%d size=%zu",
                  InlineFormatter<_Key>().format(key),
                  new_ent->pin_count_,
                  cache_map_.size());

        cache_map_[key] = new_ent;

        return val;
    }

    /*!
     * Evict the given key, calling the _EvictFcn on it.
     */
    void evict(const _Key& key) 
    {
        ScopeLock l(&lock_, "Cache::evict");
        
        typename CacheMap::iterator i = cache_map_.find(key);

        if (i == cache_map_.end()) 
        {
            return;
        }

        ASSERT(key == i->second->key_);
        _EvictFcn e;
        e(key, i->second->val_);
        log_debug("evict(%s): evicted entry size=%zu",
                  InlineFormatter<_Key>().format(key),
                  cache_map_.size());
        
        cache_list_.erase(i->second);
        cache_map_.erase(i);       
    }
    
    /*!
     * Evict all of the cached vals.
     */
    void evict_all() {
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

            _EvictFcn e;
            e(i->key_, i->val_);
        }

        cache_list_.clear();
        cache_map_.clear();
    }

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


private:
    SpinLock lock_;

    CacheList cache_list_;
    CacheMap  cache_map_;

    size_t max_;

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
            _EvictFcn e;
            e(i->key_, i->val_);
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