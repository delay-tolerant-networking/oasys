/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2005 Intel Corporation. All rights reserved. 
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


#ifndef __OASYS_DURABLE_STORE_INTERNAL_HEADER__
#error DurableObjectCache.h must only be included from within DurableStore.h
#endif

template <typename _DataType>
class DurableObjectCache : public Logger {
public:
    /**
     * Constructor.
     */
    DurableObjectCache(size_t capacity);

    /**
     * Add a new object to the cache, initially putting it on the live
     * object list. Note that this may cause some other object(s) to
     * be evicted from the cache.
     */
    int put(const SerializableObject& key, const _DataType* data, int flags);
    
    /**
     * Look up a given object in the cache.
     */
    int get(const SerializableObject& key, _DataType** data);

    /**
     * Return whether or not the key is currently live in in the cache.
     */
    bool is_live(const SerializableObject& key);

    /**
     * Release the given object, making it eligible for eviction.
     */
    int release(const SerializableObject& key, const _DataType* data);
    
    /**
     * Forcibly remove an object from the cache.
     */
    int del(const SerializableObject& key);

    /// @{
    /// Accessors
    size_t size()   { return size_; }
    size_t count()  { return cache_.size(); }
    size_t live()   { return cache_.size() - lru_.size(); }
    int hits()      { return hits_; }
    int misses()    { return misses_; }
    int evictions() { return evictions_; }
    /// @}

    /**
     * Reset the cache statistics.
     */
    void reset_stats()
    {
        hits_ = 0;
        misses_ = 0;
        evictions_ = 0;
    }

protected:
    /**
     * Build a std::string to index the hash map.
     */
    void get_cache_key(std::string* cache_key, const SerializableObject& key);
    
    /**
     * Kick the least recently used element out of the cache.
     */
    void evict_last();

    /**
     * The LRU list just stores the key for the object in the main
     * cache table.
     */
    typedef LRUList<std::string> CacheLRUList;
    
    /**
     * Type for the cache table elements. 
     */
    struct CacheElement {
        CacheElement(const _DataType* object, size_t object_size,
                     bool live, CacheLRUList::iterator lru_iter)
            
            : object_(object),
              object_size_(object_size),
              live_(live),
              lru_iter_(lru_iter) {}

        const _DataType*       object_;
        size_t                 object_size_;
        bool                   live_;
        CacheLRUList::iterator lru_iter_;
    };

    /**
     * The cache table.
     */
    class CacheTable : public StringHashMap<CacheElement*> {};
    typedef std::pair<typename CacheTable::iterator, bool> CacheInsertRet;

    size_t size_;	///< The current size of the cache
    size_t capacity_;	///< The maximum size of the cache
    int hits_;		///< Number of times the cache hits
    int misses_;	///< Number of times the cache misses
    int evictions_;	///< Number of times the cache evicted an object
    CacheLRUList lru_;	///< The LRU List of objects
    CacheTable cache_;	///< The object cache table
    SpinLock* lock_;	///< Lock to protect the in-memory cache
};
