#ifndef __LOGCONFIGPARSER_H__
#define __LOGCONFIGPARSER_H__

#include "../util/RuleSet.h"

namespace oasys {

class LogConfigParser { 
public:
    struct Option {
        const char* option_str_;
        int         flag_value_;
    };

    /*! 
     * @param rs RuleSet to add parsed logging entries into.
     *
     * @param options Set of options to parse. End of options has
     * option_str_ == 0.
     *
     */
    LogConfigParser(const char* filename, RuleSet* rs, Option* opts);
    
    /*!
     * @return 0 if there is no errors in parsing the file.
     */
    int parse();

    /*!
     * @return flags Options that were set in the configuration file
     */
    int flags() { return flags_; }

private:
    const char* filename_;
    RuleSet*    rs_;
    Option*     opts_;
    int         flags_;
};

} // namespace oasys

#endif /* __LOGCONFIGPARSER_H__ */
