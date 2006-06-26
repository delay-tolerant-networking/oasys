#ifndef __GLOB_H__
#define __GLOB_H__

namespace oasys {

class Glob {
public:
    struct State {
        const char* pat_;
        const char* to_match_;
    };

    /*! Cheapo glob with a fixed size stack and no external memory
     *  usage. XXX/bowei - handle escapes. */
    static bool fixed_glob(const char* pat, const char* str);
};

} // namespace oasys

#endif /* __GLOB_H__ */
