#include "FileSystemStore.h"
#include "StorageConfig.h"

#include <errno.h>

namespace oasys {

//----------------------------------------------------------------------------
FileSystemStore::FileSystemStore()
    : DurableStoreImpl("/FileSystemStore"),
      tables_dir_("INVALID"),
      dir_(0)
{}

//----------------------------------------------------------------------------
FileSystemStore::~FileSystemStore()
{
    if (dir_ != 0) {
        closedir(dir_);
    }
}

//----------------------------------------------------------------------------
int 
FileSystemStore::init(StorageConfig* cfg)
{
    if (cfg->dbdir_ == "") {
        return -1;
    }

    if (cfg->dbname_ == "") {
        return -1;
    }

    tables_dir_ = cfg->dbdir_ + "/" + cfg->dbname_;

    // Always regenerate the directories if we are going to be
    // deleting them anyways
    if (cfg->tidy_) {
        cfg->init_ = true;
    }

    if (cfg->init_ && cfg->tidy_) 
    {
        if (check_database() == 0) {
            tidy_database();
        }
        int err = init_database();
        if (err != 0) {
            return -1;
        }
    }
    else if (cfg->init_ && ! cfg->tidy_) 
    {
        if (check_database() != -2) {
            log_warn("Database already exists - not clobbering...");
            return -1;
        }
        int err = init_database();
        if (err != 0) {
            return -1;
        } 
    }
    else 
    {
        if (check_database() != 0) {
            log_err("Database directory not found");
            return -1;
        }
    }

    log_info("init() done");
    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemStore::get_table(DurableTableImpl** table,
                           const std::string& name,
                           int                flags,
                           PrototypeVector&   prototypes)
{
    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemStore::check_database()
{
    dir_ = opendir(tables_dir_.c_str());
    if (dir_ == 0) {
        if (errno == ENOENT) {
            return -2;
        } else {
            return -1;
        }
    }
    closedir(dir_);

    return 0;
}

//----------------------------------------------------------------------------
int 
FileSystemStore::init_database()
{
    log_notice("Init() database");

    return 0;
}

//----------------------------------------------------------------------------
void
FileSystemStore::tidy_database()
{
    log_notice("Tidy() database");
    
    char cmd[256];
    int cc = snprintf(cmd, 256, "rm -rf %s", tables_dir_.c_str());
    ASSERT(cc < 256);
    system(cmd);
}

//----------------------------------------------------------------------------
int 
FileSystemStore::del_table(const std::string& name)
{
    return 0; // XXX/bowei
}

//----------------------------------------------------------------------------
FileSystemTable::FileSystemTable()
    : DurableTableImpl("/FileSystemTable", false)
{}

//----------------------------------------------------------------------------
int 
FileSystemTable::get(const SerializableObject& key,
                     SerializableObject* data)
{
    return 0; // XXX/bowei
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::get(const SerializableObject& key,
                     SerializableObject** data,
                     TypeCollection::Allocator_t allocator)
{
    return 0; // XXX/bowei
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::put(const SerializableObject& key,
                     TypeCollection::TypeCode_t typecode,
                     const SerializableObject* data,
                     int flags)
{
    return 0; // XXX/bowei
}
    
//----------------------------------------------------------------------------
int 
FileSystemTable::del(const SerializableObject& key)
{
    return 0; // XXX/bowei
}

//----------------------------------------------------------------------------
size_t 
FileSystemTable::size()
{
    return 0; // XXX/bowei
}
    
//----------------------------------------------------------------------------
DurableIterator* 
FileSystemTable::iter()
{
    return 0; // XXX/bowei
}

//----------------------------------------------------------------------------
FileSystemIterator::FileSystemIterator(FileSystemTable* t)
{
}

//----------------------------------------------------------------------------
FileSystemIterator::~FileSystemIterator()
{
}
    
//----------------------------------------------------------------------------
int 
FileSystemIterator::next()
{
    return 0; // XXX/bowei
}

//----------------------------------------------------------------------------
int 
FileSystemIterator::get(SerializableObject* key)
{
    return 0; // XXX/bowei
}

} // namespace oasys
