#include <iostream>
#include <debug/Debug.h>
#include "serialize/MarshalSerialize.h"

using namespace std;
using namespace oasys;

#define CRC Serialize::USE_CRC

class OneOfEach : public oasys::SerializableObject {
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
    }
    OneOfEach(bool fool) : a(0), b(0), c(0), d(0), e(0), u(0), s1("") {
        memset(s2, 0, sizeof(s2));
    }
    
    virtual ~OneOfEach() {}
    
    void serialize(oasys::SerializeAction* act) {
        act->process("a", &a);
        act->process("b", &b);
        act->process("c", &c);
        act->process("d", &d);
        act->process("e", &e);
        act->process("u", &u);
        act->process("s1", &s1);
        act->process("s2", s2, sizeof(s2));
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
    Log::init("-", oasys::LOG_DEBUG);
    OneOfEach o1, o2(false);

#define LEN 256
    u_char buf[LEN];
    memset(buf, 0, sizeof(char) * LEN);

    oasys::MarshalSize sizer(Serialize::CONTEXT_NETWORK, true);
    
    sizer.logpath("/marshal-test");
    sizer.action(&o1);
    cerr << "Size = " << sizer.size() << endl;

    oasys::Marshal v(Serialize::CONTEXT_NETWORK, buf, LEN, CRC);
    v.logpath("/marshal-test");
    v.action(&o1);
    
    print_hex((u_char*)buf, LEN);

    oasys::Unmarshal uv(Serialize::CONTEXT_NETWORK, buf, sizer.size(), CRC);
    uv.logpath("/marshal-test");
    uv.action(&o2);

    if (! o1.equals(o2)) {
        cerr << "Unmarshal inconsistent!!" << endl;
    }

#if 0    
    oasys::MarshalPrinter p(buf, LEN, false);
    if(p.process(&o2, oasys::Serialize::CONTEXT_LOCAL) != 0) {
        cerr << "Can't print!" << endl;
    }

    cout << endl;
    
    cout << "o1 size =" << v.length() << endl;

    oasys::MarshalSize sz;
    sz.process(&o1, oasys::Serialize::CONTEXT_LOCAL);
    
    cout << "o2 size =" << sz.length() << endl;
#endif // if 0
}
