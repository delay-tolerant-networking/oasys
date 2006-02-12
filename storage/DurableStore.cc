#include "config.h"

#include "DurableStore.h"
#include "BerkeleyDBStore.h"
#include "FileSystemStore.h"
#include "MemoryStore.h"
#include "StorageConfig.h"

namespace oasys {

int
DurableStore::create_store(const StorageConfig& config,
                           DurableStore**       store)
{
    if (0) {} // symmetry

    // filesystem store
    else if (config.type_.compare("filesysdb") == 0)
    {
        FileSystemStore* db = new FileSystemStore();
        int err = db->init(config);
        if (err != 0)
        {
            log_err("/durablestore", "Can't initialize filesysdb %d", err);
            return -1;
        }
        *store = new DurableStore(db);
    }

    // memory backed store
    else if (config.type_.compare("memorydb") == 0)
    {
        MemoryStore* db = new MemoryStore();
        int err = db->init(config);
        if (err != 0)
        {
            log_err("/durablestore", "Can't initialize filesysdb %d", err);
            return -1;
        }
        *store = new DurableStore(db);
    }

#if LIBDB_ENABLED
    // berkeley db
    else if (config.type_.compare("berkeleydb") == 0)
    {
        BerkeleyDBStore* db = new BerkeleyDBStore();
        int err = db->init(config);
        if (err != 0)
        {
            log_err("/durablestore", "Can't initialize berkeleydb %d", err);
            return -1;
        }
        *store = new DurableStore(db);
    }
#endif

#if MYSQL_ENABLED
#error Mysql support not yet added to oasys
#endif // MYSQL_ENABLED

#if POSTGRES_ENABLED
#error Postgres support not yet added to oasys
#endif // POSTGRES_ENABLED
        
    else
    {
        log_crit("storage type %s not implemented, exiting...",
                 config.type_.c_str());
        exit(1);
    }

    return 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int
DurableStore::get_table_names(StringVector* table_names)
{
    int err = impl_->get_table_names(table_names);
    return err;
}

} // namespace oasys
