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
typedef DurableObjectCache<StringShim> StringDurableCache;

struct TestC {};

TYPE_COLLECTION_INSTANTIATE(TestC);

class Obj : public oasys::SerializableObject {
public:
    Obj(int id, const char* static_name) : id_(id), static_name_(static_name) {}
    Obj(const Builder& b) : id_(0), static_name_("no name") {}
    
    virtual const char* name() = 0;

    virtual void serialize(SerializeAction* a) {
        a->process("name", &static_name_);
        a->process("id", &id_);
    }

    int id_;
    std::string static_name_;
};

class Foo : public Obj {
public:
    static const int ID = 1;
    Foo(const char* name = "foo") : Obj(ID, name) {}
    Foo(const Builder& b) : Obj(b) {}
    virtual const char* name() { return "foo"; }
};

class Bar : public Obj {
public:
    static const int ID = 2;
    Bar(const char* name = "bar") : Obj(ID, name) {}
    Bar(const Builder& b) : Obj(b) {}
    virtual const char* name() { return "bar"; }
};

TYPE_COLLECTION_DECLARE(TestC, Foo, 1);
TYPE_COLLECTION_DECLARE(TestC, Bar, 2);
TYPE_COLLECTION_GROUP(TestC, Obj, 1, 2);

TYPE_COLLECTION_DEFINE(TestC, Foo, 1);
TYPE_COLLECTION_DEFINE(TestC, Bar, 2);

#define TYPECODE_FOO (TypeCollectionCode<TestC, Foo>::TYPECODE)
#define TYPECODE_BAR (TypeCollectionCode<TestC, Bar>::TYPECODE)

typedef MultiTypeDurableTable<Obj, TestC> ObjDurableTable;
typedef DurableObjectCache<Obj> ObjDurableCache;

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
    StringBuffer cmd("mkdir -p %s", g_config_dir);
    system(cmd.c_str());
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
    CHECK(store->get_table(&table1, "test", DS_CREATE | DS_EXCL) == 0);
    CHECK(table1 != 0);

    delete_z(table1);
    delete_z(store);

    // don't do tidy
    g_config->tidy_ = false;
    impl            = new BerkeleyDBStore();
    store           = new DurableStore(impl);
    impl->init(g_config);

    CHECK(store->get_table(&table1, "test", 0) == 0);
    CHECK(table1 != 0);
    delete_z(table1);
    delete_z(store);

    // do tidy
    g_config->tidy_ = true;
    impl            = new BerkeleyDBStore();
    store           = new DurableStore(impl);
    impl->init(g_config);

    CHECK(store->get_table(&table1, "test", 0) == DS_NOTFOUND);
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
    ObjDurableTable*    objtable = 0;

    CHECK(store->get_table(&table1, "test", 0) == DS_NOTFOUND);
    CHECK(table1 == 0);
    
    CHECK(store->get_table(&table1, "test", DS_CREATE) == 0);
    CHECK(table1 != 0);
    delete_z(table1);
    
    CHECK(store->get_table(&table2, "test", DS_CREATE | DS_EXCL) 
          == DS_EXISTS);
    CHECK(table2 == 0);
    
    CHECK(store->get_table(&table2, "test", 0) == 0);
    CHECK(table2 != 0);
    delete_z(table2);

    CHECK(store->get_table(&table1, "test", DS_CREATE) == 0);
    CHECK(table1 != 0);
    delete_z(table1);

    CHECK(store->get_table(&objtable, "objtable", DS_CREATE | DS_EXCL) == 0);
    CHECK(objtable != 0);
    delete_z(objtable);
    delete_z(store);

    return 0;
}

DECLARE_TEST(TableDelete) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);
    
    StringDurableTable* table = 0;

    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);
    CHECK(table != 0);
    delete_z(table);

    CHECK(store->del_table("test") == 0);
    
    CHECK(store->get_table(&table, "test", 0) == DS_NOTFOUND);
    CHECK(table == 0);

    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);
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
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);

    IntShim    key(99);
    StringShim data("data");

    CHECK(table->size() == 0);
    CHECK(table->put(key, &data, 0) == DS_NOTFOUND);
    CHECK(table->put(key, &data, DS_CREATE) == 0);
    CHECK(table->size() == 1);
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
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);

    IntShim    key(99);
    IntShim    key2(101);
    StringShim data("data");
    StringShim* data2 = 0;

    CHECK(table->size() == 0);
    CHECK(table->put(key, &data, DS_CREATE | DS_EXCL) == 0);
    CHECK(table->size() == 1);
    
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
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);

    IntShim    key(99);
    IntShim    key2(101);
    StringShim data("data");
    StringShim* data2;

    CHECK(table->put(key, &data, DS_CREATE | DS_EXCL) == 0);
    CHECK(table->size() == 1);
    
    CHECK(table->get(key, &data2) == 0);
    CHECK(data2 != 0);
    CHECK(data2->value() == data.value());
    delete_z(data2);
    data2 = 0;
    
    CHECK(table->del(key) == 0);
    CHECK(table->size() == 0);

    CHECK(table->get(key, &data2) == DS_NOTFOUND);
    CHECK(data2 == 0);

    CHECK(table->del(key2) == DS_NOTFOUND);
    CHECK(table->size() == 0);
    
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
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);

    int num_objs = 100;
    
    for(int i=0; i<num_objs; ++i) {
        StaticStringBuffer<256> buf;
	buf.appendf("data%d", i);
        StringShim data(buf.c_str());
        
	CHECK(table->put(IntShim(i), &data, DS_CREATE | DS_EXCL) == 0);
    }
    CHECK((int)table->size() == num_objs);
    
    delete_z(table);
    delete_z(store);

    g_config->tidy_ = false;
    impl  = new BerkeleyDBStore();
    store = new DurableStore(impl);
    impl->init(g_config);
    
    CHECK(store->get_table(&table, "test", 0) == 0);

    PermutationArray pa(num_objs);

    for(int i=0; i<num_objs; ++i) {
        StaticStringBuffer<256> buf;
	StringShim* data = 0;

        IntShim key(pa.map(i));
        buf.appendf("data%d", pa.map(i));

	CHECK(table->get(key, &data) == 0);
        CHECK_EQUALSTR(buf.c_str(), data->value().c_str());
        delete_z(data);
    }
    CHECK((int)table->size() == num_objs);
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
     
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);
    
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

DECLARE_TEST(MultiType) {
    g_config->tidy_         = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);

    ObjDurableTable* table = 0;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);
    CHECK(table != 0);

    Obj *o1, *o2 = NULL;
    Foo foo;
    Bar bar;

    CHECK(table->put(StringShim("foo"), Foo::ID, &foo, DS_CREATE | DS_EXCL) == 0);
    CHECK(table->put(StringShim("bar"), Bar::ID, &bar, DS_CREATE | DS_EXCL) == 0);

    CHECK(table->get(StringShim("foo"), &o1) == 0);
    CHECK_EQUAL(o1->id_, Foo::ID);
    CHECK_EQUALSTR(o1->name(), "foo");
    CHECK_EQUALSTR(o1->static_name_.c_str(), "foo");
    CHECK(dynamic_cast<Foo*>(o1) != NULL);
    delete_z(o1);
    
    CHECK(table->get(StringShim("bar"), &o2) == 0);
    CHECK_EQUAL(o2->id_, Bar::ID);
    CHECK_EQUALSTR(o2->name(), "bar");
    CHECK_EQUALSTR(o2->static_name_.c_str(), "bar");
    CHECK(dynamic_cast<Bar*>(o2) != NULL);
    delete_z(o2);

    // Check mixed-up typecode and object
    CHECK(table->put(StringShim("foobar"), Bar::ID, &foo, DS_CREATE | DS_EXCL) == 0);
    CHECK(table->get(StringShim("foobar"), &o1) == 0);
    CHECK_EQUAL(o1->id_, Foo::ID);
    CHECK_EQUALSTR(o1->name(), "bar");
    CHECK_EQUALSTR(o1->static_name_.c_str(), "foo");
    CHECK(dynamic_cast<Foo*>(o1) == NULL);
    CHECK(dynamic_cast<Bar*>(o1) != NULL);
    delete_z(o1);
    
    delete_z(table);
    delete_z(store);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(NoTypeTable) {
    g_config->tidy_ = true;
    DurableStoreImpl* impl  = new BerkeleyDBStore();
    DurableStore*     store = new DurableStore(impl);
    impl->init(g_config);

    NonTypedDurableTable* table = 0;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL) == 0);
    CHECK(table != 0);

    StringShim serial_string("01234567890");
    IntShim    serial_int(42);
    
    CHECK(table->put(StringShim("foo"), 
                     &serial_string, 0));
    CHECK(table->put(StringShim("bar"),
                     &serial_int, 0));

    StringShim* str;
    IntShim*    i;
    
    CHECK(table->get(StringShim("foo"), &str));
    CHECK(table->get(StringShim("bar"), &i));

    CHECK_EQUALSTR(str->value().c_str(), "01234567890");
    CHECK_EQUAL(i->value(), 42);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(SingleTypeCache) {
    g_config->tidy_           = true;
    DurableStoreImpl*   impl  = new BerkeleyDBStore();
    DurableStore*       store = new DurableStore(impl);
    StringDurableCache* cache = new StringDurableCache(32);
    impl->init(g_config);

    StringDurableTable* table;

    impl->init(g_config);

    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, cache) == 0);

    StringShim* s;
    StringShim* s1 = new StringShim("test");
    StringShim* s2 = new StringShim("test test");
    StringShim* s3 = new StringShim("test test test");
    StringShim* s4 = new StringShim("test test test test");

    CHECK(table->put(IntShim(1), s1, 0) == DS_NOTFOUND);
    CHECK_EQUAL(cache->size(), 0);
    
    CHECK(table->put(IntShim(1), s1, DS_CREATE) == DS_OK);
    CHECK_EQUAL(cache->size(), 8);

    CHECK(table->put(IntShim(1), s1, 0) == DS_OK);
    CHECK_EQUAL(cache->size(), 8);
    
    CHECK(table->put(IntShim(1), s1, DS_EXCL) == DS_EXISTS);
    CHECK_EQUAL(cache->size(), 8);
    CHECK_EQUAL(cache->count(), 1);
    
    CHECK(table->put(IntShim(2), s2, DS_CREATE | DS_EXCL) == DS_OK);
    CHECK_EQUAL(cache->size(), 21);

    CHECK(table->put(IntShim(3), s3, DS_CREATE | DS_EXCL) == DS_OK);
    CHECK_EQUAL(cache->size(), 39);

    CHECK(table->put(IntShim(4), s4, DS_CREATE | DS_EXCL) == DS_OK);
    CHECK_EQUAL(cache->size(), 62);

    CHECK_EQUAL(cache->count(), 4);
    CHECK_EQUAL(cache->live(), 4);

    // make sure all four objects are in cache
    CHECK(table->get(IntShim(1), &s) == DS_OK);
    CHECK_EQUAL(cache->hits(),  1);
    CHECK(s == s1);

    CHECK(table->get(IntShim(2), &s) == DS_OK);
    CHECK_EQUAL(cache->hits(),  2);
    CHECK(s == s2);
    
    CHECK(table->get(IntShim(3), &s) == DS_OK);
    CHECK_EQUAL(cache->hits(),  3);
    CHECK(s == s3);
    
    CHECK(table->get(IntShim(4), &s) == DS_OK);
    CHECK_EQUAL(cache->hits(),  4);
    CHECK(s == s4);

    // now release them all in reverse order which will cause s3 and
    // s4 to be evicted
    CHECK(cache->release(IntShim(4), s4) == DS_OK);
    CHECK(cache->release(IntShim(3), s3) == DS_OK);
    CHECK(cache->release(IntShim(2), s2) == DS_OK);
    CHECK(cache->release(IntShim(1), s1) == DS_OK);

    CHECK_EQUAL(cache->size(), 21);
    CHECK_EQUAL(cache->count(), 2);
    CHECK_EQUAL(cache->live(), 0);

    // make sure s1 and s2 are still in-cache
    CHECK(table->get(IntShim(1), &s) == DS_OK);
    CHECK_EQUAL(cache->hits(),  5);
    CHECK(s == s1);

    CHECK(table->get(IntShim(2), &s) == DS_OK);
    CHECK_EQUAL(cache->hits(),  6);
    CHECK(s == s2);

    // release s1 and s2, then try to get s3 and s4 which should
    // create new objects, leaving only s4 in cache when we're done
    CHECK(cache->release(IntShim(1), s1) == DS_OK);
    CHECK(cache->release(IntShim(2), s2) == DS_OK);

    CHECK(table->get(IntShim(3), &s) == DS_OK);
    CHECK_EQUAL(cache->hits(),  6);
    CHECK_EQUAL(cache->misses(), 1);
    CHECK_EQUALSTR(s->value().c_str(), "test test test");
    CHECK_EQUAL(cache->evictions(), 3);
    CHECK(cache->release(IntShim(3), s) == DS_OK);
    CHECK_EQUAL(cache->count(), 2);
    CHECK_EQUAL(cache->live(), 0);
    
    CHECK(table->get(IntShim(4), &s) == DS_OK);
    CHECK_EQUAL(cache->hits(),  6);
    CHECK_EQUAL(cache->misses(), 2);
    CHECK_EQUALSTR(s->value().c_str(), "test test test test");
    CHECK_EQUAL(cache->evictions(), 5);
    CHECK(cache->release(IntShim(4), s) == DS_OK);
    CHECK_EQUAL(cache->count(), 1);
    CHECK_EQUAL(cache->live(), 0);

    // delete an object out of cache, a live object in cache, and a
    // dead object in cache
    CHECK(table->get(IntShim(1), &s) == DS_OK);
    CHECK_EQUAL(cache->count(), 2);
    CHECK_EQUAL(cache->live(), 1);
    
    CHECK(table->del(IntShim(1)) == DS_OK);
    CHECK(table->get(IntShim(1), &s) == DS_NOTFOUND);
    CHECK(table->del(IntShim(2)) == DS_OK);
    CHECK(table->get(IntShim(2), &s) == DS_NOTFOUND);
    CHECK(table->del(IntShim(3)) == DS_OK);
    CHECK(table->get(IntShim(3), &s) == DS_NOTFOUND);
    CHECK(table->del(IntShim(4)) == DS_OK);
    CHECK(table->get(IntShim(4), &s) == DS_NOTFOUND);
    
    CHECK_EQUAL(cache->count(), 0);
    CHECK_EQUAL(cache->live(),  0);
    CHECK_EQUAL(cache->size(),  0);
    
    delete_z(table);
    delete_z(store);
    delete_z(cache);
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(MultiTypeCache) {
    g_config->tidy_           = true;
    DurableStoreImpl*   impl  = new BerkeleyDBStore();
    DurableStore*       store = new DurableStore(impl);
    ObjDurableCache*    cache = new ObjDurableCache(36);
    impl->init(g_config);

    ObjDurableTable* table = 0;
    CHECK(store->get_table(&table, "test", DS_CREATE | DS_EXCL, cache) == 0);

    Obj* o;
    Foo* f1 = new Foo("test");
    Foo* f2 = new Foo("test2");
    Bar* b1 = new Bar("test test");
    Bar* b2 = new Bar("test test test");

    CHECK(table->put(IntShim(1), Foo::ID, f1, 0) == DS_NOTFOUND);
    CHECK_EQUAL(cache->size(), 0);
    
    CHECK(table->put(IntShim(1), Foo::ID, f1, DS_CREATE) == DS_OK);
    CHECK_EQUAL(cache->size(), 12);

    CHECK(table->put(IntShim(1), Foo::ID, f1, 0) == DS_OK);
    CHECK_EQUAL(cache->size(), 12);
    
    CHECK(table->put(IntShim(1), Foo::ID, f1, DS_EXCL) == DS_EXISTS);
    CHECK_EQUAL(cache->size(), 12);
    CHECK_EQUAL(cache->count(), 1);
    
    CHECK(table->put(IntShim(2), Foo::ID, f2, DS_CREATE | DS_EXCL) == DS_OK);
    CHECK_EQUAL(cache->size(), 25);

    CHECK(table->put(IntShim(3), Bar::ID, b1, DS_CREATE | DS_EXCL) == DS_OK);
    CHECK_EQUAL(cache->size(), 42);

    CHECK(table->put(IntShim(4), Bar::ID, b2, DS_CREATE | DS_EXCL) == DS_OK);
    CHECK_EQUAL(cache->size(), 64);

    CHECK_EQUAL(cache->count(), 4);
    CHECK_EQUAL(cache->live(), 4);

    // make sure all four objects are in cache
    CHECK(table->get(IntShim(1), &o) == DS_OK);
    CHECK_EQUAL(cache->hits(),  1);
    CHECK(o == f1);

    CHECK(table->get(IntShim(2), &o) == DS_OK);
    CHECK_EQUAL(cache->hits(),  2);
    CHECK(o == f2);
    
    CHECK(table->get(IntShim(3), &o) == DS_OK);
    CHECK_EQUAL(cache->hits(),  3);
    CHECK(o == b1);
    
    CHECK(table->get(IntShim(4), &o) == DS_OK);
    CHECK_EQUAL(cache->hits(),  4);
    CHECK(o == b2);

    // now release them all in reverse order which will cause b1 and
    // b2 to be evicted
    CHECK(cache->release(IntShim(4), b2) == DS_OK);
    CHECK(cache->release(IntShim(3), b1) == DS_OK);
    CHECK(cache->release(IntShim(2), f2) == DS_OK);
    CHECK(cache->release(IntShim(1), f1) == DS_OK);

    CHECK_EQUAL(cache->size(), 25);
    CHECK_EQUAL(cache->count(), 2);
    CHECK_EQUAL(cache->live(), 0);

    // make sure f1 and f2 are still in-cache
    CHECK(table->get(IntShim(1), &o) == DS_OK);
    CHECK_EQUAL(cache->hits(),  5);
    CHECK(o == f1);

    CHECK(table->get(IntShim(2), &o) == DS_OK);
    CHECK_EQUAL(cache->hits(),  6);
    CHECK(o == f2);

    // release f1 and f2, then try to get b1 and b2 which should
    // create new objects, leaving only b1 in cache when we're done
    CHECK(cache->release(IntShim(1), f1) == DS_OK);
    CHECK(cache->release(IntShim(2), f2) == DS_OK);

    CHECK(table->get(IntShim(3), &o) == DS_OK);
    CHECK_EQUAL(cache->hits(),  6);
    CHECK_EQUAL(cache->misses(), 1);
    CHECK_EQUALSTR(o->name(), "bar");
    CHECK_EQUALSTR(o->static_name_.c_str(), "test test");
    CHECK_EQUAL(cache->evictions(), 3);
    CHECK(cache->release(IntShim(3), o) == DS_OK);
    CHECK_EQUAL(cache->count(), 2);
    CHECK_EQUAL(cache->live(), 0);
    
    CHECK(table->get(IntShim(4), &o) == DS_OK);
    CHECK_EQUAL(cache->hits(),  6);
    CHECK_EQUAL(cache->misses(), 2);
    CHECK_EQUALSTR(o->name(), "bar");
    CHECK_EQUALSTR(o->static_name_.c_str(), "test test test");
    CHECK_EQUAL(cache->evictions(), 5);
    CHECK(cache->release(IntShim(4), o) == DS_OK);
    CHECK_EQUAL(cache->count(), 1);
    CHECK_EQUAL(cache->live(), 0);

    // delete an object out of cache, a live object in cache, and a
    // dead object in cache
    CHECK(table->get(IntShim(1), &o) == DS_OK);
    CHECK_EQUAL(cache->count(), 2);
    CHECK_EQUAL(cache->live(), 1);
    
    CHECK(table->del(IntShim(1)) == DS_OK);
    CHECK(table->get(IntShim(1), &o) == DS_NOTFOUND);
    CHECK(table->del(IntShim(2)) == DS_OK);
    CHECK(table->get(IntShim(2), &o) == DS_NOTFOUND);
    CHECK(table->del(IntShim(3)) == DS_OK);
    CHECK(table->get(IntShim(3), &o) == DS_NOTFOUND);
    CHECK(table->del(IntShim(4)) == DS_OK);
    CHECK(table->get(IntShim(4), &o) == DS_NOTFOUND);
    
    CHECK_EQUAL(cache->count(), 0);
    CHECK_EQUAL(cache->live(),  0);
    CHECK_EQUAL(cache->size(),  0);
    
    delete_z(table);
    delete_z(store);
    delete_z(cache);
    
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
    ADD_TEST(NoTypeTable);
    ADD_TEST(MultiType);
    ADD_TEST(MultiTypeCache);
}

DECLARE_TEST_FILE(BerkleyDBTester, "berkeley db test");
