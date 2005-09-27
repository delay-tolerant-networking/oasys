/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2004 Intel Corporation. All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 *   Neither the name of the Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "TclCommand.h"
#include "HelpCommand.h"
#include "DebugCommand.h"
#include "LogCommand.h"

#include "debug/DebugUtils.h"
#include "io/NetUtils.h"
#include "thread/SpinLock.h"
#include "util/StringBuffer.h"
#include "util/InitSequencer.h"

namespace oasys {

/******************************************************************************
 *
 * TclCommandInterp
 *
 *****************************************************************************/
// static variables
TclCommandInterp* TclCommandInterp::instance_;
TclCommandList*   TclCommandInterp::auto_reg_ = NULL;

#include "command-init-tcl.c"

TclCommandInterp::TclCommandInterp()
    : Logger("/command")
{}

int
TclCommandInterp::do_init(char* argv0, bool no_default_cmds)
{
    interp_ = Tcl_CreateInterp();
    lock_   = new SpinLock();

    // for some reason, this needs to be called to set up the various
    // locale strings and get things like the "ascii" encoding defined
    // for a file channel
    Tcl_FindExecutable(argv0);
    
    // run Tcl_Init to set up the local tcl package path, but don't
    // depend on it succeeding in case there's a strange tcl
    // installation
    if (Tcl_Init(interp_) != TCL_OK) {
        log_warn("error in Tcl_Init: \"%s\", continuing...", interp_->result);
    }

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

        DebugCommand* debug = new DebugCommand();
        reg(debug);
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
TclCommandInterp::init(char* argv0, bool no_default_cmds)
{
    ASSERT(instance_ == NULL);
    instance_ = new TclCommandInterp();
    
    return instance_->do_init(argv0, no_default_cmds);
}

int
TclCommandInterp::exec_file(const char* file)
{
    int err;
    ScopeLock l(lock_, "TclCommandInterp::exec_file");

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
    ScopeLock l(lock_, "TclCommandInterp::exec_command");

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
TclCommandInterp::command_server(const char* prompt,
                                 in_addr_t addr, u_int16_t port)
{
    log_debug("starting command server on %s:%d", intoa(addr), port);
    StringBuffer cmd("command_server %s %s %d", prompt, intoa(addr), port);
    
    if (Tcl_Eval(interp_, cmd.c_str()) != TCL_OK) {
        log_err("tcl error in readline loop: \"%s\"",
                interp_->result);
    }
}

void
TclCommandInterp::command_loop(const char* prompt)
{
    StringBuffer cmd("command_loop %s", prompt);
    
    if (Tcl_Eval(interp_, cmd.c_str()) != TCL_OK) {
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
    ScopeLock l(lock_, "TclCommandInterp::reg");
    
    command->logf(LOG_DEBUG, "%s command registering", command->name());

    Tcl_CmdInfo info;
    if (Tcl_GetCommandInfo(interp_, (char*)command->name(), &info) != 0) {
        log_warn("re-registering command %s over existing command",
                 command->name());
    }
                 
    Tcl_CreateObjCommand(interp_, 
                         const_cast<char*>(command->name()),
                         TclCommandInterp::tcl_cmd,
                         (ClientData)command,
                         NULL);
    
    commands_.push_front(command);
}

bool
TclCommandInterp::lookup(const char* command, TclCommand** commandp)
{
    Tcl_CmdInfo info;

    if (Tcl_GetCommandInfo(interp_, (char*)command, &info) == 0) {
        log_debug("lookup tcl command %s: does not exist", command);
        return false;
    }

    if (info.objProc == TclCommandInterp::tcl_cmd) {
        log_debug("lookup tcl command %s: exists and is TclCommand %p",
                  command, info.clientData);
        
        if (commandp)
            *commandp = (TclCommand*)info.objClientData;
        
    } else {
        log_debug("lookup tcl command %s: exists but is not a TclCommand",
                  command);
    }

    return true;
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
    ScopeLock l(lock_, "TclCommandInterp::reg_atexit");
    Tcl_CreateExitHandler(fn, data);
}
    
int 
TclCommandInterp::tcl_cmd(ClientData client_data, Tcl_Interp* interp,
                          int objc, Tcl_Obj* const* objv)
{
    TclCommand* command = (TclCommand*)client_data;

    // first check for builtin commands
    if (command->do_builtins_) 
    {
        if (objc == 2) 
        {
            const char* cmd = Tcl_GetStringFromObj(objv[1], NULL);
            if (strcmp(cmd, "info") == 0) {
                return command->cmd_info(interp);
            }
        }
        else if (objc > 2) 
        {
            const char* cmd = Tcl_GetStringFromObj(objv[1], NULL);
            if (strcmp(cmd, "set") == 0) {
                return command->cmd_set(objc, (Tcl_Obj**)objv, interp);
            }
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
        append_resultf(" expected at least %d, got %d", min, argc);
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
TclCommand::TclCommand(const char* name, const char* theNamespace)
    : do_builtins_(true)
{
    logpathf("/command/%s", name);

    if (theNamespace != 0) {
        name_ += theNamespace;
        name_ += "::";
    }

    name_ += name;

    // workaround for bug seen with Cygwin 3.4.4-1, where
    // help_ ends up with two copies of the text after a
    // buffer grow operation.
    help_.reserve(1024);
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
    resultf("command %s unknown argument", argv[0]);
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
TclCommand::cmd_info(Tcl_Interp* interp)
{
    StringBuffer buf;

    for (BindingTable::iterator itr = bindings_.begin();
         itr != bindings_.end(); ++itr)
    {
        buf.appendf("%s ", (*itr).first.c_str());
    }
    
    set_result(buf.c_str());
    return TCL_OK;
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
    
    BindingTable::iterator itr;
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
            
        case BINDING_DOUBLE:
            if (Tcl_GetDoubleFromObj(interp, val, b->val_.doubleval_) != TCL_OK) {
                resultf("%s set: %s not an double value",
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

    case BINDING_DOUBLE:
        resultf("%f", *(b->val_.doubleval_));
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
_fn(const char* name, _type* val, const char* help)                     \
{                                                                       \
    if (bindings_.find(name) != bindings_.end())                        \
    {   if (Log::initialized()) {                                       \
            log_warn("warning, binding for %s already exists", name);   \
        }                                                               \
    }                                                                   \
                                                                        \
    if (Log::initialized()) {                                           \
        log_debug("creating %s binding for %s -> %p",                   \
                  #_type, name, val);                                   \
    }                                                                   \
                                                                        \
    bindings_[name] = new Binding(_typecode, val);                      \
    StringBuffer subcmd("set %s", name);                                \
    add_to_help(subcmd.c_str(), help);                                  \
}                                                                       \
                                                                        \
void                                                                    \
_fn(const char* name, _type* val, _type initval, const char* help)      \
{                                                                       \
    *val = initval;                                                     \
    if (bindings_.find(name) != bindings_.end())                        \
    {                                                                   \
        if (Log::initialized()) {                                       \
            log_warn("warning, binding for %s already exists", name);   \
        }                                                               \
    }                                                                   \
                                                                        \
                                                                        \
    if (Log::initialized()) {                                           \
        log_debug("creating %s binding for %s -> %p",                   \
                  #_type, name, val);                                   \
    }                                                                   \
                                                                        \
    bindings_[name] = new Binding(_typecode, val);                      \
    StringBuffer subcmd("set %s", name);                                \
    add_to_help(subcmd.c_str(), help);                                  \
}

BIND_FUNCTIONS(TclCommand::bind_i, int, BINDING_INT);
BIND_FUNCTIONS(TclCommand::bind_d, double, BINDING_DOUBLE);
BIND_FUNCTIONS(TclCommand::bind_b, bool, BINDING_BOOL);
BIND_FUNCTIONS(TclCommand::bind_addr, in_addr_t, BINDING_ADDR);

void
TclCommand::bind_s(const char* name, std::string* val,
                   const char* initval, const char* help)
{
    if (initval)
        val->assign(initval);
    
    if (bindings_.find(name) != bindings_.end()) {
        if (Log::initialized()) {
            log_warn("warning, binding for %s already exists", name);
        }
    }

    if (Log::initialized()) {
        log_debug("creating string binding for %s -> %p", name, val);
    }

    bindings_[name] = new Binding(BINDING_STRING, val);
    StringBuffer subcmd("set %s", name);
    add_to_help(subcmd.c_str(), help);
}

void
TclCommand::unbind(const char* name)
{
    BindingTable::iterator iter = bindings_.find(name);

    if (iter == bindings_.end()) {
        if (Log::initialized()) {
            log_warn("warning, binding for %s doesn't exist", name);
        }
        return;
    }

    if (Log::initialized()) {
        log_debug("removing binding for %s", name);
    }

    Binding* old = iter->second;
    bindings_.erase(iter);

    delete old;
}

} // namespace oasys
