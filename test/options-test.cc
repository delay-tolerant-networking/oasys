
#include <string>
#include <stdio.h>

#include <debug/Log.h>
#include <util/Options.h>

using namespace oasys;

int
main(int argc, char *const* argv)
{
    bool test = 0;
    int port = 10;
    int xyz = 50;
    std::string name("name");

    Log::init();

    new BoolOpt('t', "test", &test, "test flag");
    new IntOpt('P', "port", &port, "<port>", "listen port");
    new IntOpt(0,   "xyz",  &xyz,  "<val>", "x y z");
    new StringOpt('N', 0, &name, "<name>", "app name");

    Options::getopt("testapp", argc, argv);

    printf ("test: %d\n", test); 
    printf ("port: %d\n", port);
    printf ("name: %s\n", name.c_str());
    printf ("xyz: %d\n", xyz);
}
