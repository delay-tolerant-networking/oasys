#include <bitset>

#include "util/UnitTest.h"
#include "util/StringBuffer.h"
#include "util/Random.h"
#include "storage/BerkeleyDBStore.h"
#include "serialize/TypeShims.h"

using namespace oasys;
using namespace std;

std::string    g_db_name      = "test";
const char*    g_config_dir   = "output/berkeley-db-test/berkeley-db-test";
DurableTableID g_id;

DECLARE_TEST(DBInit) {
    BerkeleyDBStore::init(g_db_name,
                        g_config_dir,
                        true, 0);

    BerkeleyDBStore::shutdown();
    return 0;
}

DECLARE_TEST(TableCreate) {
    BerkeleyDBStore::init(g_db_name,
                        g_config_dir,
                        false, 0);

    DurableTable* table1;
    DurableTable* table2;
    DurableStore* store = DurableStore::instance();

    CHECK(store->new_table(&table1, 10) == 0);
    CHECK(store->new_table(&table2, 10) == DS_EXISTS);
    delete table1;

    const int LIMIT = 1;
    for(int i=0; i<LIMIT; ++i) {
	CHECK(store->new_table(&table1) == 0);
	CHECK(store->new_table(&table2) == 0);
	delete table1;
	delete table2;

	CHECK(store->new_table(&table1) == 0);
	delete table1;
	CHECK(store->new_table(&table1) == 0);
	delete table1;
    }

    BerkeleyDBStore::shutdown();
    return 0;
}

DECLARE_TEST(Insert) {
    BerkeleyDBStore::init(g_db_name,
                        g_config_dir,
                        false, 0);
    DurableStore* store = DurableStore::instance();
    DurableTable* table;
    CHECK(store->new_table(&table) == 0);
    
    g_id = table->id();
    
    for(int i=0; i<500; ++i) {
        StaticStringBuffer<256> key;
        StaticStringBuffer<256> data;

	key.appendf("key%d", i);
	data.appendf("data%d", i);

	CHECK(table->put(NullStringShim(key.c_str()),
		         NullStringShim(data.c_str())) == 0);
    }

    delete table; table = 0;
    BerkeleyDBStore::shutdown();

    BerkeleyDBStore::init(g_db_name,
                        g_config_dir,
                        false, 0);
    store = DurableStore::instance();
    CHECK(store->get_table(g_id, &table) == 0);

    PermutationArray pa(500);

    for(int i=0; i<500; ++i) {
        StaticStringBuffer<256> key;
        StaticStringBuffer<256> proper_data;
	NullStringShim data;

	key.appendf("key%d", pa.map(i));
        proper_data.appendf("data%d", pa.map(i));

	CHECK(table->get(NullStringShim(key.c_str()), &data) == 0);
        CHECK_EQUALSTR(proper_data.c_str(), data.value());
    }

    delete table; table = 0;
    BerkeleyDBStore::shutdown();

    return 0;
}

DECLARE_TEST(Delete) {
    BerkeleyDBStore::init(g_db_name,
                        g_config_dir,
                        false, 0);
    DurableStore* store = DurableStore::instance();
    DurableTable* table;
    
    CHECK(store->get_table(g_id, &table) == 0);
    PermutationArray pa(200);

    for(int i=0; i<200; ++i) {
        StaticStringBuffer<256> key;
        key.appendf("key%d", pa.map(i));

        CHECK(table->del(NullStringShim(key.c_str())) == 0);

        NullStringShim dummy;
        CHECK(table->get(NullStringShim(key.c_str()), &dummy) == DS_NOTFOUND);
    }

    for(int i=200; i<500; ++i) {
        StaticStringBuffer<256> key;
        key.appendf("key%d", i);
        NullStringShim dummy;
        CHECK(table->get(NullStringShim(key.c_str()), &dummy) == 0);
    }

    delete table; table = 0;

    BerkeleyDBStore::shutdown();
    return 0;
}

DECLARE_TEST(Iterator) {
    BerkeleyDBStore::init(g_db_name,
                        g_config_dir,
                        false, 0);
    DurableStore* store = DurableStore::instance();
    DurableTable* table;
     
    CHECK(store->new_table(&table) == 0);
    g_id = table->id();
    
    for(int i=0; i<500; ++i) {
        StaticStringBuffer<256> key;
        StaticStringBuffer<256> data;

	key.appendf("key%d", i);
	data.appendf("data%d", i);

	CHECK(table->put(NullStringShim(key.c_str()),
		         NullStringShim(data.c_str())) == 0);
    }

    DurableIterator* iter;
    table->iter(&iter);

    bitset<500> found;
    while(iter->next() == 0) {
        NullStringShim key, data;

        iter->get(&key, &data);
        CHECK(key.value()[0] == 'k' && // lazy
              key.value()[1] == 'e' &&
              key.value()[2] == 'y');
        int idx = atoi(&key.value()[3]);
        found.set(idx);
    }

    found.flip();
    CHECK(!found.any());

    delete iter; iter = 0;
    delete table; table = 0;

    BerkeleyDBStore::shutdown();
    return 0;    
}

DECLARE_TESTER(BerkleyDBTester) {
    ADD_TEST(DBInit);
    ADD_TEST(TableCreate);
    ADD_TEST(Insert);
    ADD_TEST(Delete);
    ADD_TEST(Iterator);
}

DECLARE_TEST_FILE(BerkleyDBTester, "berkeley db test");
