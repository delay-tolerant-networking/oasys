
#include "LogCommand.h"

#include "memory/Memory.h"
#include "debug/Log.h"

LogCommand::LogCommand()
    : TclCommand("log") {}

const char*
LogCommand::help_string()
{
    return("log <path> <level> <string>");
}

int
LogCommand::exec(int argc, const char** argv, Tcl_Interp* interp)
{
    // log prefix <string>
    if (argc == 3 && !strcmp(argv[1], "prefix")) {
        Log::instance()->set_prefix(argv[2]);
        ::logf("/log", LOG_DEBUG, "set logging prefix to '%s'", argv[2]);
        return TCL_OK;
    }
    
    // log path level string
    if (argc != 4) {
        wrong_num_args(argc, argv, 1, 4, 4);
        return TCL_ERROR;
    }

    log_level_t level = str2level(argv[2]);
    if (level == LOG_INVALID) {
        resultf("invalid log level %s", argv[2]);
        return TCL_ERROR;
    }
    
    ::logf(argv[1], level, argv[3]);

    return TCL_OK;
}
