#include <cstdarg>

#include "InitSequencer.h"
#include "../util/Singleton.h"

namespace oasys {

//////////////////////////////////////////////////////////////////////////////
//
// InitSequencer
//
InitSequencer* Singleton<InitSequencer>::instance_;

/// Sort in decreasing order
struct InitStepSort {
    bool operator()(InitStep* left, InitStep* right)
    {
        return left->time() > right->time();
    }
};


InitSequencer::InitSequencer()
    : Logger("/oasys/init_sequencer")
{}

int 
InitSequencer::start(std::string step, Plan* plan)
{
    int err;
    
    mark_dep(step);

    err = topo_sort();
    if (err != 0) 
    {
        return err;
    }

    err = run_steps();
    if (err != 0)
    {
        return err;
    }

    return 0;
}

void 
InitSequencer::add_step(InitStep* step)
{
    ASSERT(step != 0);
    
    if (steps_.find(step->name()) != steps_.end()) 
    {
        log_warn("Step %s already known to sequencer, ignoring", 
                 step->name().c_str());
        return;
    }

    steps_[step->name()] = step;
}

InitStep*
InitSequencer::get_step(const std::string& name)
{
    ASSERT(steps_.find(name) != steps_.end());
    return steps_[name];
}

void
InitSequencer::reset()
{
    for (StepMap::iterator i = steps_.begin(); 
         i != steps_.end(); ++i)
    {
        i->second->done_ = false;
    }
}

int
InitSequencer::run_steps()
{
    std::vector<InitStep*> step_list;

    for (StepMap::iterator i = steps_.begin(); 
         i != steps_.end(); ++i)
    {
        step_list.push_back(i->second);
    }
    std::sort(step_list.begin(), step_list.end(), InitStepSort());
    
    int err = 0;
    for (std::vector<InitStep*>::iterator i = step_list.begin();
         i != step_list.end(); ++i)
    {
        InitStep* step = *i;

        log_debug("step %d %s", step->time(), step->name().c_str());
        if (step->mark_ && !step->done())
        {
            log_debug("running %s", step->name().c_str());
            ASSERT(step->dep_are_satisfied());
            err = step->run();
            if (err != 0) 
            {
                log_warn("%s had an error, stopping...", step->name().c_str());
                break;
            }
        }
    }
    return err;
}

int 
InitSequencer::topo_sort()
{
    std::vector<InitStep*> step_stack;
    ReverseDepEdges edges;

    // make backwards edges
    for (StepMap::iterator i = steps_.begin(); i != steps_.end(); ++i)
    {
        InitStep* step = i->second;
        step->time_    = -1;

        for (ReverseDepList::const_iterator j = step->dependencies().begin();
             j != step->dependencies().end(); ++j)
        {
            log_debug("%s edge to %s", j->c_str(), step->name().c_str());
            edges[*j].push_back(step->name());
        }

        // parentless, so can be started at any time        
        if (step->dependencies().size() == 0)
        {
            step_stack.push_back(step);
        }
    }
    
    // Perform the DFS from each dependency-less node
    dfs_time_ = 0;
    while (step_stack.size() > 0)
    {
        InitStep* step = step_stack.back();
        step_stack.pop_back();        
        dfs(step, edges);
    }

#ifndef NDEBUG
    for (StepMap::iterator i = steps_.begin(); 
         i != steps_.end(); ++i)
    {
        InitStep* step = i->second;
        log_debug("step %s has time %d", step->name().c_str(), step->time_);
    }
#endif // NDEBUG

    return 0;
}
    
void 
InitSequencer::dfs(InitStep* step, ReverseDepEdges& edges)
{
    for (ReverseDepList::const_iterator i = edges[step->name()].begin();
         i != edges[step->name()].end(); ++i)
    {
        if (steps_[*i]->time_ == -1)
        {
            dfs(steps_[*i], edges);
        }
    }
    
    step->time_ = dfs_time_;
    ++dfs_time_;
}

void 
InitSequencer::mark_dep(const std::string& target)
{
    std::vector<InitStep*> step_stack;
    
    log_debug("target is %s", target.c_str());
    for (StepMap::iterator i = steps_.begin(); 
         i != steps_.end(); ++i)
    {
        i->second->mark_ = false;
    }

   
    ASSERT(steps_.find(target) != steps_.end());
   
    step_stack.push_back(steps_[target]);

    while (step_stack.size() > 0) 
    {
        InitStep* step = step_stack.back();
        step_stack.pop_back();

        if (!step->mark_) 
        {
            step->mark_ = true;
            log_debug("%s is a dependent step", step->name().c_str());
        }        

        for (InitStep::DepList::const_iterator i = step->dependencies().begin();
             i != step->dependencies().end(); ++i)
        {
            if (steps_.find(*i) == steps_.end())
            {
                PANIC("%s is dependent on %s which is bogus", 
                      step->name().c_str(), i->c_str());
            }
            
            if(!steps_[*i]->mark_)
            {
                step_stack.push_back(steps_[*i]);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// InitStep
//
InitStep::InitStep(const std::string& name)
    : done_(false),
      name_(name),
      mark_(false),
      time_(-1)
{
    Singleton<InitSequencer>::instance()->add_step(this);    
}

InitStep::InitStep(const std::string& name, int depsize, ...)
    : done_(false),
      name_(name),
      mark_(false),
      time_(-1)
{
    va_list ap;
    va_start(ap, depsize);
    for (int i=0; i<depsize; ++i)
    {
        dependencies_.push_back(va_arg(ap, const char*));
    }
    va_end(ap);
    
    Singleton<InitSequencer>::instance()->add_step(this);
}

InitStep::InitStep(const std::string& name, const DepList& deps)
    : done_(false),
      name_(name),
      dependencies_(deps),
      mark_(false),
      time_(-1)
{
    Singleton<InitSequencer>::instance()->add_step(this);
}

int
InitStep::run()
{ 
    int err = run_component(); 
    if (!err) 
    {
        done_ = true; 
    }

    return err;
}

bool
InitStep::dep_are_satisfied()
{
    bool sat = true;
    for (DepList::const_iterator i = dependencies_.begin();
         i != dependencies_.end(); ++i)
    {
        sat &= Singleton<InitSequencer>::instance()->get_step(*i)->done();
    }

    return sat;
}

} // namespace oasys
