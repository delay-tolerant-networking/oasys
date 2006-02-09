#include <string>
#include "storage/FileSystemStore.h"

//
// globals needed by the generic durable-store-test
//
#define DEL_DS_STORE(store) delete_z(store)

std::string g_db_name    = "test-db";
std::string g_db_table   = "test-table";
const char* g_config_dir = "output/filesys-db-test/filesys-db-test";

//
// pull in the generic test
//

#include "durable-store-test.cc"

DECLARE_TEST(DBTestInit) {
    g_config = new StorageConfig(
        "storage",              // command name
        "filesysdb",            // type
        true,                   // init
        true,                   // tidy
        0,                      // tidy wait
        g_db_name,              // dbname
        g_config_dir,           // dbdir
        0,                      // flags
        1,                      // txmax
        0,                      // lock detect
        0                       // share file
    );   

    StringBuffer cmd("mkdir -p %s", g_config_dir);
    system(cmd.c_str());

    return 0;
}

DECLARE_TESTER(FilesysDBTester) {
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
}

DECLARE_TEST_FILE(FilesysDBTester, "filesystem db test");
