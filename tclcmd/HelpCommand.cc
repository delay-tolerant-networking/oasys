
#include "HelpCommand.h"

#include "memory/Memory.h"
#include "util/StringBuffer.h"

HelpCommand::HelpCommand()
    : TclCommand("help")
{
}

int
HelpCommand::exec(int argc, const char** argv, Tcl_Interp* interp)
{
    const TclCommandList *cmdlist = NULL;
    TclCommandList::const_iterator iter;
    
    cmdlist = TclCommandInterp::instance()->commands();

    if (argc == 1) {
        StringBuffer buf;
        buf.append("for help on a particular command, type help <cmd>...\n");
        buf.append("registered commands: ");
                   
        for (iter = cmdlist->begin(); iter != cmdlist->end(); iter++) {
            buf.appendf("%s ", (*iter)->name());
        }

        set_result(buf.c_str());

        return TCL_OK;
    } else if (argc == 2) {
        for (iter = cmdlist->begin(); iter != cmdlist->end(); iter++) {
            if (!strcmp((*iter)->name(), argv[1])) {
                if ((*iter)->help_string()[0] != '\0')
                    resultf("%s", ((*iter)->help_string()));
                else
                    resultf("(no help, sorry)");
                
                return TCL_OK;
            }
        }

        resultf("no registered command '%s'", argv[1]);
        return TCL_ERROR;
        
    } else {
        wrong_num_args(argc, argv, 2, 3, 3);
        return TCL_ERROR;
    }
}

const char*
HelpCommand::help_string()
{
    return("help <cmd?>");
}
