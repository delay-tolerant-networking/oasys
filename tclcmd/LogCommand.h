#ifndef _LOG_COMMAND_H_
#define _LOG_COMMAND_H_

#include "TclCommand.h"

/**
 * Class to export the logging system to tcl scripts.
 */
class LogCommand : public TclCommand {
public:
    LogCommand();
    
    /**
     * Virtual from CommandModule.
     */
    virtual int exec(int argc, const char** argv, Tcl_Interp* interp);
    virtual const char* help_string();
};


#endif //_LOG_COMMAND_H_
