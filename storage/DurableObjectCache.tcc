
#ifndef __OASYS_DURABLE_STORE_INTERNAL_HEADER__
#error DurableObjectCache.h must only be included from within DurableStore.h
#endif

/**
 * Constructor.
 */
template <typename _DataType>
DurableObjectCache<_DataType>::DurableObjectCache(size_t capacity)
    : Logger("/storage/cache"),
      size_(0), capacity_(capacity),
      hits_(0), misses_(0), evictions_(0), lock_(new SpinLock())
{
    log_debug("init capacity=%u", (u_int)capacity);
}

/**
 * Build a std::string to index the hash map.
 */
template <typename _DataType>
void
DurableObjectCache<_DataType>::get_cache_key(std::string* cache_key,
                                             const SerializableObject& key)
{
    StringSerialize serialize(Serialize::CONTEXT_LOCAL);
    if (serialize.action(&key) != 0) {
        PANIC("error sizing key");
    }

    cache_key->assign(serialize.buf().data(), serialize.buf().length());
}
    
/**
 * Kick the least recently used element out of the cache.
 */
template <typename _DataType>
void
DurableObjectCache<_DataType>::evict_last()
{
    typename CacheTable::iterator cache_iter;
    CacheLRUList::iterator lru_iter;

    ASSERT(lock_->is_locked_by_me());

    ASSERT(!lru_.empty());
    
    lru_iter = lru_.begin();
    ASSERT(lru_iter != lru_.end());

    cache_iter = cache_.find(*lru_iter);
    ASSERT(cache_iter != cache_.end());

    CacheElement* cache_elem = cache_iter->second;
    ASSERT(cache_elem->object_ != NULL);
    ASSERT(cache_elem->lru_iter_ == lru_iter);
    ASSERT(!cache_elem->live_);

    log_debug("cache at or near capacity (%u/%u) -- "
              "evicting key '%s' object %p size %u",
              size_, capacity_, lru_iter->c_str(),
              cache_elem->object_, (u_int)cache_elem->object_size_);
    
    cache_.erase(cache_iter);
    lru_.pop_front();

    size_ -= cache_elem->object_size_;
    
    evictions_++;
    
    delete cache_elem->object_;
    delete cache_elem;
}

/**
 * Add a new object to the cache, backed by the given table. Note
 * that this may cause some other object to be evicted from the
 * cache, which may result in a flush() call to another table.
 */
template <typename _DataType>
int
DurableObjectCache<_DataType>::add(const SerializableObject& key,
                                   const _DataType* object)
{
    ScopeLock l(lock_);
    
    CacheElement* cache_elem;
    
    typename CacheTable::iterator cache_iter;
    CacheLRUList::iterator lru_iter;
    
    std::string cache_key;
    get_cache_key(&cache_key, key);

    // first make sure the object doesn't exist in the cache
    cache_iter = cache_.find(cache_key);

    if (cache_iter != cache_.end()) {
        cache_elem = cache_iter->second;
        
        if (cache_elem->object_ == object) {
            log_err("add(%s): object already exists!!", cache_key.c_str());
        } else {
            PANIC("add(%s): multiple objects %p %p for same key",
                  cache_key.c_str(), object, cache_elem->object_);
        }
        return DS_EXISTS;
    }

    // figure out the size of the new object
    MarshalSize sizer(Serialize::CONTEXT_LOCAL);
    if (sizer.action(object) != 0) {
        PANIC("error in MarshalSize");
    }
    size_t object_size = sizer.size();

    log_debug("add(%s): object %p size %u",
              cache_key.c_str(), object, (u_int)object_size);

    // now try to evict elements if the new object will put us over
    // the cache capacity
    while ((size_ + object_size) > capacity_) {
        if (lru_.empty()) {
            log_warn("cache already at capacity (size %u, capacity %u) "
                     "but all %d elements are live",
                     (u_int)size_, (u_int)capacity_, (u_int)cache_.size());
            break;
        }

        evict_last();
    }

    // now cons up an element linking to the object and put it in the
    // cache, but not the LRU list since the object is assumed to be
    // live
    cache_elem = new CacheElement(object, object_size, true, lru_iter);
    typename CacheTable::value_type val(cache_key, cache_elem);
    CacheInsertRet ret = cache_.insert(val);

    ASSERT(ret.second == true);
    ASSERT(cache_.find(cache_key) != cache_.end());

    size_ += object_size;

    return DS_OK;
}

/**
 * Look up a given object in the cache.
 */
template <typename _DataType>
int
DurableObjectCache<_DataType>::get(const SerializableObject& key,
                                   _DataType** objectp)
{
    ScopeLock l(lock_);

    std::string cache_key;
    get_cache_key(&cache_key, key);

    typename CacheTable::iterator cache_iter = cache_.find(cache_key);
    
    if (cache_iter == cache_.end()) {
        log_debug("get(%s): no match", cache_key.c_str());
        ++misses_;
        return DS_NOTFOUND;
    } 

    ++hits_;

    CacheElement* cache_elem = cache_iter->second;

    if (! cache_elem->live_) {
        cache_elem->live_ = true;
        CacheLRUList::iterator lru_iter = cache_elem->lru_iter_;
        ASSERT(lru_iter != lru_.end());
        ASSERT(*lru_iter == cache_key);
        lru_.erase(lru_iter);
    }
    
    *objectp = const_cast<_DataType*>(cache_elem->object_);
    log_debug("get(%s): match %p", cache_key.c_str(), *objectp);

    return DS_OK;
}

template <typename _DataType>
int
DurableObjectCache<_DataType>::release(const SerializableObject& key,
                                       _DataType* data)
{
    ScopeLock l(lock_);

    std::string cache_key;
    get_cache_key(&cache_key, key);
    
    typename CacheTable::iterator cache_iter = cache_.find(cache_key);

    if (cache_iter == cache_.end()) {
        log_err("release(%s): no match for object %p",
                cache_key.c_str(), data);
        return DS_ERR;
    }

    CacheElement* cache_elem = cache_iter->second;
    ASSERT(cache_elem->object_ == data);

    if (! cache_elem->live_) {
        log_err("release(%s): release object %p already on LRU list!!",
                cache_key.c_str(), data);
        lru_.move_to_back(cache_elem->lru_iter_);
    } else {
        log_debug("release(%s): release object %p", cache_key.c_str(), data);
        lru_.push_back(cache_key);
        cache_elem->live_ = false;
        cache_elem->lru_iter_ = --lru_.end();
        ASSERT(*cache_elem->lru_iter_ == cache_key);
    }

    if (size_ > capacity_) {
        log_debug("release while over capacity, evicting stale object");
        ASSERT(lru_.size() == 1);
        evict_last();
    }

    return DS_OK;
}

/**
 * Forcibly remove an object from the cache.
 */
template <typename _DataType>
int
DurableObjectCache<_DataType>::del(const SerializableObject& key, _DataType* data)
{
    ScopeLock l(lock_);

    std::string cache_key;
    get_cache_key(&cache_key, key);
    
    typename CacheTable::iterator cache_iter = cache_.find(cache_key);

    if (cache_iter == cache_.end()) {
        log_warn("del(%s): no match for object %p", cache_key.c_str(), data);
        return DS_NOTFOUND;
    }
    
    CacheElement* cache_elem = cache_iter->second;
    ASSERT(cache_elem->object_ == data);
    
    if (! cache_elem->live_) {
        log_err("del(%s): object %p on LRU list!!", cache_key.c_str(), data);
        lru_.erase(cache_elem->lru_iter_);
    }

    log_debug("del(%s): removing object %p (size %u) from cache",
              cache_key.c_str(), data, cache_elem->object_size_);
    
    cache_.erase(cache_iter);
    size_ -= cache_elem->object_size_;

    delete cache_elem;
    return DS_OK;
}
