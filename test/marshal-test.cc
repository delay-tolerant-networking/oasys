#include <iostream>
#include <debug/DebugUtils.h>
#include <serialize/MarshalSerialize.h>

using namespace std;
using namespace oasys;

class OneOfEach : public SerializableObject {
public:
    OneOfEach() : a(200), b(-100), c(0x77), d(0xbaddf00d), e(56789), u(INT_MAX), s1("hello") {
        memset(s2, 0, sizeof(s2));
        strcpy(s2, "Zanzibar");
    }
    OneOfEach(bool fool) : a(0), b(0), c(0), d(0), e(0), u(0), s1("") {
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
    }

    bool equals(const OneOfEach& other) {
        return (a == other.a &&
                b == other.b &&
                c == other.c &&
                d == other.d &&
                e == other.e &&
                u == other.u &&
                s1.compare(other.s1) == 0 &&
                !memcmp(s2, other.s2, sizeof(s2)));
    }

private:
    int32_t    a, b, c, d;
    short  e;
    u_int32_t u;
    string s1;
    char   s2[32];
};

void
print_hex(u_char* buf, int len)
{
    // print 8 groups 2 in a line
    for(int i=0;i<len;i++) {
        if(i%2 == 0) {
            printf(" ");
        }
        
        if(i%4 == 0) {
            printf(" ");
        }
        
        if(i%8 == 0) {
            printf("\n");
        }

        u_int val = (u_int)buf[i];
        printf("%02x", val);
    }
    printf("\n");
}

int
main()
{
    Log::init(LOG_DEBUG);
    OneOfEach o1, o2(false);

#define LEN 256
    u_char buf[LEN];
    
    memset(buf, 0, sizeof(char) * LEN);
    Marshal v(buf, LEN);
    v.logpath("/marshal-test");

    if (v.action(&o1) != 0) {
        cerr << "Can't marshal!" << endl;
    }
    
    print_hex((u_char*)buf, LEN);

    Unmarshal uv(buf, LEN);
    uv.logpath("/marshal-test");

    if(uv.action(&o2) != 0) {
        cerr << "Can't unmarshal!" << endl;
    }

    if (! o1.equals(o2)) {
        cerr << "Unmarshal inconsistent!!" << endl;
    }
    
//     MarshalPrinter p(buf, LEN, false);
//     if(p.process(&o2) != 0) {
//         cerr << "Can't print!" << endl;
//     }

//     cout << endl;
    

    MarshalSize sz;
    sz.action(&o1);
    cout << "o1 size =" << sz.size() << endl;
    
    sz.action(&o2);
    cout << "o2 size =" << sz.size() << endl;
}
