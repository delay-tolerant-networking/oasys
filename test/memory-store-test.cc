#include <string>
#include "storage/MemoryStore.h"

//
// globals needed by the generic durable-store-test
//

#define DEL_DS_STORE(store) delete_z(store) 

using namespace oasys;

//
// pull in the generic test
//
#include "durable-store-test.cc"

DECLARE_TEST(DBTestInit) {
    g_config = new StorageConfig(
        "storage",              // command name
        "memorydb",             // type
        true,                   // init
        true,                   // tidy
        0,                      // tidy wait
        "",	                // dbname
        "", 		        // dbdir
        0,                      // flags
        0,			// bdb specific
        0,			// bdb specific
        false			// sharefile
    );   

    return 0;
}

DECLARE_TESTER(MemoryStoreTester) {
    ADD_TEST(DBTestInit);

    ADD_TEST(DBInit);
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

DECLARE_TEST_FILE(MemoryStoreTester, "memory store test");
