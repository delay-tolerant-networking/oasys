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

#include <stdio.h>
#include <string>
#include "util/URL.h"

using namespace std;
using namespace oasys;

void
test(const string url, int expected_ok, const string expected_host,
     u_int16_t expected_port, const string expected_path)
{
    URL u;
    URL u2;

    string expected_proto("http");

    int ok = u.parse(url);

    if (ok != expected_ok) {
        printf("PARSE ERROR: %s -> expected ok %d got %d\n",
               url.c_str(), expected_ok, ok);
        return;
    }

    if (ok == URLPARSE_OK) {

#define checkstr(what)                                                  \
        if ( u.what##_ != expected_ ## what )                           \
        {                                                               \
            printf("PARSE ERROR: %s -> expected %s %s got %s\n",        \
                   url.c_str(), #what, expected_ ## what.c_str(),       \
                   u.what##_ .c_str());                                 \
            return;                                                     \
        }

        checkstr(proto);
        checkstr(host);
        checkstr(path);

        if (u.port_ != expected_port) {
            printf("PARSE ERROR: %s -> expected port %d got %d\n",
                   url.c_str(), expected_port, u.port_);
            return;
        }
    }

    // now test format. note that u may end up being shorter since u2
    // will always have a trailing slash
    if (ok == URLPARSE_OK)
    {
        u2.format(u.proto_, u.host_, u.port_, u.path_);
        if (u.url_.compare(0, u.length(), u2.url_, 0, u.length()) != 0)
        {
            printf("FORMAT ERROR: %s != %s\n", u.c_str(), u2.c_str());
        }
    }
              
    printf("TEST OK: %s\n", url.c_str());
}

int
main()
{
    URL u;
    
    printf("Testing url parsing / formatting...\n");
    
    test("http://my.yahoo.com", 		0, "my.yahoo.com", 0, "");
    test("http://my.yahoo.com/", 		0, "my.yahoo.com", 0, "");
    test("http://10.0.0.1", 			0, "10.0.0.1",     0, "");
    test("http://my.yahoo.com:100", 		0, "my.yahoo.com", 100, "");
    test("http://my.yahoo.com/foo", 		0, "my.yahoo.com", 0, "foo");
    test("http://my.yahoo.com:100/foo",		0, "my.yahoo.com", 100, "foo");
    test("http://my.yahoo.com:100/foo/bar", 	0, "my.yahoo.com", 100, "foo/bar");

    test("http://my.yahoo.com:100x/foo", 	URLPARSE_BADPORT, "", 0, "");
    test("http://my.yahoo.com:70000/foo", 	URLPARSE_BADPORT, "", 0, "");
    test("http:/one/slash", 			URLPARSE_BADPROTO, "", 0, "");
    test("http://", 				URLPARSE_NOHOST, "", 0, "");
}

