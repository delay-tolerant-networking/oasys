// Included by DurableTable.h


//----------------------------------------------------------------------------
template <typename _Type>
inline bool
DurableTable<_Type>::is_live_in_cache(const SerializableObject& key) const
{
    if (this->cache_ != 0) {
        return this->cache_->is_live(key);
    }
    return false;
}

//----------------------------------------------------------------------------
template <typename _Type>
inline int
DurableTable<_Type>::del(const SerializableObject& key)
{
    if (this->cache_ != 0) {
        this->cache_->del(key); // ignore return
    }
    
    return this->impl_->del(key);
}

//----------------------------------------------------------------------------
template <typename _Type>
inline size_t
DurableTable<_Type>::size()
{
    return this->impl_->size();
}

//----------------------------------------------------------------------------
template <typename _DataType>
inline int
SingleTypeDurableTable<_DataType>::get(const SerializableObject& key,
                                       _DataType** data)
{
    int err;

    if (this->cache_ != 0) {
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

    if (this->cache_ != 0) {
        err = this->cache_->put(key, d, DS_CREATE | DS_EXCL);
        ASSERT(err == DS_OK);
    }

    *data = d;
    return 0;
}

//----------------------------------------------------------------------------
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
    
    if (this->cache_ != 0) {
        ret = this->cache_->put(key, data, flags);
        ASSERT(ret == DS_OK);
    }
    
    return ret;
}
    
//----------------------------------------------------------------------------
template <typename _BaseType, typename _Collection>
inline int
MultiTypeDurableTable<_BaseType, _Collection>::get(
    const SerializableObject& key,
    _BaseType** data
    )
{
    int err;
    TypeCollection::TypeCode_t typecode;

    if (this->cache_ != 0) {
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

    if (this->cache_ != 0) {
        err = this->cache_->put(key, *data, DS_CREATE | DS_EXCL);
        ASSERT(err == DS_OK);
    }

    return DS_OK;
}

//----------------------------------------------------------------------------
template <typename _BaseType, typename _Collection>
inline int
MultiTypeDurableTable<_BaseType, _Collection>::put(
    const SerializableObject& key,
    TypeCollection::TypeCode_t type,
    const _BaseType* data,
    int flags
    )
{
    int ret = this->impl_->put(key, type, data, flags);

    if (ret != DS_OK) {
        return ret;
    }
    
    if (this->cache_ != 0) {
        ret = this->cache_->put(key, data, flags);
        ASSERT(ret == DS_OK);
    }

    return ret;
}

//----------------------------------------------------------------------------
template<typename _Type>
inline int 
StaticTypedDurableTable::put(
    const SerializableObject& key,
    const _Type*              data,
    int                       flags
    )
{
    ASSERT(this->cache_ == 0); // XXX/bowei - don't support caches for now
    int ret = this->impl_->put(key, TypeCollection::UNKNOWN_TYPE, 
                               data, flags);
    ASSERT(ret == DS_OK);

    return ret;
}

//----------------------------------------------------------------------------
template<typename _Type>
inline int 
StaticTypedDurableTable::get(
    const SerializableObject& key,
    _Type**                   data
    )
{
    ASSERT(this->cache_ == 0); // XXX/bowei - don't support caches for now

    _Type* new_obj = new _Type(Builder());
    ASSERT(new_obj != 0);
    int err = this->impl_->get(key, new_obj);

    if (err != DS_OK) {
        delete_z(new_obj);
	*data = 0;

        return err;
    }
	
    *data = new_obj;

    return DS_OK;
}
