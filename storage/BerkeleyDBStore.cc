#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "debug/Debug.h"
#include "util/StringBuffer.h"

#include "DurableTable.h"
#include "BerkeleyTable.h"

#define NO_TX  0 // for easily going back and changing TX id's later

namespace oasys {

/******************************************************************************
 *
 * BerkeleyDs
 *
 *****************************************************************************/
BerkeleyTableStore::BerkeleyTableStore() 
    : Logger("/berkeleydb/store")
{
    // Real init code in do_init. 
}

BerkeleyTableStore::~BerkeleyTableStore()
{
    StringBuffer err_str;

    err_str.append("Tables still open at deletion time: ");
    bool busy = false;

    for(RefCountMap::iterator itr = ref_count_.begin(); 
        itr != ref_count_.end();
        ++itr)
    {
        if(itr->second != 0)
        {
            err_str.appendf("%d ", itr->first);
            busy = true;
        }
    }

    if(busy)
    {
        log_err(err_str.c_str());
    }
    dbenv_->close(0);
    
    delete dbenv_;
    dbenv_ = 0;

    fclose(err_log_);
}

void
BerkeleyTableStore::init()
{
    BerkeleyTableStore* s = new BerkeleyTableStore();
    DurableTableStore::init(s);
    if(s->do_init() != 0) {
        PANIC("Can't init() BerkeleyDB");
    }
}

int 
BerkeleyTableStore::do_init()
{
    BerkeleyTableStoreCommand* config = BerkeleyTableStoreCommand::instance();
    config->ds_ = this;
    
    // cache database name
    db_name_ = config->db_name_;

    // create database directory
    struct stat f_stat;
    const char* config_dir = config->dir_.c_str();
    if (stat(config_dir, &f_stat) == -1)
    {
        if (errno == ENOENT)
        {
            log_info("creating new database directory %s", config_dir);

            if (mkdir(config_dir, 0700) != 0) 
            {
                log_crit("can't create datastore directory %s: %s",
                         config_dir, strerror(errno));
                return -1;
            }
        }
    }

    // Do real initialization
    dbenv_   = new DbEnv(0);
    err_log_ = ::fopen(config->err_log_.c_str(), "w");

    if(err_log_ == NULL) 
    {
        log_err("Can't create db error log file");
    }
    else 
    {
        dbenv_->set_errfile(err_log_);
    }

    log_info("Using dbdir = %s, errlog = %s", config_dir, config->err_log_.c_str());
    
    if (config->tidy_db_)
    {
        char cmd[256];
        for (int i = config->tidy_wait_; i > 0; --i) {
            log_warn("PRUNING CONTENTS OF %s IN %d SECONDS",
                     config_dir, i);
            sleep(1);
        }
        sprintf(cmd, "/bin/rm -rf %s", config_dir);
        system(cmd);
    }

    if (stat(config_dir, &f_stat) == -1)
    {
        if (errno == ENOENT)
        {
            log_info("creating new database directory %s", config_dir);

            if (mkdir(config_dir, 0700) != 0) {
                log_crit("can't create datastore directory: %s", strerror(errno));
                return -1;
            }
        }
    }

    try 
    {
        dbenv_->open(config_dir,
                     DB_CREATE     | // create new files
                     DB_INIT_MPOOL | // initialize memory pool
                     DB_INIT_LOG   | // use logging
                     DB_INIT_TXN   | // use transactions
                     DB_RECOVER    | // recover from previous crash (if any)
                     DB_PRIVATE,     // only one process can access the db
                     0);             // no flags

        dbenv_->set_flags(DB_AUTO_COMMIT, // every op is automatically in a tx
                          1);
    } 
    catch(DbException e) 
    {
        log_crit("DB: %s, cannot open database", e.what());
        return -1;
    }

    // Create database file if none exists. Table 0 is used for
    // storing general global metadata.
    std::string dbpath = config_dir;
    dbpath += "/";
    dbpath += config->db_name_;

    if(stat(dbpath.c_str(), &f_stat) == -1) 
    {
        if(errno == ENOENT)
        {
            Db db(dbenv_, 0);
            log_info("Creating new datafile file");
            
            try 
            {
                db.open(NO_TX, config->db_name_.c_str(),
                        "0", DB_HASH ,DB_CREATE, 0);
                db.close(0);
            }
            catch(DbException e) 
            {
                log_err("DB: %s, could not create metatable", e.what());
                return -1;
            }
        }
        else
        {
            log_err("Unable to stat database file %d", errno);
            return -1;
        }
    }

    // Initialize max id from meta-database.
    DurableTable* metatable;
    if(get_meta_table(&metatable) != 0) 
    {
        log_crit("Unable to open metatable!");
        return -1;
    }
    
    DurableTableItr* itr = new BerkeleyTableItr(metatable);
//    int max_id = 0;

/* XXX/bowei
    while(itr->next() == DS_OK)
    {
        // XXX
        int id = atoi(static_cast<const char*>(itr->key()));
        max_id = (max_id < id) ? id : max_id;
    }
    next_id_ = max_id + 1;
*/

    delete itr;
    delete metatable;

    return 0;
}

int
BerkeleyTableStore::new_table(DurableTable** table)
{
    DurableTableId id = next_id_;
    next_id_++;
    
    Db* db = new Db(dbenv_, 0);

    try 
    {
        db->open(NO_TX, db_name_.c_str(), get_name(id).c_str(), 
                 DB_HASH, DB_CREATE, 0);
    } 
    catch(DbException e)
    {
        // XXX/bowei - errors
        log_err("DB: %s", e.what());
        ASSERT(0);
    }
    
    log_debug("Creating new table %d", id);

    *table = new BerkeleyTable(id, db);

    return DS_OK;
}


int
BerkeleyTableStore::del_table(int id)
{
    if(ref_count_[id] != 0)
    {
        log_info("Trying to delete table %d with %d refs still on it",
                 id, ref_count_[id]);

        return DS_BUSY;
    }

    try 
    {
        dbenv_->dbremove(NO_TX, db_name_.c_str(), get_name(id).c_str(), 0);
        log_info("Deleting table %d", id);
    } 
    catch(DbException e)
    {
        log_err("DB: del_table %s", e.what());

        if(e.get_errno() == ENOENT) 
        {
            return DS_NOTFOUND;
        }
        else 
        {
            return DS_ERR;
        }
    }
    
    ref_count_.erase(id);

    return DS_OK;
}


int 
BerkeleyTableStore::get_table(int id, DurableTable** table)
{
    Db* db = new Db(dbenv_, 0);
    
    try 
    {
        db->open(NO_TX, db_name_.c_str(), get_name(id).c_str(), DB_UNKNOWN, 0, 0);
    } 
    catch(DbException e)
    {
        switch(e.get_errno()) 
        {
        case ENOENT:
            return DS_NOTFOUND;
        default:
            log_err("DB: %s", e.what());
            return DS_ERR;
        }
    }
    
    *table = new BerkeleyTable(id, db);
    
    return DS_OK;
}

int  
BerkeleyTableStore::get_meta_table(DurableTable** table)
{
    Db* db = new Db(dbenv_, 0);
    
    try 
    {
        db->open(NO_TX, db_name_.c_str(), 
                 NULL, DB_UNKNOWN, DB_RDONLY, 0);
    }
    catch(DbException e) 
    {
        log_err("unable to open metatable - DB: %s", e.what());
        return DS_ERR;
    }
    
    *table = new BerkeleyTable(META_TABLE_ID, db);
    
    return DS_OK;
}

int
BerkeleyTableStore::acquire_table(DurableTableId id)
{
    ++ref_count_[id];
    ASSERT(ref_count_[id] >= 0);

    log_debug("table %d, +refcount=%d", id, ref_count_[id]);

    return ref_count_[id];
}

int
BerkeleyTableStore::release_table(DurableTableId id)
{
    --ref_count_[id];
    ASSERT(ref_count_[id] >= 0);

    log_debug("table %d, -refcount=%d", id, ref_count_[id]);

    return ref_count_[id];
}

std::string 
BerkeleyTableStore::get_name(int id)
{
    char buf[256];
    sprintf(buf, "%d", id);

    return buf;
}

/******************************************************************************
 *
 * Dstable
 *
 *****************************************************************************/
BerkeleyTable::BerkeleyTable(int id, Db* db)
    : DurableTable(id), db_(db)
{
    logpathf("/berkeleydb/table(%d)", id);

    BerkeleyTableStore* store = 
        static_cast<BerkeleyTableStore*>(BerkeleyTableStore::instance()); 
    store->acquire_table(this->id());
}


BerkeleyTable::~BerkeleyTable() 
{
    // Note: If we are to multithread access to the same table, this
    // will have potential concurrency problems, because close can
    // only happen if no other instance of Db is around.
    BerkeleyTableStore* store = 
        static_cast<BerkeleyTableStore*>(BerkeleyTableStore::instance()); 
    
    if(store->release_table(id()) == 0)
    {
        log_debug("closing Db %d", id());
        db_->close(0);
    }

    delete db_;
}

int 
BerkeleyTable::get(const SerializableObject& key, SerializableObject* data)
{
    return 0;
}

int 
BerkeleyTable::put(const SerializableObject& key, const SerializableObject* data)
{
    return 0;
}

int 
BerkeleyTable::del(const SerializableObject& key)
{
    return 0;
}

#if 0
int 
BerkeleyTable::get(const void* key, int key_len, void* data, int *data_len)
{
    Dbt k(const_cast<void*>(key), key_len), d;
    int err = 0;

    try 
    {
        err = db_->get(NO_TX, &k, &d, 0);
    } 
    catch(DbException e) 
    {
        log_err("DB: %s", e.what());
        ASSERT(0); // XXX/bowei for now
    }
     
    if(err == DB_NOTFOUND) 
    {
        return DS_NOTFOUND;
    }

    if(*data_len < static_cast<int>(d.get_size())) {
        return DS_BUFSIZE;
    }

    *data_len = d.get_size();
    memcpy(data, d.get_data(), d.get_size());

    return DS_OK;
}


int 
BerkeleyTable::put(const void* key, int key_len, const void* data, int data_len)
{
    Dbt k(const_cast<void*>(key), key_len);
    Dbt d(const_cast<void*>(data), data_len);

    log_debug("put(%.*s): length %d", key_len, (char*)key, data_len);

    // safety check
    if (data_len > 10000000) {
        log_warn("large db put length %d (table %d key %.*s)",
                 data_len, id_, key_len, (char*)key);
    }

    int err = 0;
    
    try 
    {
        err = db_->put(NO_TX, &k, &d, 0);
    }
    catch(DbException e)
    {
        log_err("DB: %s", e.what());
        ASSERT(0); // XXX/bowei for now
    }

    return DS_OK;
}

int 
BerkeleyTable::del(const void* key, int key_len)
{
    Dbt k(const_cast<void*>(key), key_len);
    int err = DS_OK;

    try 
    {
        if(db_->del(NO_TX, &k, 0) == DB_NOTFOUND) 
        {
            err = DS_NOTFOUND;
        }
    } 
    catch(DbException e)
    {
        log_err("DB: %s", e.what());
        ASSERT(0); // XXX/bowei for now        
    }

    return err;
}
#endif // 0

/******************************************************************************
 *
 * BerkeleyTableItr
 *
 *****************************************************************************/
BerkeleyTableItr::BerkeleyTableItr(DurableTable* d)
    : cur_(NULL), valid_(false)
{
    logpathf("/berkeleydb/itr(%d)", d->id());

    BerkeleyTable* t = dynamic_cast<BerkeleyTable*>(d);
    ASSERT(t != 0);

    try {
        t->db_->cursor(NO_TX, &cur_, 0);
    } catch(DbException e) {
        log_err("DB: cannot create a DB iterator, err=%s", e.what());
        cur_ = NULL;
    }

    if(cur_)
    {
        valid_ = true;
    }
}

BerkeleyTableItr::~BerkeleyTableItr()
{
    valid_ = false;
    if(cur_) 
    {
        cur_->close();
    }
}

int
BerkeleyTableItr::next()
{
    ASSERT(valid_);

    int err;

    try {
        err = cur_->get(&key_, &data_, DB_NEXT);
    } catch(DbException e) {
        log_err("DB: %s", e.what());
        valid_ = false;
        
        return DS_ERR;
    }
    
    if(err == DB_NOTFOUND)
    {
        valid_ = false;

        return DS_NOTFOUND;
    }

    return DS_OK;
}

int 
BerkeleyTableItr::get(SerializableObject* obj)
{
    return 0; // XXX/bowei
}


/******************************************************************************
 *
 * BerkeleyTableStoreCommand
 *
 *****************************************************************************/
BerkeleyTableStoreCommand* BerkeleyTableStoreCommand::instance_;

BerkeleyTableStoreCommand::BerkeleyTableStoreCommand()
    : oasys::AutoTclCommand("ds"), 
      tidy_db_(false),
      tidy_wait_(3),
      dir_("/var/tier/db"),
      err_log_("/var/tier/db/err.log"),
      ds_(NULL)
{
}

void
BerkeleyTableStoreCommand::bind_vars()
{
    bind_b("tidy",   	&tidy_db_);
    bind_i("tidywait",	&tidy_wait_);
    bind_s("dir",    	&dir_);
    bind_s("errlog", 	&err_log_);
}

int
BerkeleyTableStoreCommand::exec(int argc, const char** argv, Tcl_Interp* interp)
{
#if 0
    int err;

    switch(argc) 
    {
    case 2: {
        if(strcmp(argv[1], "info") == 0) 
        {
            
            std::string out;
            for(std::map<int,int>::iterator itr = ds_->ref_count_.begin();
                itr != ds_->ref_count_.end();
                ++itr)
            {
                char buf[50];
                sprintf(buf, "(%d, %d) ", (*itr).first, (*itr).second);
                out += buf;
            }
            log_info("%s", out.c_str());
            
            return 0;
        } // if(info)
        break;
    } // case 2

    case 3: {
        if(strcmp(argv[1], "get") == 0) 
        {
            if(strcmp(argv[2], "tables") == 0) 
            {
                BerkeleyTable* m;
                err = ds_->get_meta_table(&m);
                if (err != DS_OK) {
                    resultf("unable to get metatable");
                    return TCL_ERROR;
                }
                
                BerkeleyTableItr* itr = new BerkeleyTableItr(m);
                std::string result;
                
                while(itr->next() == DS_OK) 
                {
                    char buf[200];
                    memcpy(buf, itr->key(), itr->key_len());
                    buf[itr->key_len()] = '\0';
                    result = result + " " + buf;
                }

                log_info("tables %s", result.c_str());
                set_result(result.c_str());

                delete itr;
                delete m;

		return 0;
            }
        } // if(get)
        else if(strcmp(argv[1], "delete") == 0)
        {
            int id = atoi(argv[2]);
            
            if(id <= 0)
            {
                set_result( "Can't delete table <= 0.");
                return -1;
            }

            if(ds_->ref_count_[id] != 0) 
            {
                set_result( "Table still has references.");
                return -1;
            }

            int err;
            if((err = ds_->del_table(id)) != DS_OK)
            {
                if(err == DS_NOTFOUND)
                {
                    set_result("Table not found.");
                    return -1;
                }
                else
                {
                    set_result("Error deleting table.");
                    return -1;
                }
            }
            return 0;
        } // if(delete)

        break;
    } // case 3

    case 4: {
        if(strcmp(argv[1], "get") == 0)
        {
            if(strcmp(argv[2], "table") == 0)
            {
                DurableTable* t;
                int table = atoi(argv[3]);

                err = BerkeleyTableStore:instance()->get_table(table, &t);
                if (err != DS_OK) {
                    set_result("Couldn't get table");
                    return -1;
                }
                BerkeleyTableItr* itr = new BerkeleyTableItr(t);
             
                oasys::StringBuffer result;
                    
                while(itr->next() == DS_OK) {
                    result.appendf("%.*s { %.*s }",
                                   itr->key_len(),
                                   (char*)itr->key(),
                                   itr->data_len() < 50 ? itr->data_len() : 50,
                                   (char*)itr->data());
                }

                // make unprintable characters '?'
                char* buf = result.data();
                for (size_t i = 0; i < result.length(); i++) {
                    if (buf[i] < 32 || buf[i] > 126)
                        buf[i] = '?';
                }
                
                set_result(result.c_str());
                
                delete itr;
                delete t;

		return 0;
            }
        } // if(get)        

        break;
    } // case 4
    } // switch(argc)

    set_result("Unsupported command");
    return -1;
    #endif // if 0

    return -1;
}

}; // namespace oasys
