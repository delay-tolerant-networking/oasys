#include <bitset>

#include "util/UnitTest.h"
#include "util/StringBuffer.h"
#include "util/Random.h"
#include "storage/BerkeleyDBStore.h"
#include "storage/StorageConfig.h"
#include "serialize/TypeShims.h"

using namespace oasys;
using namespace std;

std::string    g_db_name      = "test";
std::string    g_db_table     = "test-table";
const char*    g_config_dir   = "output/berkeley-db-test/berkeley-db-test";

typedef SingleTypeDurableTable<StringShim> StringDurableTable;

DECLARE_TEST(DBInit) {
    StorageConfig::init("berkeleydb", true, true, 0,
                        g_db_name, g_config_dir, "error.log", 0);
    BerkeleyDBStore::init();
    DurableStore::shutdown();
    return 0;
}

DECLARE_TEST(DBTidy) {
    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    DurableStore* store = DurableStore::instance();
    StringDurableTable* table1 = NULL;
    
    CHECK(store->get_table(&table1, "test", DS_CREATE | DS_EXCL, NULL) == 0);
    CHECK(table1 != NULL);
    delete table1;
    table1 = NULL;
    
    DurableStore::shutdown();

    StorageConfig::instance()->tidy_ = false;
    BerkeleyDBStore::init();
    store = DurableStore::instance();

    CHECK(store->get_table(&table1, "test", 0, NULL) == 0);
    CHECK(table1 != NULL);
    delete table1;
    table1 = NULL;

    DurableStore::shutdown();

    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    store = DurableStore::instance();

    CHECK(store->get_table(&table1, "test", 0, NULL) == DS_NOTFOUND);
    CHECK(table1 == NULL);

    DurableStore::shutdown();
    return 0;
}

DECLARE_TEST(TableCreate) {
    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    DurableStore* store = DurableStore::instance();
    
    StringDurableTable* table1 = NULL;
    StringDurableTable* table2 = NULL;

    CHECK(store->get_table(&table1, "test", 0, NULL) == DS_NOTFOUND);
    CHECK(table1 == NULL);
    
    CHECK(store->get_table(&table1, "test", DS_CREATE, NULL) == 0);
    CHECK(table1 != NULL);

    delete table1;
    table1 = NULL;
    
    CHECK(store->get_table(&table2, "test", DS_CREATE | DS_EXCL, NULL) == DS_EXISTS);
    CHECK(table2 == NULL);
    
    CHECK(store->get_table(&table2, "test", 0, NULL) == 0);
    CHECK(table2 != NULL);

    delete table2;
    table2 = NULL;

    CHECK(store->get_table(&table1, "test", DS_CREATE, NULL) == 0);
    CHECK(table1 != NULL);
    delete table1;

    DurableStore::shutdown();
    return 0;
}

DECLARE_TEST(TableDelete) {
    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    DurableStore* store = DurableStore::instance();
    
    StringDurableTable* table = NULL;

    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, NULL) == 0);
    CHECK(table != NULL);
    delete table;
    table = NULL;

    CHECK(store->del_table("test") == 0);
    
    CHECK(store->get_table(&table, "test", 0, NULL) == DS_NOTFOUND);
    CHECK(table == NULL);

    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, NULL) == 0);
    CHECK(table != NULL);
    delete table;
    table = NULL;

    DurableStore::shutdown();
    return 0;
}

DECLARE_TEST(SingleTypePut) {
    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    DurableStore* store = DurableStore::instance();

    StringDurableTable* table;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, NULL) == 0);

    IntShim    key(99);
    StringShim data("data");

    CHECK(table->put(key, &data, 0) == DS_NOTFOUND);
    CHECK(table->put(key, &data, DS_CREATE) == 0);
    CHECK(table->put(key, &data, 0) == 0);
    CHECK(table->put(key, &data, DS_CREATE) == 0);
    CHECK(table->put(key, &data, DS_CREATE | DS_EXCL) == DS_EXISTS);

    delete table;
    
    DurableStore::shutdown();
    return 0;
}

DECLARE_TEST(SingleTypeGet) {
    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    DurableStore* store = DurableStore::instance();

    StringDurableTable* table;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, NULL) == 0);

    IntShim    key(99);
    IntShim    key2(101);
    StringShim data("data");
    StringShim* data2 = NULL;

    CHECK(table->put(key, &data, DS_CREATE | DS_EXCL) == 0);
    
    CHECK(table->get(key, &data2) == 0);
    CHECK(data2 != NULL);
    CHECK(data2->value() == data.value());

    delete data2;
    data2 = NULL;
    
    CHECK(table->get(key2, &data2) == DS_NOTFOUND);
    CHECK(data2 == NULL);

    data.assign("new data");
    CHECK(table->put(key, &data, 0) == 0);

    CHECK(table->get(key, &data2) == 0);
    CHECK(data2 != NULL);
    CHECK(data2->value() == data.value());

    delete data2;
    data2 = NULL;

    delete table;
    DurableStore::shutdown();

    return 0;
}

DECLARE_TEST(SingleTypeDelete) {
    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    DurableStore* store = DurableStore::instance();

    StringDurableTable* table;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, NULL) == 0);

    IntShim    key(99);
    IntShim    key2(101);
    StringShim data("data");
    StringShim* data2;

    CHECK(table->put(key, &data, DS_CREATE | DS_EXCL) == 0);
    
    CHECK(table->get(key, &data2) == 0);
    CHECK(data2 != NULL);
    CHECK(data2->value() == data.value());
    delete data2;
    data2 = NULL;
    
    CHECK(table->del(key) == 0);

    CHECK(table->get(key, &data2) == DS_NOTFOUND);
    CHECK(data2 == NULL);

    CHECK(table->del(key2) == DS_NOTFOUND);
    
    delete table;
    DurableStore::shutdown();

    return 0;
}

DECLARE_TEST(SingleTypeMultiObject) {
    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    DurableStore* store = DurableStore::instance();

    StringDurableTable* table;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, NULL) == 0);

    int num_objs = 100;
    
    for(int i=0; i<num_objs; ++i) {
        StaticStringBuffer<256> buf;
	buf.appendf("data%d", i);
        StringShim data(buf.c_str());
        
	CHECK(table->put(IntShim(i), &data, DS_CREATE | DS_EXCL) == 0);
    }
    
    delete table;
    table = 0;
    DurableStore::shutdown();

    StorageConfig::instance()->tidy_ = false;
    BerkeleyDBStore::init();
    store = DurableStore::instance();
    
    CHECK(store->get_table(&table, "test", 0, NULL) == 0);

    PermutationArray pa(num_objs);

    for(int i=0; i<num_objs; ++i) {
        StaticStringBuffer<256> buf;
	StringShim* data = NULL;

        IntShim key(pa.map(i));
        buf.appendf("data%d", pa.map(i));

	CHECK(table->get(key, &data) == 0);
        CHECK_EQUALSTR(buf.c_str(), data->value().c_str());
    }

    delete table;
    table = 0;
    DurableStore::shutdown();

    return 0;
}

DECLARE_TEST(SingleTypeIterator) {
    StorageConfig::instance()->tidy_ = true;
    BerkeleyDBStore::init();
    DurableStore* store = DurableStore::instance();
    StringDurableTable* table;

    static const int num_objs = 100;
     
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, NULL) == 0);
    
    for(int i=0; i<num_objs; ++i) {
        StaticStringBuffer<256> buf;
	buf.appendf("data%d", i);
        StringShim data(buf.c_str());
        
	CHECK(table->put(IntShim(i), &data, DS_CREATE | DS_EXCL) == 0);
    }

    DurableIterator* iter = table->iter();

    bitset<num_objs> found;
    while(iter->next() == 0) {
        // XXX/demmer I don't know why a temporary doesn't work here...
        Builder b;
        IntShim key(b);
        
        iter->get(&key);
        CHECK(found[key.value()] == false);
        found.set(key.value());
    }

    found.flip();
    CHECK(!found.any());

    delete iter; iter = 0;
    delete table; table = 0;

    DurableStore::shutdown();
    return 0;    
}

DECLARE_TESTER(BerkleyDBTester) {
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
