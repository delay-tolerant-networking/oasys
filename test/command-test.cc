
#include <iostream>

#include "debug/Log.h"
#include "tclcmd/TclCommand.h"

using namespace std;
using namespace oasys;

class TestModule : public TclCommand {
public:
    TestModule() : TclCommand("test") {
        bind_i("val", &val_);
        bind_s("buf", &buf_);
    }

    virtual int exec(int argc, const char** args, Tcl_Interp* interp)
    {
        resultf("test of var args %s %d", "\"string\"", 66);
        return TCL_ERROR;
    }
    
    int  val_;
    std::string buf_;
};

using namespace oasys;

int
main(int argc, char* argv[])
{
    Log::init(LOG_DEBUG);
    TclCommandInterp::init(argv[0]);
    TclCommandInterp* interp = TclCommandInterp::instance();

    TclCommand* mod = new TclCommand("mod1");
    TestModule* mod2 = new TestModule();

    (void)mod;
    
    interp->reg(mod);
    interp->reg(mod2);

    interp->exec_command("test set val 2");
    cout << "val=" << mod2->val_ << endl;

    interp->exec_command("puts \"The value of val"
                         "=[test set val]\"");
    
    interp->exec_command("test set buf \"hello\"");
    
    cout << "buf=" << mod2->buf_ << endl;

    interp->exec_command("test error");
 
    cout << "Done" << endl;
}
