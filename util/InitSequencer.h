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
 * @code
 * COMPONENT_4("name", "a", "b", "c", "d");
 * COMPONENT_3("name", "a", "b", "c");
 * COMPONENT_2("name", "a", "b");
 * 
 * Singleton<InitSequencer>::instance()->start("component");
 * 
 * // -- or --
 * 
 * InitSequencer::Plan plan;
 * plan.push_back(...);
 * ...
 * 
 * Singleton<InitSequencer>::instance()->start("component", &plan);
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

private:
    typedef std::vector<std::string>              ReverseDepList;
    typedef std::map<std::string, ReverseDepList> ReverseDepEdges;

    StepMap steps_;
    int     dfs_time_;

    /// Run the steps
    int run_steps();

    /// Do topological sort
    int topo_sort();

    // helper function to dfs
    void dfs(InitStep* step, ReverseDepEdges& edges);
    
    /// Mark steps that are needed to start target 
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
    InitStep(const std::string& name);
    InitStep(const std::string& name, int depsize, ...);
    InitStep(const std::string& name, const DepList& deps);

    virtual ~InitStep() {}
    
    /// Run this component. Returns 0 on no error.
    virtual int run();

    /// @return true if all dependencies have been met.
    bool dep_are_satisfied();
    
    const DepList& dependencies() { return dependencies_; }
    std::string name() { return name_; }
    bool        done() { return done_; }
    int         time() { return time_; }

protected:
    bool        done_;

    /// Override this to start the component
    virtual int run_component() = 0;

private:
    std::string name_;
    DepList     dependencies_;

    bool        mark_;          // mark for dep checking
    int         time_;          // finishing time for topo-sort
};

class InitConfigStep : public InitStep {
public:
    InitConfigStep(const std::string& name) : InitStep(name) {}
    
    int run() { return 0; }
    void set_configured() { done_ = true; }
};

} // namespace oasys

#endif /* __INITSEQUENCER_H__ */
