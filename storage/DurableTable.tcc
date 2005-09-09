
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
int
SingleTypeDurableTable<_DataType>::get(const SerializableObject& key,
                                       _DataType** data)
{
    int err;
    _DataType* d = new _DataType(Builder());

    err = impl_->get(key, d);
    if (err != 0) {
        delete d;
        return err;
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
                                       const SerializableObject* data,
                                       int flags)
{
    return impl_->put(key, TypeCollection::UNKNOWN_TYPE, data, flags);
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
    TypeCollection::TypeCode_t typecode;

    int err = impl_->get_typecode(key, &typecode);
    if (err != DS_OK) {
        return err;
    }

    
    err = TypeCollectionInstance<_Collection>::instance()->
          new_object(typecode, data);
    if (err != 0) {
        return DS_ERR;
    }
    ASSERT(*data != NULL);

    err = impl_->get(key, *data);
    if (err != DS_OK) {
        delete *data;
        *data = NULL;
        return err;
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
int
MultiTypeDurableTable<_BaseType, _Collection>::put(const SerializableObject& key,
                                                   TypeCollection::TypeCode_t type,
                                                   const _BaseType* data,
                                                   int flags)
{
    return impl_->put(key, type, data, flags);
}

/**
 * Delete a (key,data) pair from the database
 *
 * @return DS_OK, DS_NOTFOUND if key is not found
 */
inline int
DurableTable::del(const SerializableObject& key)
{
    return impl_->del(key);
}

/**
 * Return the number of elements in the table.
 */
inline size_t
DurableTable::size()
{
    return impl_->size();
}
