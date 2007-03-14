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

#ifndef _OASYS_URI_H_
#define _OASYS_URI_H_

#include <string>

#include "../serialize/Serialize.h"

namespace oasys {

typedef enum uri_parse_err_t {
    URI_PARSE_OK,             /* valid URI */
    URI_PARSE_NO_URI,         /* no URI in object */
    URI_PARSE_NOT_PARSED,     /* URI not parsed yet */
    URI_PARSE_NO_SEP,         /* missing seperator char ':' */
    URI_PARSE_BAD_PERCENT,    /* invalid percent-encoded character */
    URI_PARSE_BAD_IP_LITERAL, /* invalid IP-literal encoding */
    URI_PARSE_BAD_IPV6,       /* invalid IPv6 address */
    URI_PARSE_BAD_SCHEME,     /* invalid scheme name */
    URI_PARSE_BAD_USERINFO,   /* invalide userinfo subcomponent */
    URI_PARSE_BAD_HOST,       /* invalid host subcomponent */
    URI_PARSE_BAD_PORT,       /* invalid port subcomponent */
    URI_PARSE_BAD_PATH,       /* invalid path component */
    URI_PARSE_BAD_QUERY,      /* invalid query component */
    URI_PARSE_BAD_FRAGMENT,   /* invalid fragment component */
    URI_PARSE_NO_PARAM_VAL,   /* missing value for cgi query parameter */
};

/**
 * Simple class for managing generic URIs based on RFC 3986.
 */
class URI : public SerializableObject {
public:
    /**
     * Default constructor.
     */
    URI(): port_include_(false), port_num_(0),
           parse_err_(URI_PARSE_NO_URI),
           validate_(true), normalize_(true) {}

    /**
     * Constructs a URI from the given std::string.
     */
    URI(const std::string& uri, bool validate = true):
        uri_(uri), port_include_(false), port_num_(0),
        validate_(validate), normalize_(true)
        { reset_parsing_status(); }

    /**
     * Constructs a URI from the given character string.
     */
    URI(const char* uri, bool validate = true):
        uri_(uri), port_include_(false), port_num_(0),
        validate_(validate), normalize_(true)
        { reset_parsing_status(); }

    /**
     * Deep copy constructor.
     */
    URI(const URI& uri):
        SerializableObject(uri),
        uri_(uri.uri_),
        scheme_(uri.scheme_),
        ssp_(uri.ssp_),
        authority_(uri.authority_),
        path_(uri.path_),
        query_(uri.query_),
        fragment_(uri.fragment_),
        userinfo_(uri.userinfo_),
        host_(uri.host_),
        port_(uri.port_),
        port_include_(uri.port_include_),
        port_num_(uri.port_num_),
        parse_err_(uri.parse_err_),
        validate_(uri.validate_),
        normalize_(uri.normalize_) {}

    /**
     * Destructor.
     */
    ~URI() {}

    /**
     * Parse the internal URI into its components, validate the URI
     * (if flagged to do so), and normalize the URI (if flagged to
     * do so).
     *
     * @return status code
     */
    uri_parse_err_t parse();

    /**
     * Set the URI to be the given std::string.
     */
    void assign(const std::string& str) {
             clear();
             uri_.assign(str);
             reset_parsing_status();
    }

    /**
     * Set the URI to be the given character string.
     */
    void assign(const char* str, size_t len) {
             clear();
             uri_.assign(str, len);
             reset_parsing_status();
    }

    /**
     * Set the URI to be the same as the given URI.
     */
    void assign(const URI& other) {
             clear();

             uri_          = other.uri_;
             scheme_       = other.scheme_;
             ssp_          = other.ssp_;
             authority_    = other.authority_;
             path_         = other.path_;
             query_        = other.query_;
             fragment_     = other.fragment_;
             userinfo_     = other.userinfo_;
             host_         = other.host_;
             port_         = other.port_;
             port_include_ = other.port_include_;
             port_num_     = other.port_num_;
             parse_err_    = other.parse_err_;
             validate_     = other.validate_;
             normalize_    = other.normalize_;
    }

    /**
     * Clear the URI components, and main URI string if not flagged otherwise.
     */
    void clear(bool clear_uri = true) {
             if (clear_uri)
                 uri_.erase();
             reset_parsing_status();

             scheme_.erase();
             ssp_.erase();
             authority_.erase();
             path_.erase();
             query_.erase();
             fragment_.erase();
             userinfo_.erase();
             host_.erase();
             port_.erase();
             port_include_ = false;
             port_num_ = true;
    }

    /**
     * Operator overload for equality operator.
     */
    bool operator==(const URI& other) const
             { return (uri_ == other.uri_); }

    /**
     * Operator overload for inequality operator.
     */
    bool operator!=(const URI& other) const
             { return (uri_ != other.uri_); }

    /**
     * Operator overload for less-than operator.
     */
    bool operator<(const URI& other) const
             { return (uri_ < other.uri_); }

    /**
     * Rebuild (and parse) the URI from the scheme and SSP components.
     *
     * @return true if the URI is valid, false if not.
     */
    uri_parse_err_t format() {
        uri_ = scheme_ + ":" + ssp_;
        return parse();
    }

    /**
     * Virtual from SerializableObject.
     */
    void serialize(SerializeAction* a)
             { a->process("uri", &uri_); }

    /**
     * Set validate and normalize flags.
     */
    void set_validate(bool validate = true)   { validate_ = validate; }
    void set_normalize(bool normalize = true) { normalize_ = normalize; }

    /**
     * Accessors
     */
    bool valid() const
             { return (parse_err_ == URI_PARSE_OK); }
    bool parsed() const
             { return ((parse_err_ != URI_PARSE_NO_URI) &&
                       (parse_err_ != URI_PARSE_NOT_PARSED)); }

    uri_parse_err_t    parse_status() const { return parse_err_; }

    bool               validate()     const { return validate_; }
    bool               normalize()    const { return normalize_; }

    const std::string& uri()          const { return uri_; }
    const std::string& scheme()       const { return scheme_; }
    const std::string& ssp()          const { return ssp_; }
    std::string*       ssp_ptr()            { return &ssp_; }

    const std::string& authority()    const { return authority_; }
    const std::string& userinfo()     const { return userinfo_; }
    const std::string& host()         const { return host_; }
    const std::string& port()         const { return port_; }
    const std::string& path()         const { return path_; }
    const std::string& query()        const { return query_; }
    const std::string& fragment()     const { return fragment_; }

    unsigned int       port_num()     const { return port_num_; }

    const char*        c_str()        const { return uri_.c_str(); }

private:
    /**
     * Parse URI scheme-specific parts (SSP) based on generic rules
     * defined in RFC 3986.
     *
     * @return status code
     */
    uri_parse_err_t parse_generic_ssp();
    uri_parse_err_t parse_authority();

    /**
     * Validate the URI components based on generic rules defined in RFC 3986.
     *
     * @return status code
     */
    uri_parse_err_t validate_scheme_name();
    uri_parse_err_t validate_userinfo();
    uri_parse_err_t validate_host();
    uri_parse_err_t validate_port();
    uri_parse_err_t validate_path();
    uri_parse_err_t validate_query();
    uri_parse_err_t validate_fragment();
    uri_parse_err_t validate_ip_literal(const std::string& host);
 
    /**
     * Normalize the URI to the standard format as described in RFC 3986
     * so that equivalency is more accurately determined.
     *
     * These methods should be called only when the uri_ string has been
     * successfully parsed and found to be valid. The uri_ and component
     * strings may be modified by these methods.
     */
    void normalize();
    void normalize_scheme();
    void normalize_authority();
    void normalize_path();
    void normalize_query();
    void normalize_fragment();

    /**
     * Decode percent-encoded characters in the URI SSP components.
     *
     * These methods should be called only when the uri_ string has been
     * successfully parsed and found to be valid. The uri_ and component
     * strings may be modified by these methods.
     */
    void decode_authority();
    void decode_path();
    void decode_query();
    void decode_fragment();

    /**
     * @return true if the character is an "unreserved" character as
     * defined by RFC 3986, false if not.
     */
    bool is_unreserved(char c);

    /**
     * @return true if the character is a "sub-delims" character as
     * defined by RFC 3986, false if not.
     */
    bool is_sub_delim(char c);

    /**
     * @return true if the character is a hexidecimal character, false if not.
     */
    bool is_hexdig(char c);

    /**
     * Reset the parsing status based on existence of URI string.
     */ 
    void reset_parsing_status() {
             if (uri_.empty()) {
                 parse_err_ = URI_PARSE_NO_URI;
             } else {
                 parse_err_ = URI_PARSE_NOT_PARSED;
             }
    }

    /**
     * Data Members
     */
    std::string     uri_;          // URI string

    std::string     scheme_;       // scheme component (not including ':')
    std::string     ssp_;          // scheme-specific part (SSP) components

    std::string     authority_;    // authority component
    std::string     path_;         // path component
    std::string     query_;        // query component
    std::string     fragment_;     // fragment component

    std::string     userinfo_;     // userinfo subcomponent of authority
    std::string     host_;         // host subcomponent of authority
    std::string     port_;         // port subcomponent of authority

    bool            port_include_; // port separator ':' included
    unsigned int    port_num_;     // port as integer; 0 if no port

    uri_parse_err_t parse_err_;    // parsing status

    bool            validate_;     // true if URI is to be validated
    bool            normalize_;    // true if URI is to be normalized
};

} // namespace oasys

#endif /* _OASYS_URI_H_ */
