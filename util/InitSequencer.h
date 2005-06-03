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
#ifndef __INITSEQUENCER_H__
#define __INITSEQUENCER_H__

#include <vector>
#include <map>
#include <string>

#include "../debug/Logger.h"

namespace oasys {

// predecl
class InitStep;

/*!
 * Automatically checks and sequences initialization. Note, this code
 * assumes single threading among the InitStep objects.
 * 
 * Suppose you have a singleton style modules A, B and C with an
 * ::init method and B depends on A and C depends on A and B. C also
 * has configuration options which need to be set. Then:
 * 
 * @code
 * // A.cc
 * OASYS_DECLARE_INIT_MODULE_0(example, A) { 
 *     A::init(); 
 * }
 *
 * // B.cc - depends on A
 * OASYS_DECLARE_INIT_MODULE_1(example, B, "example::A") { 
 *     B::init(); 
 * }
 *
 * // C.cc - configuration option
 * OASYS_DECLARE_INIT_CONFIG(example, C_config);
 *
 * // C.cc - depends on A, B and C_config
 * OASYS_DECLARE_INIT_MODULE_2(example, C, "example::A", 
 *                             "example::B", "example::C_config") 
 * { 
 *     C::init(); 
 * }
 * @endcode
 *
 * Now to start component "C", something needs to call
 * OASYS_INIT_CONFIG_DONE(example, C_config) first, and then:
 *
 * @code
 * // ... some configuration setting code
 * OASYS_INIT_CONFIG_DONE(example, C_config);
 *
 * // ...
 *
 * Singleton<InitSequencer> sequencer;
 * sequencer->start("example::C");
 * @endcode
 */
class InitSequencer : public Logger {
public:
    typedef std::map<std::string, InitStep*> StepMap;
    typedef std::vector<std::string>         Plan;

    InitSequencer();

    /*!
     * Perform the actions needed to start the component.
     *
     * @param component to start
     * @param plan optional plan (made by humans) which the
     *        InitSequencer will try to execute.
     */
    int start(std::string step, Plan* plan = 0);

    /*!
     * Add a step to the initialization.
     */
    void add_step(InitStep* step);

    /*!
     * Get a InitStep* from the name
     */
    InitStep* get_step(const std::string& name);

    /*!
     * Reset the done state of all of the modules. NOTE: This is
     * mostly for the testing harness. You better know what you're
     * doing if you call this.
     */
    void reset();

private:
    typedef std::vector<std::string>              ReverseDepList;
    typedef std::map<std::string, ReverseDepList> ReverseDepEdges;

    StepMap steps_;
    int     dfs_time_;

    //! Run the steps
    int run_steps();

    //! Do topological sort
    int topo_sort();

    // helper function to dfs
    void dfs(InitStep* step, ReverseDepEdges& edges);
    
    //! Mark steps that are needed to start target 
    void mark_dep(const std::string& target);
};

/*!
 * A single step in the dependency ordering.
 */
class InitStep {
    friend class InitSequencer;

public:
    typedef std::vector<std::string> DepList;

    /*!
     * Takes a list of depsize (const char*) dependency names.
     */
    InitStep(const std::string& the_namespace, const std::string& name);
    InitStep(const std::string& the_namespace, 
             const std::string& name, int depsize, ...);
    InitStep(const std::string& the_namespace, 
             const std::string& name, const DepList& deps);

    virtual ~InitStep() {}
    
    //! Run this component. Returns 0 on no error.
    virtual int run();

    //! @return true if all dependencies have been met.
    bool dep_are_satisfied();
    
    const DepList& dependencies() { return dependencies_; }
    std::string name() { return name_; }
    bool        done() { return done_; }
    int         time() { return time_; }

protected:
    bool        done_;

    //! Override this to start the component
    virtual int run_component() = 0;

private:
    std::string name_;
    DepList     dependencies_;

    bool mark_;                 // mark for dep checking
    int  time_;                 // finishing time for topo-sort
};

/*!
 * InitStep for configuration modules. Configurations are not done by
 * running them, they need to be explicitly set (e.g. configured) to
 * be marked done.
 */
class InitConfigStep : public InitStep {
public:
    InitConfigStep(const std::string& the_namespace,
                   const std::string& name) 
        : InitStep(the_namespace, name) {}

    int  run()                { return 0; } 
    void configuration_done() { done_ = true; }

protected:
    int  run_component()      { NOTREACHED; }
};

/*!
 * @{
 * Prefer these macros to declaring the dependencies because they
 * check the number of arguments passed to the decl.
 */
#define OASYS_DECLARE_INIT_MODULE_0(_namespace, _name)                  \
class InitModule##_namespace##_name : public ::oasys::InitStep {        \
public:                                                                 \
    InitModule##_namespace##_name() : InitStep(#_namespace, #_name) {}  \
protected:                                                              \
    int run_component();                                                \
};                                                                      \
InitModule##_namespace##_name *                                         \
    ::oasys::Singleton<InitModule##_namespace##_name>::instance_ = 0;   \
InitModule##_namespace##_name * init_module_##_name =                   \
    ::oasys::Singleton<InitModule##_namespace##_name>::instance();      \
int InitModule##_namespace##_name::run_component()

#define OASYS_DECLARE_INIT_MODULE_1(_namespace, _name, _dep1)                   \
    OASYS_DECLARE_INIT_MODULE(_namespace, _name, 1, _dep1)                 
#define OASYS_DECLARE_INIT_MODULE_2(_namespace, _name, _dep1, _dep2)    \
    OASYS_DECLARE_INIT_MODULE(_namespace, _name, 2, _dep1, _dep2)
#define OASYS_DECLARE_INIT_MODULE_3(_namespace, _name, _dep1, _dep2, _dep3)     \
    OASYS_DECLARE_INIT_MODULE(_namespace, _name, 3, _dep1, _dep2, _dep3)
#define OASYS_DECLARE_INIT_MODULE_4(_namespace, _name, _dep1, _dep2, _dep3, _dep4)      \
    OASYS_DECLARE_INIT_MODULE(_namespace, _name, 4, _dep1, _dep2, _dep3, _dep4)
//! @}

/*!
 * Declare an initialization module with _num_dep dependencies. Use
 * the above macros instead of
 */
#define OASYS_DECLARE_INIT_MODULE(_namespace, _name, _num_dep, _args...)        \
class InitModule##_namespace##_name : public ::oasys::InitStep {                \
public:                                                                         \
    InitModule##_namespace##_name()                                             \
        : InitStep(#_namespace, #_name, _num_dep, _args) {}                     \
protected:                                                                      \
    int run_component();                                                        \
};                                                                              \
InitModule##_namespace##_name *                                                 \
    ::oasys::Singleton<InitModule##_namespace##_name>::instance_ = 0;           \
InitModule##_namespace##_name * init_module_##_name =                           \
    ::oasys::Singleton<InitModule##_namespace##_name>::instance();              \
int InitModule##_namespace##_name::run_component()

/*!
 * Declare a configuration module. 
 */
#define OASYS_DECLARE_INIT_CONFIG(_namespace, _name)                    \
class InitModule##_namespace##_name : public InitConfigStep {           \
public:                                                                 \
    InitModule##_namespace##_name()                                     \
        : InitConfigStep(#_namespace, #_name) {}                        \
};                                                                      \
InitModule##_namespace##_name *                                         \
    ::oasys::Singleton<InitModule##_namespace##_name>::instance_ = 0;   \
InitModule##_namespace##_name * init_module_##_name =                   \
    ::oasys::Singleton<InitModule##_namespace##_name>::instance();

/*!
 * Call this to set a configuration module to the "done" state.
 */
#define OASYS_INIT_CONFIG_DONE(_namespace, _name)                       \
do {                                                                    \
    ::oasys::Singleton<InitModule##_namespace##_name>::instance()       \
         ->configuration_done();                                        \
} while (0)

} // namespace oasys

#endif /* __INITSEQUENCER_H__ */
