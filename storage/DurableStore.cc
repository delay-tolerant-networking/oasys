#include "DurableStore.h"

namespace oasys {

int 
DurableStore::get_table(StaticTypedDurableTable** table, 
                        std::string               table_name,
                        int                       flags,
                        DurableObjectCache<SerializableObject>* cache)
{
    ASSERT(cache == 0); // no cache for now

    // XXX/bowei -- can't support tables that require 
    // prototyping...
    PrototypeVector prototypes;  

    DurableTableImpl* table_impl;
    int err = impl_->get_table(&table_impl, table_name, flags, prototypes);
    if (err != 0) {
        return err;
    }

    *table = new StaticTypedDurableTable(table_impl, table_name);
    return 0;
}

} // namespace oasys
