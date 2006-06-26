#include "Glob.h"

namespace oasys {

bool
Glob::fixed_glob(const char* pat, const char* str)
{
    const int STACK_SIZE = 32;
    State stk[STACK_SIZE];
    int         stk_size = 1;

    stk[0].pat_      = pat;
    stk[0].to_match_ = str;

    while (stk_size > 0) 
    {
        int old_stk_size = stk_size;
        
        // Match the next character or '*'
        for (int i = 0; i < old_stk_size; ++i) 
        {
            State* state = &stk[i];
            
            switch (* (state->pat_)) 
            {
            case '*':
                if (*(state->pat_ + 1) == *state->to_match_)
                {
                    // Can start another state to match the rest of
                    // the string after the '*'
                    if (stk_size == STACK_SIZE) 
                    {
                        // On stack overflow, just return not matched
                        return false;
                    }
                    
                    stk[stk_size].pat_      = state->pat_ + 1;
                    stk[stk_size].to_match_ = state->to_match_;
                    stk_size++;
                }

                state->to_match_++;
                break;
                
            default:
                if (* (state->pat_) == *(state->to_match_)) 
                {
                    state->pat_++;
                    state->to_match_++;
                }
                else
                {
                    state->pat_      = "";
                    state->to_match_ = "NO_MATCH";
                }
                break;
            }
        }

        // Clean up the states
        old_stk_size = stk_size;
        for (int i = 0, j = 0; i < old_stk_size; i++) 
        {
            State* state = &stk[i];
            
            if ((*state->pat_ == '\0' && *state->to_match_ == '\0') ||
                (*state->pat_ == '*' && *(state->pat_ + 1) == '\0' && 
                 *state->to_match_ == '\0'))
            {
                return true;
            }

            if (*state->pat_ == '\0' || *state->to_match_ == '\0')
            {
                stk_size--;
                continue;
            }

            stk[j] = stk[i];
            j++;
        }
    }

    return false;
}

} // namespace oasys

#if 0

#include <cstdio>

void 
g(const char* pat, const char* str) 
{
    printf("%s:%s - %s\n", 
           pat, str, oasys::Glob::fixed_glob(pat, str) ? "match" : "no match");
}

int 
main()
{
    g("hello", "hello");
    g("hello", "world");
    g("hel*", "world"); 
    g("hel*", "helicopter"); 
    g("hel*lo*world", "hello there world");
    g("hel*", "hello, world!");
    g("hel*o", "hellow");
    g("*world", "hello cruel world");
}

#endif
