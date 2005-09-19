#include <iostream>
#include <debug/DebugUtils.h>
#include <serialize/MarshalSerialize.h>
#include <util/UnitTest.h>

using namespace std;
using namespace oasys;

class OneOfEach : public SerializableObject {
public:
    OneOfEach() :
        a(200),
        b(-100),
        c(0x77),
        d(0xbaddf00d),
        e(56789),
        u(INT_MAX),
        s1("hello")
    {
        memset(s2, 0, sizeof(s2));
        strcpy(s2, "Zanzibar");

        const_len = 10;
        const_buf = (u_char*)malloc(10);
        for (int i = 0; i < 10; ++i) {
            const_buf[i] = 'a' + i;
        }

        nullterm_buf = (u_char*)strdup("Testing one two");
        nullterm_len = strlen((char*)nullterm_buf) + 1;

        null_buf = 0;
        null_len  = 0;
    }
    
    OneOfEach(const Builder&) :
        a(0),
        b(0),
        c(0),
        d(0),
        e(0),
        u(0),
        s1(""),
        const_len(0x99),
        nullterm_len(0x99),
        null_len(0x99),
        const_buf((u_char*)0xbaddf00d),
        nullterm_buf((u_char*)0xbaddf00d),
        null_buf((u_char*)0xbaddf00d)
    {
        memset(s2, 0, sizeof(s2));
    }
    
    virtual ~OneOfEach() {}
    
    void serialize(SerializeAction* action) {
        action->process("a", &a);
        action->process("b", &b);
        action->process("c", &c);
        action->process("d", &d);
        action->process("e", &e);
        action->process("u", &u);
        action->process("s1", &s1);
        action->process("s2", s2, sizeof(s2));
        action->process("const_buf", &const_buf, &const_len, Serialize::ALLOC_MEM);
        action->process("nullterm_buf", &nullterm_buf, &nullterm_len, Serialize::NULL_TERMINATED);
        action->process("null_buf", &null_buf, &null_len, Serialize::ALLOC_MEM);
    }

    int32_t   a, b, c, d;
    short     e;
    u_int32_t u;
    string    s1;
    char      s2[32];

    size_t    const_len, nullterm_len, null_len;
    u_char    *const_buf, *nullterm_buf, *null_buf;
};

Builder b;
OneOfEach o1;
OneOfEach o2(b);

#define LEN 512
u_char buf[LEN];

DECLARE_TEST(Marshal) {
    memset(buf, 0, sizeof(char) * LEN);
    Marshal v(buf, LEN);
    v.logpath("/marshal-test");
    CHECK(v.action(&o1) == 0);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Unmarshal) {
    Unmarshal uv(buf, LEN);
    uv.logpath("/marshal-test");
    CHECK(uv.action(&o2) == 0);

    return UNIT_TEST_PASSED;
}

DECLARE_TEST(MarshalSize) {

    MarshalSize sz1;
    sz1.action(&o1);

    MarshalSize sz2;
    sz2.action(&o2);

    CHECK_EQUAL(sz1.size(), sz2.size());
    
    return UNIT_TEST_PASSED;
}

DECLARE_TEST(Compare) {
    CHECK_EQUAL(o1.a, o2.a);
    CHECK_EQUAL(o1.b, o2.b);
    CHECK_EQUAL(o1.c, o2.c);
    CHECK_EQUAL(o1.d, o2.d);
    CHECK_EQUAL(o1.e, o2.e);
    CHECK_EQUAL(o1.u, o2.u);
    CHECK_EQUALSTR(o1.s1.c_str(), o2.s1.c_str());
    CHECK_EQUALSTRN(o1.s2, o2.s2, 32);
    
    CHECK_EQUAL(o1.const_len, o2.const_len);
    CHECK_EQUALSTRN(o1.const_buf, o2.const_buf, o1.const_len);

    CHECK_EQUAL(o1.nullterm_len, o2.nullterm_len);
    CHECK_EQUALSTRN(o1.nullterm_buf, o2.nullterm_buf, o1.nullterm_len);

    CHECK_EQUAL(o1.null_len, 0);
    CHECK_EQUAL((int)o2.null_buf, 0);
    CHECK_EQUAL(o1.null_len, 0);
    CHECK_EQUAL((int)o2.null_buf, 0);

    return UNIT_TEST_PASSED;
}
    
DECLARE_TESTER(MarshalTester) {
    ADD_TEST(Marshal);
    ADD_TEST(Unmarshal);
    ADD_TEST(MarshalSize);
    ADD_TEST(Compare);
}

DECLARE_TEST_FILE(MarshalTester, "marshal unit test");
