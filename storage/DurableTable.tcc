
/**
 * Get the data for key from a single type table, possibly creating a
 * new object of the given template type _DataType.
 *
 * Note that the specified type must match the actual type that was
 * stored in the database, or this will return undefined garbage.
 *
 * @param key  Key object
 * @param data Data object
 *
 * @return DS_OK, DS_NOTFOUND if key is not found
 */
template <typename _DataType>
inline int
SingleTypeDurableTable<_DataType>::get(const SerializableObject& key,
                                       _DataType** data)
{
    int err;

    if (this->cache_) {
        err = this->cache_->get(key, data);
        if (err == DS_OK) {
            ASSERT(*data != NULL);
            return DS_OK;
        }
    }
    
    _DataType* d = new _DataType(Builder());

    err = this->impl_->get(key, d);
    if (err != 0) {
        delete d;
        return err;
    }

    if (this->cache_) {
        err = this->cache_->put(key, d, DS_CREATE | DS_EXCL);
        ASSERT(err == DS_OK);
    }

    *data = d;
    return 0;
}

/** 
 * Update the value of the key, data pair in the database. It
 * should already exist.
 *
 * @param key   Key object
 * @param data  Data object
 * @param flags Bit vector of DurableStoreFlags_t values.
 * @return DS_OK, DS_NOTFOUND, DS_ERR
 */
template <typename _DataType>
inline int
SingleTypeDurableTable<_DataType>::put(const SerializableObject& key,
                                       const _DataType* data,
                                       int flags)
{
    int ret = this->impl_->put(key, TypeCollection::UNKNOWN_TYPE, data, flags);

    if (ret != DS_OK) {
        return ret;
    }
    
    if (this->cache_) {
        ret = this->cache_->put(key, data, flags);
        ASSERT(ret == DS_OK);
    }
    
    return ret;
}
    
/** 
 * Update the value of the key, data pair in the database. It
 * should already exist.
 *
 * @param key   Key object
 * @param data  Data object
 * @param flags Bit vector of DurableStoreFlags_t values.
 * @return DS_OK, DS_NOTFOUND, DS_ERR
 */
template <typename _BaseType, typename _Collection>
inline int
MultiTypeDurableTable<_BaseType, _Collection>::get(const SerializableObject& key,
                                                   _BaseType** data)
{
    int err;
    TypeCollection::TypeCode_t typecode;

    if (this->cache_) {
        err = this->cache_->get(key, data);
        if (err == DS_OK) {
            ASSERT(*data != NULL);
            return DS_OK;
        }
    }
    
    err = this->impl_->get_typecode(key, &typecode);
    if (err != DS_OK) {
        return err;
    }

    
    err = TypeCollectionInstance<_Collection>::instance()->
          new_object(typecode, data);
    if (err != 0) {
        return DS_ERR;
    }
    ASSERT(*data != NULL);

    err = this->impl_->get(key, *data);
    if (err != DS_OK) {
        delete *data;
        *data = NULL;
        return err;
    }

    if (this->cache_) {
        err = this->cache_->put(key, *data, DS_CREATE | DS_EXCL);
        ASSERT(err == DS_OK);
    }

    return DS_OK;
}

/**
 * Get the data for key from a multitype table, possibly creating a
 * new object based on the typecode in the multitype collection
 * specified by _Collection.
 *
 * @param key  Key object
 * @param data Data object
 *
 * @return DS_OK, DS_NOTFOUND if key is not found
 */
template <typename _BaseType, typename _Collection>
inline int
MultiTypeDurableTable<_BaseType, _Collection>::put(const SerializableObject& key,
                                                   TypeCollection::TypeCode_t type,
                                                   const _BaseType* data,
                                                   int flags)
{
    int ret = this->impl_->put(key, type, data, flags);

    if (ret != DS_OK) {
        return ret;
    }
    
    if (this->cache_) {
        ret = this->cache_->put(key, data, flags);
        ASSERT(ret == DS_OK);
    }

    return ret;
}

/**
 * Delete a (key,data) pair from the database
 *
 * @return DS_OK, DS_NOTFOUND if key is not found
 */
template <typename _Type>
inline int
DurableTable<_Type>::del(const SerializableObject& key)
{
    if (this->cache_) {
        this->cache_->del(key); // ignore return
    }
    
    return this->impl_->del(key);
}

/**
 * Return the number of elements in the table.
 */
template <typename _Type>
inline size_t
DurableTable<_Type>::size()
{
    return this->impl_->size();
}
