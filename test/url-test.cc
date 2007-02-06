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

#include "util/UnitTest.h"
#include "util/URL.h"

using namespace oasys;

#define TEST_URL(url, err, proto, host, port, path, params)             \
{                                                                       \
    int                expected_err = err;                              \
    const std::string& expected_proto = proto;                          \
    const std::string& expected_host = host;                            \
    u_int16_t          expected_port = port;                            \
    const std::string& expected_path = path;                            \
    const std::vector<std::string>& expected_params = params;           \
                                                                        \
    URL u;                                                              \
    URL u2;                                                             \
                                                                        \
    CHECK_EQUAL(u.parse(url), expected_err);                            \
                                                                        \
    if (expected_err == URLPARSE_OK) {                                  \
        CHECK_EQUALSTR(u.proto_.c_str(), expected_proto.c_str());       \
        CHECK_EQUALSTR(u.host_.c_str(), expected_host.c_str());         \
        CHECK_EQUAL(u.port_, expected_port);                            \
        CHECK_EQUALSTR(u.path_.c_str(), expected_path.c_str());         \
        CHECK_EQUAL(u.params_.size(), expected_params.size());          \
        for (size_t i = 0; i < u.params_.size(); ++i) {                 \
            CHECK_EQUALSTR(u.params_[i].c_str(), expected_params[i].c_str()); \
        }                                                               \
                                                                        \
        DO(u2.format(u.proto_, u.host_, u.port_, u.path_, u.params_));  \
        CHECK_EQUALSTR(u.c_str(), u2.c_str());                          \
    }                                                                   \
}


std::vector<std::string> g_params;
std::vector<std::string>&
params(const char* p1 = 0,
       const char* p2 = 0,
       const char* p3 = 0,
       const char* p4 = 0,
       const char* p5 = 0,
       const char* p6 = 0)
{
    g_params.clear();
    if (p1) { g_params.push_back(p1); }
    if (p2) { g_params.push_back(p2); }
    if (p3) { g_params.push_back(p3); }
    if (p4) { g_params.push_back(p4); }
    if (p5) { g_params.push_back(p5); }
    if (p6) { g_params.push_back(p6); }
    return g_params;
}

DECLARE_TEST(Basic) {
    TEST_URL("http://my.yahoo.com", URLPARSE_OK,
             "http", "my.yahoo.com", 0, "", params());
    TEST_URL("http://my.yahoo.com/", URLPARSE_OK,
             "http", "my.yahoo.com", 0, "", params());
    TEST_URL("http://10.0.0.1", URLPARSE_OK,
             "http", "10.0.0.1", 0, "", params());
    TEST_URL("http://my.yahoo.com:100", URLPARSE_OK,
             "http", "my.yahoo.com", 100, "", params());
    TEST_URL("http://my.yahoo.com/foo", URLPARSE_OK,
             "http", "my.yahoo.com", 0, "foo", params());
    TEST_URL("http://my.yahoo.com:100/foo", URLPARSE_OK,
             "http", "my.yahoo.com", 100, "foo", params());
    TEST_URL("http://my.yahoo.com:100/foo/bar", URLPARSE_OK,
             "http", "my.yahoo.com", 100, "foo/bar", params());

    return UNIT_TEST_PASSED;
}


DECLARE_TEST(Params) {
    TEST_URL("http://www.google.com/search?query=something", URLPARSE_OK,
             "http", "www.google.com", 0, "search", params("query", "something"));
    TEST_URL("http://www.google.com/search?query=something&a=b&c=d", URLPARSE_OK,
             "http", "www.google.com", 0, "search",
             params("query", "something", "a", "b", "c", "d"));
    TEST_URL("http://www.google.com/?query=something", URLPARSE_OK,
             "http", "www.google.com", 0, "", params("query", "something"));
    TEST_URL("http://www.google.com/search?query=&foo=", URLPARSE_OK,
             "http", "www.google.com", 0, "search", params("query", "", "foo", ""));
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(BadParse) {
    TEST_URL("http://my.yahoo.com:100x/foo", URLPARSE_BADPORT,
             "", "", 0, "", params());
    TEST_URL("http://my.yahoo.com:70000/foo", 	URLPARSE_BADPORT,
             "", "", 0, "", params());
    TEST_URL("http:/one/slash", URLPARSE_BADPROTO,
             "", "", 0, "", params());
    TEST_URL("http://", URLPARSE_NOHOST,
             "", "", 0, "", params());

    TEST_URL("http://www.google.com/?query", URLPARSE_NOPARAMVAL,
             "", "", 0, "", params());
        
    return UNIT_TEST_PASSED;
}

DECLARE_TESTER(URLTest) {
    ADD_TEST(Basic);
    ADD_TEST(Params);
    ADD_TEST(BadParse);
}

DECLARE_TEST_FILE(URLTest, "url test");

