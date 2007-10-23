/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#  include <oasys-config.h>
#endif

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
