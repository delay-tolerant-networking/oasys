
#include <string>
#include <stdio.h>

#include <debug/Log.h>
#include <util/Getopt.h>

using namespace oasys;

int
main(int argc, char *const* argv)
{
    bool test = 0;
    bool test_set = 0;
    int port = 10;
    int xyz = 50;
    double f = 10.5;
    std::string name("name");

    Log::init();

    Getopt::addopt(new BoolOpt('t', "test", &test, "test flag", &test_set));
    Getopt::addopt(new IntOpt('P', "port", &port, "<port>", "listen port"));
    Getopt::addopt(new IntOpt(0,   "xyz",  &xyz,  "<val>", "x y z"));
    Getopt::addopt(new DoubleOpt(0,   "f",  &f,  "<val>", "f"));
    Getopt::addopt(new StringOpt('N', 0, &name, "<name>", "app name"));

    Getopt::getopt("testapp", argc, argv);

    printf("test: %d\n", test); 
    printf("test_set: %d\n", test_set);
    printf("port: %d\n", port);
    printf("name: %s\n", name.c_str());
    printf("xyz: %d\n", xyz);
}
