#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"

#include "DurableStore.h"
#include "BerkeleyDBStore.h"
#include "FileSystemStore.h"
#include "MemoryStore.h"
#include "StorageConfig.h"

namespace oasys {

DurableStore::~DurableStore()
{ 
    delete impl_; 
    impl_ = 0;

    int fd = creat(clean_shutdown_file_.c_str(), S_IRUSR);
    if (fd < 0) {
        log_err("/storage",
                "error creating shutdown file '%s': %s",
                clean_shutdown_file_.c_str(), strerror(errno));
    } else {
        log_debug("/storage", "successfully created clean shutdown file '%s'",
                  clean_shutdown_file_.c_str());
        close(fd);
    }
}

int
DurableStore::create_store(const StorageConfig& config,
                           DurableStore**       store,
                           bool*                clean_shutdown)
{
    DurableStoreImpl* impl = NULL;
        
    if (0) {} // symmetry

    // filesystem store
    else if (config.type_ == "filesysdb")
    {
        impl = new FileSystemStore();
    }

    // memory backed store
    else if (config.type_ == "memorydb")
    {
        impl = new MemoryStore();
    }

#if LIBDB_ENABLED
    // berkeley db
    else if (config.type_ == "berkeleydb")
    {
        impl = new BerkeleyDBStore();
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
        log_crit("/storage", "storage type %s not implemented, exiting...",
                 config.type_.c_str());
        exit(1);
    }
    
    int err = impl->init(config);
    if (err != 0)
    {
        log_err("/storage", "can't initialize %s %d",
                config.type_.c_str(), err);
        return DS_ERR;
    }

    *store = new DurableStore(impl);

    (*store)->clean_shutdown_file_ = config.dbdir_;
    (*store)->clean_shutdown_file_ += "/.ds_clean";
    
    // try to remove the clean shutdown file
    err = unlink((*store)->clean_shutdown_file_.c_str());
    if ((err == 0) ||
        (errno == ENOENT && config.init_ == true))
    {
        log_info("/storage", "datastore %s was cleanly shut down",
                 config.dbdir_.c_str());
        if (clean_shutdown) {
            *clean_shutdown = true;
        }
    } else {
        log_info("/storage", "datastore %s was not cleanly shut down",
                 config.dbdir_.c_str());
        if (clean_shutdown) {
            *clean_shutdown = false;
        }
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
