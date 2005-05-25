
/**
 * Get a new handle on a table.
 *
 * @param flags options for creating the table
 * @param id what the id of the table should be if specified
 * @return DS_OK, DS_NOTFOUND, DS_EXISTS, DS_ERR
 */
template <typename _DataType>
int
DurableStore::get_table(SingleTypeDurableTable<_DataType>** table,
                        std::string         table_name,
                        int                 flags,
                        DurableObjectCache* cache)
{
    int err;

    // build the (single-element) vector of prototypes -- note that
    // the vector stores auto_ptr instances so the objects are cleaned
    // up automatically when the vector goes away
    PrototypeVector prototypes;
    prototypes.push_back(std::auto_ptr(new _DataType(Builder())));

    DurableTableImpl* table_impl;
    err = impl_->get_table(&table_impl, flags, db_name, prototypes);
    if (err != 0) {
        return err;
    }

    *table = new SingleTypeDurableTable<_DataType>(table_impl, table_name, cache);

    return 0;
}

/**
 * Template specialization for the NoCollection case.
 */
template <typename _BaseType, typename _Collection>
int
DurableStore::get_table(MultiTypeDurableTable<_BaseType, _Collection>** table,
                        std::string         table_name,
                        int                 flags,
                        DurableObjectCache* cache)
{
    int err;
    TypeCollection<_Collection> *collection =
        TypeCollection<_Collection>::instance();
    
    typename TypeCollection<_Collection>::TypeCode_t code, low, high;
    _BaseType* obj;

    // Find the range of type codes
    low  = TypeCollectionCode<_Collection, _BaseType>::TYPECODE_LOW;
    high = TypeCollectionCode<_Collection, _BaseType>::TYPECODE_HIGH;

    // Now for each defined element in the range, create an instance
    // and stuff it in the vector of protypes. Note that the vector
    // stores auto_ptr instances so the objects are cleaned up
    // automatically when the vector is destroyed.
    PrototypeVector prototypes;

    for (typecode = low; typecode <= high; ++typecode)
    {
        err = collection->new_object(typecode, &obj);

        if (err == 0)
        {
            protoypes.push_back(std::auto_ptr(obj));
        }
        else if (err == TypeCollectionErr::TYPECODE)
        {
            continue;
        }
        else
        {
            log_crit("unknown error from TypeCollection::new_object");
            return DS_ERR;
        }
    }

    DurableTableImpl* table_impl;
    err = impl_->get_table(&table_impl, flags, db_name, prototypes);
    if (err != 0) {
        return err;
    }

    *table = new MultiTypeDurableTable<_BaseType, _Collection>(table_impl,
                                                               table_name,
                                                               cache);
    return 0;
}

/**
 * Delete the table (by id) from the datastore.
 */
int
DurableStore::del_table(std::string db_name)
{
    return impl_->del_table(db_name);
}
