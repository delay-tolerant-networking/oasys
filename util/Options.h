#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>
#include <vector>

/*
 * Wrapper class for getopt calls.
 */
class Opt;
class Options {
public:
    static void addopt(Opt* opt);
    static void getopt(const char* progname, int argc, char* const argv[]);
    static void usage(const char* progname);
    
protected:
    typedef std::vector<Opt*> List;
    
    static Opt* opts_[];	// indexed by option character
    static List allopts_;	// list of all options
};

class Opt {
    friend class Options;
    
protected:
    Opt(char shortopt, const char* longopt,
        void* valp, bool* setp, bool hasval,
        const char* valdesc, const char* desc);
    virtual ~Opt();
    
    virtual int set(char* val) = 0;
    
    char shortopt_;
    const char* longopt_;
    void* valp_;
    bool* setp_;
    bool hasval_;
    const char* valdesc_;
    const char* desc_;
    Opt*  next_;
};

class BoolOpt : public Opt {
public:
    BoolOpt(char shortopt, const char* longopt,
            bool* valp, const char* desc);
    BoolOpt(char shortopt, const char* longopt,
            bool* valp, bool* setp, const char* desc);

protected:
    int set(char* val);
};

class IntOpt : public Opt {
public:
    IntOpt(char shortopt, const char* longopt,
           int* valp,
           const char* valdesc, const char* desc);

    IntOpt(char shortopt, const char* longopt,
           int* valp, bool* setp,
           const char* valdesc, const char* desc);

protected:
    int set(char* val);
};

class StringOpt : public Opt {
public:
    StringOpt(char shortopt, const char* longopt,
              std::string* valp,
              const char* valdesc, const char* desc);
    
    StringOpt(char shortopt, const char* longopt,
              std::string* valp, bool* setp,
              const char* valdesc, const char* desc);

protected:
    int set(char* val);
};

#endif /* _OPTIONS_H_ */
