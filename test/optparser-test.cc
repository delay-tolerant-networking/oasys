
#include <string>
#include <stdio.h>

#include <debug/Log.h>
#include <util/OptParser.h>

using namespace oasys;

bool test = 0;
bool test_set = 0;
int port = 10;
int xyz = 50;
std::string name("name");

OptParser p;

void
testfn(const char* teststr, bool expected)
{
    const char* invalid;
    bool valid = p.parse(teststr, &invalid);
    
    if (expected && !valid) {
        printf("TEST ERROR '%s': arg %s invalid\n", teststr, invalid);
        
    } else if (!expected && valid) {
        printf("TEST ERROR '%s': unexpectedly valid \n", teststr);
        
    } else if (expected && valid) {
        printf("TEST OK    '%s'\n", teststr);

    } else if (!expected && !valid) {
        printf("TEST OK    '%s': invalid '%s'\n", teststr, invalid);
    }
}

int
main(int argc, char *const* argv)
{
    Log::init();
    
    p.addopt(new BoolOpt("test", &test, "test flag", &test_set));
    p.addopt(new IntOpt("port", &port, "<port>", "listen port"));
    p.addopt(new IntOpt("xyz",  &xyz,  "<val>", "x y z"));
    p.addopt(new StringOpt("name", &name, "<name>", "app name"));

    const char* invalid;
    bool valid = p.parse(argc-1, argv+1, &invalid);

    if (valid) {
        printf("TEST OK    argv\n");
    } else {
        printf("TEST ERROR argv: arg '%s' invalid\n", invalid);
        exit(1);
    }

    printf("  test: %d\n", test); 
    printf("  test_set: %d\n", test_set);
    printf("  port: %d\n", port);
    printf("  name: %s\n", name.c_str());
    printf("  xyz: %d\n", xyz);

    testfn("test port=100 name=mike xyz=10", true);
    ASSERT(test == true);
    ASSERT(test_set == true);
    ASSERT(port == 100);
    ASSERT(name.compare("mike") == 0);
    ASSERT(xyz == 10);

    testfn("test=false", true);
    ASSERT(test == false);
    ASSERT(test_set == true);
    
    testfn("test=F", true);
    ASSERT(test == false);
    ASSERT(test_set == true);
    
    testfn("test=0", true);
    ASSERT(test == false);
    ASSERT(test_set == true);
    
    testfn("test=TRUE", true);
    ASSERT(test == true);
    ASSERT(test_set == true);

    testfn("test=T", true);
    ASSERT(test == true);
    ASSERT(test_set == true);
    
    testfn("test=1", true);
    ASSERT(test == true);
    ASSERT(test_set == true);
    
    testfn("test=abc", false);
    
    testfn("", true);
    ASSERT(test == true);
    ASSERT(test_set == true);
    ASSERT(port == 100);
    ASSERT(name.compare("mike") == 0);
    ASSERT(xyz == 10);

    testfn("port", false);
    testfn("port=", false);
    testfn("port=foo", false);
    testfn("test=", false);
    testfn("test=foo", false);
    
}
