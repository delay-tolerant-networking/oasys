/*
 *    Copyright 2004-2006 Intel Corporation
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef _OASYS_TCL_COMMAND_H_
#define _OASYS_TCL_COMMAND_H_

#include <list> 
#include <map>
#include <string>

#include <stdarg.h>
#include <tcl.h>

#include <netinet/in.h>

#include "../compat/inttypes.h"
#include "../debug/DebugUtils.h"
#include "../util/Singleton.h"
#include "../util/StringBuffer.h"

namespace oasys {

// forward decls
class TclCommandInterp;
class TclCommand;
class Lock;

/**
 * A list of TclCommands
 */
typedef std::list<TclCommand*> TclCommandList;

/**
 * Command interpreter class
 * 
 * Command files are Tcl scripts. When a module registers itself, the
 * interpreter binds a new command with the module name.
 */
class TclCommandInterp : public Logger {
public:
    /**
     * Return the singleton instance of config. This should only ever
     * be called after Command::init is called to initialize the
     * instance, hence the assertion that instance_ isn't null.
     */
    static TclCommandInterp* instance() {
        ASSERT(instance_ != NULL);
        return instance_;
    }

    /**
     * Initialize the interpreter instance.
     */
    static int init(char* objv0, bool no_default_cmds = false);

    /**
     * Shut down the interpreter.
     */
    static void shutdown();

    /**
     * Read in a configuration file and execute its contents
     * \return 0 if no error, -1 otherwise.
     */ 
    int exec_file(const char* file);

    /**
     * Parse a single command string.
     *
     * \return 0 if no error, -1 otherwise
     */
    int exec_command(const char* command);

    /**
     * Run a command interpreter over tcp sockets on the given port.
     */
    void command_server(const char* prompt, in_addr_t addr, u_int16_t port);

    /**
     * Run a command interpreter loop. Doesn't return.
     */
    void command_loop(const char* prompt);

    /**
     * Just run the event loop. Also doesn't return.
     */
    void event_loop();

    /**
     * Bail out of the event loop or the command loop, whichever one
     * happens to be running.
     */
    void exit_event_loop();

    /**
     * Static callback function from Tcl to execute the commands.
     *
     * @param client_data Pointer to config module for which this
     *     command was registered.
     * @param interp Tcl interpreter
     * @param objc Argument count.
     * @param objv Argument values.
     */
    static int tcl_cmd(ClientData client_data, Tcl_Interp* interp,
                       int objc, Tcl_Obj* const* objv);

    /** 
     * Register the command module.
     */
    void reg(TclCommand* module);

    /**
     * Check if there's a tcl command registered by the given name. If
     * so, return true. If the command is an instance of TclCommand,
     * return it as well in the commandp parameter.
     */
    bool lookup(const char* command, TclCommand** commandp = NULL);

    /**
     * Schedule the auto-registration of a command module. This _must_
     * be called from a static initializer, before the Command
     * instance is actually created.
     */
    static void auto_reg(TclCommand* module);

    /**
     * Register a function to be called at exit.
     */
    void reg_atexit(void(*fn)(void*), void* data);

    /**
     * Set the TclResult string.
     */
    void set_result(const char* result);

    /**
     * Set an object for the TclResult.
     */
    void set_objresult(Tcl_Obj* obj);

    /**
     * Append the string to the TclResult
     */
    void append_result(const char* result);

    /**
     * Format and set the TclResult string.
     */
    void resultf(const char* fmt, ...) PRINTFLIKE(2, 3);

    /**
     * Format and append the TclResult string.
     */
    void append_resultf(const char* fmt, ...) PRINTFLIKE(2, 3);
    
    /**
     * Format and set the TclResult string.
     */
    void vresultf(const char* fmt, va_list ap, bool append);

    /**
     * Useful function for generating error strings indicating that
     * the wrong number of arguments were passed to the command.
     *
     * @param objc	original argument count to the command
     * @param objv	original argument vector to the command
     * @param parsed	number of args to include in error string
     * @param min	minimum number of expected args
     * @param max	maximum number of expected args (or INT_MAX)
     */
    void wrong_num_args(int objc, const char** objv, int parsed,
                        int min, int max);

    /**
     * Useful function for generating error strings indicating that
     * the wrong number of arguments were passed to the command.
     *
     * @param objc	original argument count to the command
     * @param objv	original argument vector to the command
     * @param parsed	number of args to include in error string
     * @param min	minimum number of expected args
     * @param max	maximum number of expected args (or INT_MAX)
     */
    void wrong_num_args(int objc, Tcl_Obj** objv, int parsed,
                        int min, int max);

    /**
     * Get the TclResult string.
     */
    const char* get_result();

    /**
     * Return the list of registered commands
     */
    const TclCommandList* commands() { return &commands_; }
    
protected:
    /**
     * Constructor (does nothing)
     */
    TclCommandInterp();

    /**
     * Do all of the actual initialization.
     */
    int do_init(char* objv0, bool no_default_cmds);
    
    /**
     * Destructor to clean up and finalize.
     */
    ~TclCommandInterp();
    
    Lock* lock_;			///< Lock for command execution
    Tcl_Interp* interp_;		///< Tcl interpreter

    TclCommandList commands_;		///< List of registered commands
    static TclCommandList* auto_reg_;	///< List of commands to auto-register

    static TclCommandInterp* instance_;	///< Singleton instance
};

/** 
 * Extend this class to provide the command hooks for a specific
 * module. Register commands with Command::instance()->reg() or use
 * the AutoTclCommand class.
 *
 * The "set" command is automatically defined for a module. Set is
 * used to change the value of variable which are bound using the
 * bind_* functions defined below.
 */
class TclCommand : public Logger {
public:
    /** 
     * Constructor
     *
     * @param name Name of the module
     * @param theNamespace Optional tcl namespace
     */
    TclCommand(const char* name, const char* theNamespace = 0);
    virtual ~TclCommand();
    
    /** 
     * Override this to get the arguments as raw tcl objects.
     *
     * @param objc Argument count 
     * @param objv Argument values
     * @param interp Tcl interpreter
     *
     * @return 0 on success, -1 on error
     */
    virtual int exec(int objc, Tcl_Obj** objv, Tcl_Interp* interp);

    /** 
     * Override this to parse the list of arguments as strings.
     *
     * @param argc Argument count 
     * @param argv Argument values
     * @param interp Tcl interpreter
     *
     * @return 0 on success, -1 on error
     */
    virtual int exec(int argc, const char** argv, Tcl_Interp* interp);

    /**
     * Handles the "info" command which prints out the value of the
     * bound variables for a TclCommand.
     *
     * @param interp Tcl interperter
     */
    virtual int cmd_info(Tcl_Interp* interp);

    /**
     * Internal handling of the "set" command.
     *
     * @param objc Argument count 
     * @param objv Argument values
     * @param interp Tcl interpreter
     *
     * @return 0 on success, -1 on error
     */
    virtual int cmd_set(int objc, Tcl_Obj** objv, Tcl_Interp* interp);
    
    /** 
     * Get the name of the module.
     */
    const char* name() const { return name_.c_str(); }

    /**
     * Return the help string for this command.
     */
    virtual const char* help_string() { return help_.c_str(); }

    /**
     * Does this command have any bindings?
     */
    bool hasBindings() { return ! bindings_.empty(); }

protected:
    friend class TclCommandInterp;
    
    std::string name_;          ///< Name of the module
    StringBuffer help_;		///< Help string
    bool do_builtins_;		///< Set to false if a module doesn't want
                                ///< builtin commands like "set"

    /**
     * Binding type constants
     */
    enum type_t {
        BINDING_INVALID = -1,
        BINDING_INT = 1,
        BINDING_INT16,
        BINDING_DOUBLE, 
        BINDING_BOOL, 
        BINDING_STRING, 
        BINDING_ADDR
    };
    
    /** 
     * Internal structure for bindings.
     */
    struct Binding {
        Binding(type_t type, void* val)
            : type_(type)
        {
            val_.voidval_ = val;
        }

        type_t type_;             	///< Type of the binding
        
        union {
            void*	voidval_;
            int*	intval_;
            int16_t*	int16val_;
            double*	doubleval_;
            bool*	boolval_;
            in_addr_t*	addrval_;
            std::string* stringval_;
        } val_; 			///< Union for value pointer
    };
    
    /** 
     * Type for the table of bindings.
     */
    typedef std::map<std::string, Binding*> BindingTable;

    /**
     * The table of registered bindings.
     */
    BindingTable bindings_;
   
    /**
     * Bind an integer to the set command
     */
    void bind_i(const char* name, int* val, const char *help = NULL);
    void bind_i(const char* name, int* val, int initval,
                const char* help = NULL);

    /**
     * Bind a 16 bit integer to the set command.
     */
    void bind_i(const char* name, int16_t* val, const char *help = NULL);
    void bind_i(const char* name, int16_t* val, int16_t initval,
                const char* help = NULL);
    
    ///@{
    /**
     * Unsigned / signed aliases for other integer types.
     */
    void bind_i(const char* name, unsigned int* val,
                const char* help = NULL)
    {
        bind_i(name, (int*)val, help);
    }

    void bind_i(const char* name, unsigned int* val, unsigned int initval,
                const char* help = NULL)
    {
        bind_i(name, (int*)val, initval, help);
    }
    
    void bind_i(const char* name, u_int16_t* val,
                const char* help = NULL)
    {
        bind_i(name, (int16_t*)val, help);
    }

    void bind_i(const char* name, u_int16_t* val, u_int16_t initval,
                const char* help = NULL)
    {
        bind_i(name, (int16_t*)val, initval, help);
    }
    
    ///@}
    
    /**
     * Bind a double to the set command
     */
    void bind_d(const char* name, double* val, const char* help = NULL);
    void bind_d(const char* name, double* val, double initval,
                const char* help = NULL);

    /**
     * Bind a boolean to the set command
     */
    void bind_b(const char* name, bool* val, const char* help = NULL);
    void bind_b(const char* name, bool* val, bool initval,
                const char* help = NULL);
    
    /**
     * Bind a string to the set command
     */
    void bind_s(const char* name, std::string* str,
                const char* initval, const char* help);

    void bind_s(const char* name, std::string* str,
                const char* help = NULL)
    {
        return bind_s(name, str, NULL, help);
    }

    /**
     * Bind an ip addr for the set command, allowing the user to pass
     * a hostname and/or a dotted quad style address
     */
    void bind_addr(const char* name, in_addr_t* addrp,
                   const char* help = NULL);
    void bind_addr(const char* name, in_addr_t* addrp,
                   in_addr_t initval, const char* help = NULL);

    /**
     * Unbind a variable.
     */
    void unbind(const char* name);
    
    /**
     * Set the TclResult string.
     */
    void set_result(const char* result)
    {
        TclCommandInterp::instance()->set_result(result);
    }

    /**
     * Set a Tcl_Obj as the result.
     */
    void set_objresult(Tcl_Obj* obj)
    {
        TclCommandInterp::instance()->set_objresult(obj);
    }

    /**
     * Append the TclResult string.
     */
    void append_result(const char* result)
    {
        TclCommandInterp::instance()->append_result(result);
    }

    /**
     * Format and set the TclResult string.
     */
    void resultf(const char* fmt, ...) PRINTFLIKE(2, 3);

    /**
     * Format and set the TclResult string.
     */
    void append_resultf(const char* fmt, ...) PRINTFLIKE(2, 3);

    /**
     * Useful function for generating error strings indicating that
     * the wrong number of arguments were passed to the command.
     *
     * @param objc	original argument count to the command
     * @param objv	original argument vector to the command
     * @param parsed	number of args to include in error string
     * @param min	minimum number of expected args
     * @param max	maximum number of expected args (or INT_MAX)
     */
    void wrong_num_args(int objc, const char** objv, int parsed,
                        int min, int max)
    {
        TclCommandInterp::instance()->
            wrong_num_args(objc, objv, parsed, min, max);
    }

    /**
     * Useful function for generating error strings indicating that
     * the wrong number of arguments were passed to the command.
     *
     * @param objc	original argument count to the command
     * @param objv	original argument vector to the command
     * @param parsed	number of args to include in error string
     * @param min	minimum number of expected args
     * @param max	maximum number of expected args (or INT_MAX)
     */
    void wrong_num_args(int objc, Tcl_Obj** objv, int parsed,
                        int min, int max)
    {
        TclCommandInterp::instance()->
            wrong_num_args(objc, objv, parsed, min, max);
    }
    
    /**
     * Append the given information to the current help string,
     * typically used for a set of alternatives for subcommands.
     */
    void add_to_help(const char* subcmd, const char* help_str)
    {
        help_.appendf("%s %s\n", name(), subcmd);
        if (help_str) {
            help_.appendf("\t%s\n", help_str);
        }
        help_.append("\n");
    }
};

/**
 * TclCommand that auto-registers itself.
 */
class AutoTclCommand : public TclCommand {
public:
    AutoTclCommand(const char* name, const char* theNamespace = 0)
        : TclCommand(name, theNamespace)
    {
        TclCommandInterp::auto_reg(this);
    }
};

} // namespace oasys

#endif /* _OASYS_TCL_COMMAND_H_ */
