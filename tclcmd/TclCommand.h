#ifndef _TCL_COMMAND_H_
#define _TCL_COMMAND_H_

#include <list> 
#include <map>
#include <string>

#include <stdarg.h>
#include <tcl.h>

#include <netinet/in.h>

#include "../debug/Debug.h"

/*
 * In tcl8.4, the objv to commands is const, but not so in tcl8.3, so
 * we need this stupid define, and force cast everything to const in
 * the implementation of tcl_cmd.
 */
#ifdef CONST84
#define CONSTTCL84 CONST84
#else
#define CONSTTCL84
#endif

// forward decls
class TclCommandInterp;
class TclCommand;
class Lock;

/**
 * A list of TclCommands
 */
typedef std::list<TclCommand*> TclCommandList;

/**
 *
 * Command interpreter class
 * 
 * Command files are Tcl scripts. When a module registers itself, the
 * interpreter binds a new command with the module name.
 *
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
    static int init(char* objv0, bool no_default_cmds = false) {
        ASSERT(instance_ == NULL);
        instance_ = new TclCommandInterp();
        return instance_->do_init(objv0, no_default_cmds);
    }

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
     * Run a command interpreter loop. Doesn't return.
     */
    void command_loop(const char* prompt);

    /**
     * Just run the event loop. Also doesn't return.
     */
    void event_loop();

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
     * Destructor is never called (and issues an assertion).
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
 */
class TclCommand : public Logger {
public:
    /** 
     * Constructor
     *
     *  @param name Name of the module
     */
    TclCommand(const char* name);
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
     * @param objc Argument count 
     * @param objv Argument values
     * @param interp Tcl interpreter
     *
     * @return 0 on success, -1 on error
     */
    virtual int exec(int objc, const char** objv, Tcl_Interp* interp);

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
    const char* name() const { return name_; }

    /**
     * Return the help string for this command.
     */
    virtual const char* help_string() { return help_.c_str(); }

protected:
    friend class TclCommandInterp;
    
    const char* name_;          ///< Name of the module
    std::string help_;		///< Help string
    bool do_builtins_;		///< Set to false if a module doesn't want
                                ///< builtin commands like "set"

    /**
     * Binding type constants
     */
    enum type_t {
        BINDING_INVALID = -1,
        BINDING_INT = 1,
        BINDING_BOOL, 
        BINDING_STRING, 
        BINDING_ADDR
    };
    
    /** 
     * storage types for bindings 
     */
    struct Binding {
        Binding(type_t type, void* val)
            : type_(type)
        {
            val_.voidval_ = val;
        }
        
        /**
         * Add a destructor declaration with no implementation since
         * this is never called.
         */
        ~Binding();

        type_t type_;             	///< Type of the binding
        union {
            void*	voidval_;
            int*	intval_;
            bool*	boolval_;
            in_addr_t*	addrval_;
            std::string* stringval_;
        } val_; 			///< Union for value pointer
    };
    
    /** 
     * Binding list.
     *
     * The "set" command is automatically defined for a module. Set is
     * used to change the value of variable which are bound using the
     * bind_* functions defined below.
     * 
     */
    std::map<std::string, Binding*> bindings_;
   
    /**
     * Bind an integer to the set command
     */
    void bind_i(const char* name, int* val);
    void bind_i(const char* name, int* val, int initval);

    ///@{
    /**
     * Aliases for other integer types.
     */
    void bind_i(const char* name, int16_t* val)   { bind_i(name, (int*)val); }
    void bind_i(const char* name, int8_t* val)    { bind_i(name, (int*)val); }
    void bind_i(const char* name, u_int32_t* val) { bind_i(name, (int*)val); }
    void bind_i(const char* name, u_int16_t* val) { bind_i(name, (int*)val); }
    void bind_i(const char* name, u_int8_t* val)  { bind_i(name, (int*)val); }
    ///@}
    
    /**
     * Bind a boolean to the set command
     */
    void bind_b(const char* name, bool* val);
    void bind_b(const char* name, bool* val, bool initval);
    
    /**
     * Bind a string to the set command
     */
    void bind_s(const char* name, std::string* str,
                const char* initval = 0);

    /**
     * Bind an ip addr for the set command, allowing the user to pass
     * a hostname and/or a dotted quad style address
     */
    void bind_addr(const char* name, in_addr_t* addrp);
    void bind_addr(const char* name, in_addr_t* addrp,
                   in_addr_t initval);

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
     * Callback that's issued just after the command is registered.
     * This allows commands (particularly AutoTclCommand instances)
     * to do any post-registration activities like binding their vars.
     */
    virtual void at_reg() {}

    /**
     * Append the given information to the current help string,
     * typically used for a set of alternatives for subcommands.
     */
    void add_to_help(const char* subcmd, const char* help_str)
    {
        help_.append(name());
        help_.append(" ");
        help_.append(subcmd);
        help_.append(": ");
        help_.append(help_str);
    }
};

/**
 * TclCommand that auto-registers itself.
 */
class AutoTclCommand : public TclCommand {
public:
    AutoTclCommand(const char* name)
        : TclCommand(name)
    {
        TclCommandInterp::auto_reg(this);
    }
};   

#endif /* _TCL_COMMAND_H_ */