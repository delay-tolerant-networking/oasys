#include <bitset>

#include "util/UnitTest.h"
#include "util/StringBuffer.h"
#include "util/Random.h"
#include "storage/BerkeleyDBStore.h"
#include "storage/StorageConfig.h"
#include "serialize/TypeShims.h"

using namespace oasys;
using namespace std;

std::string g_db_name    = "test";
std::string g_db_table   = "test-table";
const char* g_config_dir = "output/berkeley-db-test/berkeley-db-test";

DurableStore*  g_store  = 0;
StorageConfig* g_config = 0;

typedef SingleTypeDurableTable<StringShim> StringDurableTable;

DECLARE_TEST(DBTestInit) {
    g_config = new StorageConfig(
        "storage",              // command name
        "berkeleydb",           // type
        true,                   // init
        true,                   // tidy
        0,                      // tidy wait
        g_db_name,              // dbname
        g_config_dir,           // dbdir
        "error.log",            // errlog
        0                       // flags
    );   

    return 0;
}

DECLARE_TEST(DBInit) {
    DurableStoreImpl* impl = new BerkeleyDBStore();
    DurableStore* store    = new DurableStore(impl);
    impl->init(g_config);

    delete_z(store);

    return 0;
}

DECLARE_TEST(DBTidy) {
    // do tidy
    g_config->tidy_        = true;
    DurableStoreImpl* impl = new BerkeleyDBStore();
    DurableStore* store    = new DurableStore(impl);
    impl->init(g_config);

    StringDurableTable* table1 = 0;    
    CHECK(store->get_table(&table1, "test", DS_CREATE | DS_EXCL, 0) == 0);
    CHECK(table1 != 0);

    delete_z(table1);
    delete_z(store);

    // don't do tidy
    g_config->tidy_ = false;
    impl            = new BerkeleyDBStore();
    store           = new DurableStore(impl);
    impl->init(g_config);

    CHECK(store->get_table(&table1, "test", 0, 0) == 0);
    CHECK(table1 != 0);
    delete_z(table1);
    delete_z(store);

    // do tidy
    g_config->tidy_ = true;
    impl            = new BerkeleyDBStore();
    store           = new DurableStore(impl);
    impl->init(g_config);

    CHECK(store->get_table(&table1, "test", 0, 0) == DS_NOTFOUND);
    CHECK(table1 == 0);
    delete_z(store);

    return 0;
}

DECLARE_TEST(TableCreate) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);
    
    StringDurableTable* table1 = 0;
    StringDurableTable* table2 = 0;

    CHECK(store->get_table(&table1, "test", 0, 0) == DS_NOTFOUND);
    CHECK(table1 == 0);
    
    CHECK(store->get_table(&table1, "test", DS_CREATE, 0) == 0);
    CHECK(table1 != 0);
    delete_z(table1);
    
    CHECK(store->get_table(&table2, "test", DS_CREATE | DS_EXCL, 0) 
          == DS_EXISTS);
    CHECK(table2 == 0);
    
    CHECK(store->get_table(&table2, "test", 0, 0) == 0);
    CHECK(table2 != 0);
    delete_z(table2);

    CHECK(store->get_table(&table1, "test", DS_CREATE, 0) == 0);
    CHECK(table1 != 0);
    delete_z(table1);
    delete_z(store);

    return 0;
}

DECLARE_TEST(TableDelete) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);
    
    StringDurableTable* table = 0;

    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, 0) == 0);
    CHECK(table != 0);
    delete_z(table);

    CHECK(store->del_table("test") == 0);
    
    CHECK(store->get_table(&table, "test", 0, 0) == DS_NOTFOUND);
    CHECK(table == 0);

    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, 0) == 0);
    CHECK(table != 0);
    delete_z(table);
    delete_z(store);

    return 0;
}

DECLARE_TEST(SingleTypePut) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);

    StringDurableTable* table;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, 0) == 0);

    IntShim    key(99);
    StringShim data("data");

    CHECK(table->put(key, &data, 0) == DS_NOTFOUND);
    CHECK(table->put(key, &data, DS_CREATE) == 0);
    CHECK(table->put(key, &data, 0) == 0);
    CHECK(table->put(key, &data, DS_CREATE) == 0);
    CHECK(table->put(key, &data, DS_CREATE | DS_EXCL) == DS_EXISTS);
    delete_z(table);
    delete_z(store);

    return 0;
}

DECLARE_TEST(SingleTypeGet) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);

    StringDurableTable* table;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, 0) == 0);

    IntShim    key(99);
    IntShim    key2(101);
    StringShim data("data");
    StringShim* data2 = 0;

    CHECK(table->put(key, &data, DS_CREATE | DS_EXCL) == 0);
    
    CHECK(table->get(key, &data2) == 0);
    CHECK(data2 != 0);
    CHECK(data2->value() == data.value());

    delete_z(data2);
    data2 = 0;
    
    CHECK(table->get(key2, &data2) == DS_NOTFOUND);
    CHECK(data2 == 0);

    data.assign("new data");
    CHECK(table->put(key, &data, 0) == 0);

    CHECK(table->get(key, &data2) == 0);
    CHECK(data2 != 0);
    CHECK(data2->value() == data.value());

    delete_z(data2);
    delete_z(table);
    delete_z(store);

    return 0;
}

DECLARE_TEST(SingleTypeDelete) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);

    StringDurableTable* table;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, 0) == 0);

    IntShim    key(99);
    IntShim    key2(101);
    StringShim data("data");
    StringShim* data2;

    CHECK(table->put(key, &data, DS_CREATE | DS_EXCL) == 0);
    
    CHECK(table->get(key, &data2) == 0);
    CHECK(data2 != 0);
    CHECK(data2->value() == data.value());
    delete_z(data2);
    data2 = 0;
    
    CHECK(table->del(key) == 0);

    CHECK(table->get(key, &data2) == DS_NOTFOUND);
    CHECK(data2 == 0);

    CHECK(table->del(key2) == DS_NOTFOUND);
    
    delete_z(table);
    delete_z(store);

    return 0;
}

DECLARE_TEST(SingleTypeMultiObject) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);

    StringDurableTable* table;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, 0) == 0);

    int num_objs = 100;
    
    for(int i=0; i<num_objs; ++i) {
        StaticStringBuffer<256> buf;
	buf.appendf("data%d", i);
        StringShim data(buf.c_str());
        
	CHECK(table->put(IntShim(i), &data, DS_CREATE | DS_EXCL) == 0);
    }
    
    delete_z(table);
    delete_z(store);

    g_config->tidy_ = false;
    impl  = new BerkeleyDBStore();
    store = new DurableStore(impl);
    impl->init(g_config);
    
    CHECK(store->get_table(&table, "test", 0, 0) == 0);

    PermutationArray pa(num_objs);

    for(int i=0; i<num_objs; ++i) {
        StaticStringBuffer<256> buf;
	StringShim* data = 0;

        IntShim key(pa.map(i));
        buf.appendf("data%d", pa.map(i));

	CHECK(table->get(key, &data) == 0);
        CHECK_EQUALSTR(buf.c_str(), data->value().c_str());
    }
    delete_z(table);
    delete_z(store);

    return 0;
}

DECLARE_TEST(SingleTypeIterator) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);

    StringDurableTable* table;
    static const int num_objs = 100;
     
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, 0) == 0);
    
    for(int i=0; i<num_objs; ++i) {
        StaticStringBuffer<256> buf;
	buf.appendf("data%d", i);
        StringShim data(buf.c_str());
        
	CHECK(table->put(IntShim(i), &data, DS_CREATE | DS_EXCL) == 0);
    }

    DurableIterator* iter = table->iter();

    bitset<num_objs> found;
    while(iter->next() == 0) {
        Builder b; // XXX/demmer can't use temporary ??
        IntShim key(b);
        
        iter->get(&key);
        CHECK(found[key.value()] == false);
        found.set(key.value());
    }

    found.flip();
    CHECK(!found.any());

    delete_z(iter); 
    delete_z(table);
    delete_z(store);

    return 0;    
}

DECLARE_TESTER(BerkleyDBTester) {
    ADD_TEST(DBTestInit);
    ADD_TEST(DBInit);
    ADD_TEST(DBTidy);
    ADD_TEST(TableCreate);
    ADD_TEST(TableDelete);
    ADD_TEST(SingleTypePut);
    ADD_TEST(SingleTypeGet);
    ADD_TEST(SingleTypeDelete);
    ADD_TEST(SingleTypeMultiObject);
    ADD_TEST(SingleTypeIterator);
}

DECLARE_TEST_FILE(BerkleyDBTester, "berkeley db test");
