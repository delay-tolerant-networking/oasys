#ifndef _HELP_COMMAND_H_
#define _HELP_COMMAND_H_

#include "TclCommand.h"

/**
 * The "help" command.
 */
class HelpCommand : public TclCommand {
public:
    HelpCommand();
    
    /**
     * Virtual from CommandModule.
     */
    virtual int exec(int argc, const char** argv, Tcl_Interp* interp);
    virtual const char* help_string();
};

#endif /* _HELP_COMMAND_H_ */
