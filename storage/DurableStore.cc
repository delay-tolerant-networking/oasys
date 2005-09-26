#include "DurableStore.h"

namespace oasys {

int 
DurableStore::get_table(NonTypedDurableTable** table, 
                        std::string            table_name,
                        int                    flags,
                        DurableObjectCache<SerializableObject>* cache)
{
    ASSERT(cache == 0); // no cache for now
    PrototypeVector prototypes;  // XXX/bowei -- can't support tables that require 
                                 // prototyping...

    DurableTableImpl* table_impl;
    int err = impl_->get_table(&table_impl, table_name, flags, prototypes);
    if (err != 0) {
        return err;
    }

    *table = new NonTypedDurableTable(table_impl, table_name);
    return 0;
}

} // namespace oasys
