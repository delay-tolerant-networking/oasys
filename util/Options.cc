#include <unistd.h>
#include <getopt.h>

#include "Options.h"

Opt* Options::opts_[256];
Options::List Options::allopts_;

void
Options::addopt(Opt* opt)
{
    int c = opt->shortopt_;
    if (opts_[c]) {
        fprintf(stderr,
                "FATAL ERROR: multiple addopt calls for char '%c'\n", c);
        abort();
    }
        
    opts_[c] = opt;
    allopts_.push_back(opt);
}

void
Options::getopt(const char* progname, int argc, char* const argv[])
{
    Opt* opt;
    char short_opts[256];
    char* optstring = short_opts;
    int c, i;
    struct option* long_opts;

    int nopts = allopts_.size(); 

    // alloc two extra options -- one for help, one for all zeros
    long_opts = (struct option*) malloc(sizeof(struct option) * (nopts + 2));
    memset(long_opts, 0, sizeof(struct option) * (nopts + 2));
    
    for (i = 0; i < nopts; ++i)
    {
        opt = allopts_[i];
        
        if (opt->shortopt_) {
            *optstring++ = opt->shortopt_;
            if (opt->hasval_) {
                *optstring++ = ':';
            }
        }

        if (opt->longopt_) {
            long_opts[i].name = opt->longopt_;
            long_opts[i].has_arg = opt->hasval_;
        } else {
            // ignore this slot
            long_opts[i].name = "help";
        }
    }
                                        
    // tack on the help option
    *optstring++ = 'h';
    *optstring++ = 'H';
    long_opts[nopts].name = "help";
    
    while (1) {
        c = ::getopt_long(argc, argv, short_opts, long_opts, &i);
        switch(c) {
        case 0:
            if (!strcmp(long_opts[i].name, "help"))
            {
                usage(progname);
                exit(0);
            }

            opt = allopts_[i];

            if (opt->set(optarg) != 0) {
                fprintf(stderr, "invalid value '%s' for option '--%s'\n",
                        optarg, opt->longopt_);
                exit(1);
            }

            if (opt->setp_)
                *opt->setp_ = true;
            

            break;
        case ':':
            // missing value to option
            fprintf(stderr, "option %s requires a value\n", long_opts[i].name);
            usage(progname);
            exit(0);
            
        case '?':
        case 'h':
        case 'H':
            usage(progname);
            exit(0);
            
        case -1:
            // end of list
            return;
            
        default:
            if (c < 0 || c > 256) {
                fprintf(stderr, "FATAL ERROR: %d returned from getopt\n", c);
                abort();
            }
            opt = opts_[c];

            if (!opt) {
                fprintf(stderr, "unknown char '%c' returned from getopt\n", c);
                exit(1);
            }
                
            if (opt->set(optarg) != 0) {
                fprintf(stderr, "invalid value '%s' for option '-%c'\n",
                        optarg, c);
                exit(1);
            }
            
            if (opt->setp_)
                *opt->setp_ = true;
            
        }
    }
}

void
Options::usage(const char* progname)
{
    List::iterator iter;
    char opts[128];
    fprintf(stderr, "%s usage:\n", progname);

    snprintf(opts, sizeof(opts), "-h, --help");
    fprintf(stderr, "  %-24s%s\n", opts, "show usage");

    for (iter = allopts_.begin(); iter != allopts_.end(); ++iter)
    {
        Opt* opt = *iter;
        
        if (opt->shortopt_ && opt->longopt_)
        {
            snprintf(opts, sizeof(opts), "-%c, --%s %s",
                     opt->shortopt_, opt->longopt_, opt->valdesc_);
        }
        else if (opt->shortopt_)
        {
            snprintf(opts, sizeof(opts), "-%c %s",
                     opt->shortopt_, opt->valdesc_);
        } else {
            snprintf(opts, sizeof(opts), "    --%s %s",
                     opt->longopt_, opt->valdesc_);
        }
        fprintf(stderr, "  %-24s%s\n", opts, opt->desc_);
    }
}

Opt::Opt(char shortopt, const char* longopt,
         void* valp, bool* setp, bool hasval,
         const char* valdesc, const char* desc)
    : shortopt_(shortopt),
      longopt_(longopt),
      valp_(valp),
      setp_(setp),
      hasval_(hasval),
      valdesc_(valdesc),
      desc_(desc),
      next_(0)
{
    Options::addopt(this);
    if (setp) *setp = false;
}

Opt::~Opt()
{
    fprintf(stderr, "FATAL ERROR: opt classes should never be destroyed\n");
    abort();
}

BoolOpt::BoolOpt(char shortopt, const char* longopt,
                 bool* valp, const char* desc)
    : Opt(shortopt, longopt, valp, NULL, 0, "", desc)
{
}

BoolOpt::BoolOpt(char shortopt, const char* longopt,
                 bool* valp, bool* setp, const char* desc)
    : Opt(shortopt, longopt, valp, setp, 0, "", desc)
{
}

int
BoolOpt::set(char* val)
{
    *((bool*)valp_) = true;
    return 0;
}

IntOpt::IntOpt(char shortopt, const char* longopt,
               int* valp,
               const char* valdesc, const char* desc)
    : Opt(shortopt, longopt, valp, NULL, 1, valdesc, desc)
{
}

IntOpt::IntOpt(char shortopt, const char* longopt,
               int* valp, bool* setp,
               const char* valdesc, const char* desc)
    : Opt(shortopt, longopt, valp, setp, 1, valdesc, desc)
{
}

int
IntOpt::set(char* val)
{
    int newval;
    char* endptr = 0;

    newval = strtol(val, &endptr, 0);
    if (*endptr != '\0')
        return -1;

    *((int*)valp_) = newval;

    return 0;
}

StringOpt::StringOpt(char shortopt, const char* longopt,
                     std::string* valp,
                     const char* valdesc, const char* desc)
    : Opt(shortopt, longopt, valp, NULL, 1, valdesc, desc)
{
}

StringOpt::StringOpt(char shortopt, const char* longopt,
                     std::string* valp, bool* setp,
                     const char* valdesc, const char* desc)
    : Opt(shortopt, longopt, valp, setp, 1, valdesc, desc)
{
}

int
StringOpt::set(char* val)
{
    ((std::string*)valp_)->assign(val);
    return 0;
}
