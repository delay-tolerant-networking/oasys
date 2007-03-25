/*
 *    Copyright 2007 Intel Corporation
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

#ifndef _OASYS_APP_H_
#define _OASYS_APP_H_

#include <string>

#include "../debug/Logger.h"
#include "../util/Daemonizer.h"
#include "../util/Getopt.h"

namespace oasys {

class Getopt;

/**
 * Helper class to wrap application initialization, handling all
 * common arguments.
 */
class App : public Logger {
public:
    /**
     * Constructor given the name and version of the app.
     */
    App(const char* classname,
        const char* name,
        const char* version = "(unknown)");

    /**
     * Destructor.
     */
    virtual ~App() {}

    /**
     * Main app initialization hook.
     */
    void init_app(int argc, char* const argv[],
                  const char* extra_usage = "");

    /**
     * Virtual callback to provide app-specific options in the opts_
     * instance variable. The base class implementation should be
     * called by any derived classes to fill in the default options.
     */
    virtual void fill_options();

    /**
     * Virtual callback to validate the options after Getopt was
     * called. The default implementation ensures there were no
     * extraneous arguments, but subclasses may override this
     * behavior.
     */
    virtual void validate_options(int argc,
                                  char* const argv[],
                                  int remainder);
    
protected:
    /**
     * Flags used for initialization of default options
     */
    typedef enum {
        DAEMONIZE_OPT = 1 << 1,
        CONF_FILE_OPT = 1 << 2,
    } default_opt_flags_t;
        
    /**
     * Hook used by subclasses to initialize the default options, some
     * of which are controlled by flags.
     */
    void fill_default_options(int flags);

    /**
     * When daemonized, notifies the parent of the exit status. In
     * either case, exits with the given status.
     */
    void notify_and_exit(char status);

    /// @{ Option fields
    Getopt                opts_;
    std::string           name_;
    std::string           version_;
    
    int                   random_seed_;
    bool                  random_seed_set_;
    bool                  print_version_;
    std::string           loglevelstr_;
    oasys::log_level_t    loglevel_;
    std::string           logfile_;
    std::string           debugpath_;
    bool                  daemonize_;
    oasys::Daemonizer     daemonizer_;
    std::string           conf_file_;
    bool                  conf_file_set_;
    bool                  ignore_sigpipe_;
    /// @} 

private:    
    void init_log();
    void init_signals();
    void init_random();
};

} // namespace oasys

#endif /* _OASYS_APP_H_ */
