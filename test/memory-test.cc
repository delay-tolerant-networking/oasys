#include <list>

#include <debug/Log.h>
#include <memory/Memory.h>

class Foo_1 {};
std::list<Foo_1*> lFoos;

void
Alloc_1()
{
    lFoos.push_back(new Foo_1());
}

void
Alloc_2()
{
    lFoos.push_back(new Foo_1());
}

void Alloc_3()
{
    lFoos.push_back(new Foo_1());

    Alloc_1();
    Alloc_2();
}

void
delete_all_foo()
{
    while(lFoos.size() > 0)
    {
	Foo_1* obj = lFoos.front();
	delete obj;

	lFoos.pop_front();
    }
}

int
main(int argc, char* argv[])
{
    Log::init(1, LOG_DEBUG, "memory-test.debug");
    DbgMemInfo::init();
    
    // Create 11 Foo_1 object in different places
    Alloc_3();
    Alloc_3();
    Alloc_3();
    Alloc_2();
    Alloc_1();

    DbgMemInfo::dump();

    // Delete all Foo_1 objects
    delete_all_foo();
    DbgMemInfo::dump();
}
