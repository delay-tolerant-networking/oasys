#include "TclCommand.h"
#include "HelpCommand.h"
#include "LogCommand.h"

#include "debug/Debug.h"
#include "io/NetUtils.h"
#include "thread/SpinLock.h"
#include "util/StringBuffer.h"

/******************************************************************************
 *
 * TclCommandInterp
 *
 *****************************************************************************/
// static variables
TclCommandInterp* TclCommandInterp::instance_;
TclCommandList* TclCommandInterp::auto_reg_ = NULL;

#include "command-init-tcl.c"

TclCommandInterp::TclCommandInterp()
    : Logger("/command")
{
}

int
TclCommandInterp::do_init(char* argv0, bool no_default_cmds)
{
    interp_ = Tcl_CreateInterp();
    
    lock_ = new SpinLock();

    // run Tcl_Init to set up the local tcl package path
    if (Tcl_Init(interp_) != TCL_OK) {
        log_err("error in Tcl_Init: \"%s\"", interp_->result);
        return TCL_ERROR;
    }

    // for some reason, this needs to be called to set up the various
    // locale strings and get things like the "ascii" encoding defined
    // for a file channel
    Tcl_FindExecutable(argv0);
    
    // do auto registration of commands (if any)
    if (auto_reg_) {
        ASSERT(auto_reg_); 
        while (!auto_reg_->empty()) {
            TclCommand* m = auto_reg_->front();
            auto_reg_->pop_front();
            reg(m);
        }
    
        delete auto_reg_;
        auto_reg_ = NULL;
    }

    // register the default commands
    if (! no_default_cmds) {
        HelpCommand* help = new HelpCommand();
        reg(help);

        LogCommand* log = new LogCommand();
        reg(log);
    }
    
    // evaluate the boot-time tcl commands (copied since tcl may
    // overwrite the string value)
    char* cmd = strdup(INIT_COMMAND);
    if (Tcl_Eval(interp_, cmd) != TCL_OK) {
        log_err("error in init commands: \"%s\"", interp_->result);
        return TCL_ERROR;
    }
    free(cmd);

    return TCL_OK;
}

TclCommandInterp::~TclCommandInterp()
{
    // the destructor isn't ever called
    NOTREACHED;
}

int
TclCommandInterp::exec_file(const char* file)
{
    int err;
    ScopeLock l(lock_);

    log_debug("executing command file %s", file);
    
    err = Tcl_EvalFile(interp_, (char*)file);
    
    if (err != TCL_OK) {
        logf(LOG_ERR, "error: line %d: '%s':\n%s",
             interp_->errorLine, Tcl_GetStringResult(interp_),
             Tcl_GetVar(interp_, "errorInfo", TCL_GLOBAL_ONLY));
    }
    
    return err;    
}

int
TclCommandInterp::exec_command(const char* command)
{
    int err;
    ScopeLock l(lock_);

    // ignore empty command lines
    if (command[0] == '\0')
        return TCL_OK;

    // tcl modifies the command string while executing it, so we need
    // to make a copy
    char* buf = strdup(command);

    log_debug("executing command '%s'", buf);
    
    err = Tcl_Eval(interp_, buf);
    
    free(buf);
    
    if (err != TCL_OK) {
        logf(LOG_ERR, "error: line %d: '%s':\n%s",
             interp_->errorLine, Tcl_GetStringResult(interp_),
             Tcl_GetVar(interp_, "errorInfo", TCL_GLOBAL_ONLY));
    }
    
    return err;
}

void
TclCommandInterp::command_loop(const char* prompt)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command_loop %s", prompt);
    
    if (Tcl_Eval(interp_, cmd) != TCL_OK) {
        log_err("tcl error in readline loop: \"%s\"",
                interp_->result);
    }
}

void
TclCommandInterp::event_loop()
{
    if (Tcl_Eval(interp_, "event_loop") != TCL_OK) {
        log_err("tcl error in event_loop: \"%s\"",
                interp_->result);
    }
}

void
TclCommandInterp::reg(TclCommand *command)
{
    ScopeLock l(lock_);
    
    command->logf(LOG_DEBUG, "%s command registering", command->name());
    
    Tcl_CreateObjCommand(interp_, 
                         (char*)command->name(),
                         TclCommandInterp::tcl_cmd,
                         (ClientData)command,
                         NULL);
    
    commands_.push_front(command);

    command->at_reg();
}

void
TclCommandInterp::auto_reg(TclCommand *command)
{
    // this should only be called from the static initializers, i.e.
    // we haven't been initialized yet
    ASSERT(instance_ == NULL);

    // we need to explicitly create the auto_reg list the first time
    // since there's no guarantee of ordering of static constructors
    if (!auto_reg_)
        auto_reg_ = new TclCommandList();
    
    auto_reg_->push_back(command);
}

void
TclCommandInterp::reg_atexit(void(*fn)(void*), void* data)
{
    ScopeLock l(lock_);
    Tcl_CreateExitHandler(fn, data);
}
    
int 
TclCommandInterp::tcl_cmd(ClientData client_data, Tcl_Interp* interp,
                       int objc, Tcl_Obj* const* objv)
{
    TclCommand* command = (TclCommand*)client_data;

    // first check for builtin commands
    if (command->do_builtins_ && objc > 2) {
        const char* cmd = Tcl_GetStringFromObj(objv[1], NULL);
        if (strcmp(cmd, "set") == 0) {
            return command->cmd_set(objc, (Tcl_Obj**)objv, interp);
        }
    }

    return command->exec(objc, (Tcl_Obj**)objv, interp);
}

void
TclCommandInterp::set_result(const char* result)
{
    Tcl_SetResult(interp_, (char*)result, TCL_VOLATILE);
}

void
TclCommandInterp::set_objresult(Tcl_Obj* obj)
{
    Tcl_SetObjResult(interp_, obj);
}

void
TclCommandInterp::append_result(const char* result)
{
    Tcl_AppendResult(interp_, (char*)result, NULL);
}

void
TclCommandInterp::vresultf(const char* fmt, va_list ap, bool append)
{
    StringBuffer buf;
    buf.vappendf(fmt, ap);
    
    if (append) {
        Tcl_AppendResult(interp_, buf.c_str(), NULL);
    } else {
        Tcl_SetResult(interp_, buf.c_str(), TCL_VOLATILE);
    }
}

void
TclCommandInterp::resultf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vresultf(fmt, ap, false);
    va_end(ap);
}

void
TclCommandInterp::append_resultf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vresultf(fmt, ap, true);
    va_end(ap);
}

void
TclCommandInterp::wrong_num_args(int argc, const char** argv, int parsed,
                                 int min, int max)
{
    set_result("wrong number of arguments to '");
    append_result(argv[0]);
    
    for (int i = 1; i < parsed; ++i) {
        append_result(" ");
        append_result(argv[i]);
    }
    append_result("'");

    if (max == min) {
        append_resultf(" expected %d, got %d", min, argc);
    } else if (max == INT_MAX) {
        append_resultf(" expected >%d, got %d", min, argc);
    } else {
        append_resultf(" expected %d - %d, got %d", min, max, argc);
    }
}

void
TclCommandInterp::wrong_num_args(int objc, Tcl_Obj** objv, int parsed,
                              int min, int max)
{
    char* argv[objc];
    for (int i = 0; i < objc; ++i) {
        argv[i] = Tcl_GetStringFromObj(objv[i], NULL);
    }
    wrong_num_args(objc, (const char**)argv, parsed, min, max);
}

const char*
TclCommandInterp::get_result()
{
    return Tcl_GetStringResult(interp_);
}

/******************************************************************************
 *
 * TclCommand
 *
 *****************************************************************************/
TclCommand::TclCommand(const char* name)
    : name_(name), do_builtins_(true)
{
    logpathf("/command/%s", name);
}

TclCommand::~TclCommand()
{
}

int
TclCommand::exec(int objc, Tcl_Obj** objv, Tcl_Interp* interp)
{
    // If the default implementation is called, then convert all
    // arguments to strings and then call the other exec variant.
    char* argv[objc];

    for (int i = 0; i < objc; ++i) {
        argv[i] = Tcl_GetStringFromObj(objv[i], NULL);
    }

    return exec(objc, (const char**) argv, interp);
}

int
TclCommand::exec(int argc, const char** argv, Tcl_Interp* interp)
{
    resultf("command %s not implemented", argv[0]);
    return TCL_ERROR;
}

void
TclCommand::resultf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    TclCommandInterp::instance()->vresultf(fmt, ap, false);
    va_end(ap);
}

void
TclCommand::append_resultf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    TclCommandInterp::instance()->vresultf(fmt, ap, true);
    va_end(ap);
}

int
TclCommand::cmd_set(int objc, Tcl_Obj** objv, Tcl_Interp* interp)
{
    ASSERT(objc >= 2);
    
    // handle "set binding [value]" command
    if (objc < 3 || objc > 4) {
        resultf("wrong number of args: expected 3-4, got %d", objc);
        return TCL_ERROR;
    }

    const char* var = Tcl_GetStringFromObj(objv[2], NULL);
    Tcl_Obj* val = objv[3];
    
    std::map<std::string, Binding*>::iterator itr;
    itr = bindings_.find(var);
    
    if (itr == bindings_.end()) {
        resultf("set: binding for %s does not exist", var);
        return TCL_ERROR;
    }

    // set value (if any)
    Binding* b = (*itr).second;

    if (objc == 4) 
    {
        switch(b->type_) 
        {
        case BINDING_INT:
            if (Tcl_GetIntFromObj(interp, val, b->val_.intval_) != TCL_OK) {
                resultf("%s set: %s not an integer value",
                        Tcl_GetStringFromObj(objv[0], 0),
                        Tcl_GetStringFromObj(val, 0));
                return TCL_ERROR;
            }
            break;
            
        case BINDING_BOOL:
            int boolval;
            if (Tcl_GetBooleanFromObj(interp, val, &boolval) != TCL_OK) {
                resultf("%s set: %s not an integer value",
                        Tcl_GetStringFromObj(objv[0], 0),
                        Tcl_GetStringFromObj(val, 0));
                return TCL_ERROR;
            }
            *(b->val_.boolval_) = boolval;
            break;
            
        case BINDING_ADDR:
        {
            char* addr = Tcl_GetStringFromObj(val, 0);
            if (gethostbyname(addr, b->val_.addrval_) != 0) {
                resultf("%s set: invalid value '%s' for addr", 
                        Tcl_GetStringFromObj(objv[0], 0), addr);
                return TCL_ERROR;
            }
            break;

        }    
        case BINDING_STRING:
            b->val_.stringval_->assign(Tcl_GetStringFromObj(val, 0));
            break;
            
        default:
            logf(LOG_CRIT, "unimplemented binding type %d", b->type_);
            ASSERT(0);
        }
    }

    switch(b->type_) 
    {
    case BINDING_INT:
        resultf("%d", *(b->val_.intval_));
        break;

    case BINDING_BOOL:
        if (*(b->val_.boolval_))
            set_result("true");
        else
            set_result("false");
        break;
        
    case BINDING_ADDR:
        resultf("%s", intoa(*(b->val_.addrval_)));
        break;
        
    case BINDING_STRING:
        set_result(b->val_.stringval_->c_str());
        break;
        
    default:
        logf(LOG_CRIT, "unimplemented binding type %d", b->type_);
        ASSERT(0);
    }
    
    return 0;
}

// boilerplate code
#define BIND_FUNCTIONS(_fn, _type, _typecode)                           \
void                                                                    \
_fn(const char* name, _type* val)                                       \
{                                                                       \
    if (bindings_.find(name) != bindings_.end())                        \
    {                                                                   \
        log_warn("warning, binding for %s already exists", name);       \
    }                                                                   \
                                                                        \
    log_debug("creating %s binding for %s -> %p", #_type, name, val);   \
                                                                        \
    bindings_[name] = new Binding(_typecode, val);                      \
}                                                                       \
                                                                        \
void                                                                    \
_fn(const char* name, _type* val, _type initval)                        \
{                                                                       \
    *val = initval;                                                     \
    if (bindings_.find(name) != bindings_.end())                        \
    {                                                                   \
        log_warn("warning, binding for %s already exists", name);       \
    }                                                                   \
                                                                        \
    log_debug("creating %s binding for %s -> %p", #_type, name, val);   \
                                                                        \
    bindings_[name] = new Binding(_typecode, val);                      \
}

BIND_FUNCTIONS(TclCommand::bind_i, int, BINDING_INT);
BIND_FUNCTIONS(TclCommand::bind_b, bool, BINDING_BOOL);
BIND_FUNCTIONS(TclCommand::bind_addr, in_addr_t, BINDING_ADDR);

void
TclCommand::bind_s(const char* name, std::string* val,
                      const char* initval)
{
    if (initval)
        val->assign(initval);
    
    if (bindings_.find(name) != bindings_.end()) {
        log_warn("warning, binding for %s already exists", name);
    }

    log_debug("creating string binding for %s -> %p", name, val);

    bindings_[name] = new Binding(BINDING_STRING, val);
}
