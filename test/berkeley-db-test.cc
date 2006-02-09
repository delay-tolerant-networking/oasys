#include <string>
#include "storage/BerkeleyDBStore.h"

//
// globals needed by the generic durable-store-test
//
#define DEL_DS_STORE(store) delete_z(store)

std::string g_db_name    = "test";
std::string g_db_table   = "test-table";
const char* g_config_dir = "output/berkeley-db-test/berkeley-db-test";

//
// pull in the generic test
//
#include "durable-store-test.cc"

DECLARE_TEST(DBTestInit) {
    g_config = new StorageConfig(
        "storage",              // command name
        "berkeleydb",           // type
        true,                   // init
        true,                   // tidy
        0,                      // tidy wait
        g_db_name,              // dbname
        g_config_dir,           // dbdir
        0,                      // flags,
        1000,			// num tx
        5000,			// deadlock check interval
        false			// sharefile
    );   

    StringBuffer cmd("mkdir -p %s", g_config_dir);
    system(cmd.c_str());

    return 0;
}

DECLARE_TEST(DBSwitchToSharedFile) {
    g_config->dbsharefile_ = true;
    return UNIT_TEST_PASSED;
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
    ADD_TEST(SingleTypeCache);

    ADD_TEST(NonTypedTable);
    ADD_TEST(MultiType);
    ADD_TEST(MultiTypeCache);

    ADD_TEST(DBSwitchToSharedFile);
    
    ADD_TEST(DBInit);
    ADD_TEST(DBTidy);
    ADD_TEST(TableCreate);
    ADD_TEST(TableDelete);

    ADD_TEST(SingleTypePut);
    ADD_TEST(SingleTypeGet);
    ADD_TEST(SingleTypeDelete);
    ADD_TEST(SingleTypeMultiObject);
    ADD_TEST(SingleTypeIterator);
    ADD_TEST(SingleTypeCache);

    ADD_TEST(NonTypedTable);
    ADD_TEST(MultiType);
    ADD_TEST(MultiTypeCache);

}

DECLARE_TEST_FILE(BerkleyDBTester, "berkeley db test");
