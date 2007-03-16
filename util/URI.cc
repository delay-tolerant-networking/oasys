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

#include "URI.h"
#include "debug/Log.h"

namespace oasys {

//----------------------------------------------------------------------
uri_parse_err_t
URI::parse()
{
    static const char* log = "/oasys/util/uri/";

    clear(false);

    if (uri_.empty()) {
        log_debug_p(log, "URI::parse: empty URI string");
        return (parse_err_ = URI_PARSE_NO_URI);
    }

    size_t scheme_len;
    if ((scheme_len = uri_.find(':')) == std::string::npos) {
        log_debug_p(log, "URI::parse: no semicolon");
        return (parse_err_ = URI_PARSE_NO_SEP);
    }

    if (scheme_len == 0) {
        log_debug_p(log, "URI::parse: empty scheme name");
        return (parse_err_ = URI_PARSE_BAD_SCHEME);
    }

    scheme_.assign(uri_, 0, scheme_len);

    size_t ssp_start = scheme_len + 1; // skip semicolon
    size_t ssp_len   = uri_.length() - ssp_start;
    if (ssp_len > 0) {
        ssp_.assign(uri_, ssp_start, ssp_len);
    }

    uri_parse_err_t err;
    if (((err = parse_generic_ssp()) != URI_PARSE_OK) ||
        ((err = parse_authority()) != URI_PARSE_OK)) {
        return (parse_err_ = err);
    }

    if (validate_) {
        uri_parse_err_t err;
        if (((err = validate_scheme_name()) != URI_PARSE_OK) ||
            ((err = validate_userinfo()) != URI_PARSE_OK) ||
            ((err = validate_host()) != URI_PARSE_OK) ||
            ((err = validate_port()) != URI_PARSE_OK) ||
            ((err = validate_path()) != URI_PARSE_OK) ||
            ((err = validate_query()) != URI_PARSE_OK) ||
	    ((err = validate_fragment()) != URI_PARSE_OK)) {
            return (parse_err_ = err);
        }

        if (normalize_) {
            normalize();
        }
    }

    return (parse_err_ = URI_PARSE_OK);
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::parse_generic_ssp()
{
    static const char* log = "/oasys/util/uri/";

    if (ssp_.empty()) {
        log_debug_p(log, "URI::parse_generic_ssp: empty ssp");
        return URI_PARSE_OK;
    }

    size_t curr_pos = 0;

    // check for presence of authority component
    if ((ssp_.length() >= 2) && (ssp_.substr(0,2) == "//")) {
        size_t authority_end = ssp_.find_first_of("/?#", 2);
        if (authority_end == std::string::npos) {
            authority_end = ssp_.length();
        }

        size_t authority_start = curr_pos;
        size_t authority_len   = authority_end - authority_start;
        ASSERT(authority_len > 0);

        authority_.assign(ssp_, authority_start, authority_len);
	curr_pos = authority_end;
    }

    // path component is required (although it may be empty)
    if (curr_pos != ssp_.length()) {
        size_t path_end = ssp_.find_first_of("?#", curr_pos);
        if (path_end == std::string::npos) {
            path_end = ssp_.length();
        }

        size_t path_start = curr_pos;
        size_t path_len   = path_end - path_start;
        if (path_len > 0) {
            path_.assign(ssp_, path_start, path_len);
        }

        curr_pos = path_end;
    }

    // check for presence of query component
    if ((curr_pos != ssp_.length()) && (ssp_.at(curr_pos) == '?')) {
        size_t query_end = ssp_.find('#', curr_pos);
        if (query_end == std::string::npos) {
            query_end = ssp_.length();
        }

        size_t query_start = curr_pos;
        size_t query_len   = query_end - query_start;
        ASSERT(query_len > 0);

        query_.assign(ssp_, query_start, query_len);
        curr_pos = query_end;
    }

    // check for presence of fragment component
    if ((curr_pos != ssp_.length()) && (ssp_.at(curr_pos) == '#')) {
        size_t fragment_start = curr_pos;
        size_t fragment_len   = ssp_.length() - fragment_start;
        ASSERT(fragment_len > 0);
	
        fragment_.assign(ssp_, fragment_start, fragment_len);
        curr_pos = ssp_.length();
    }

    ASSERT(curr_pos == ssp_.length());

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::parse_authority()
{
    static const char* log = "/oasys/util/uri/";

    if (authority_.empty()) {
        return URI_PARSE_OK;
    }

    ASSERT(authority_.length() >= 2);
    ASSERT(authority_.substr(0, 2) == "//");

    size_t curr_pos = 2; // skip initial "//" characters

    // check for presence of user information subcomponent
    size_t userinfo_end = authority_.find('@', curr_pos);
    if (userinfo_end != std::string::npos) {
        size_t userinfo_len = (userinfo_end + 1) - curr_pos; // includes '@'
        userinfo_.assign(authority_, curr_pos, userinfo_len);

        curr_pos = userinfo_end + 1; // skip the @ character
    }

    // host subcomponent is required (although it may be empty)
    if (curr_pos != authority_.length()) {
        size_t host_end;
        if (authority_.at(curr_pos) == '[') {
            host_end = authority_.find(']', curr_pos);
            if (host_end == std::string::npos) {
                log_debug_p(log, "URI::parse_authority: "
                            "literal host component must end with ']'");
                return URI_PARSE_BAD_IP_LITERAL;
            }
            host_end++; // include '[' character
        } else {
            host_end = authority_.find(':', curr_pos);
            if (host_end == std::string::npos) {
                host_end = authority_.length();
            }
        }

        size_t host_len = host_end - curr_pos;
        if (host_len > 0) {
            host_.assign(authority_, curr_pos, host_len);
        }

        curr_pos = host_end;
    }

    // check for presence of port subcomponent
    if (curr_pos != authority_.length()) {
        if (authority_.at(curr_pos) != ':') {
            log_debug_p(log, "URI::parse_authority: "
                        "semicolon expected prior to port");
            return URI_PARSE_BAD_PORT;
        }
        port_include_ = true;
	
        size_t port_start = curr_pos + 1; // skip ':' character
        size_t port_len   = authority_.length() - port_start;
        if (port_len > 0) {
            port_.assign(authority_, port_start, port_len);
        }

        curr_pos = authority_.length();
    }

    ASSERT(curr_pos == authority_.length());

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
void
URI::normalize()
{
    ASSERT(normalize_);

    normalize_scheme();
    normalize_authority();
    normalize_path();
    normalize_query();
    normalize_fragment();

    ssp_ = authority_ + path_ + query_ + fragment_;
    uri_ = scheme_ + ":" + ssp_;

    log_debug_p("/oasys/util/uri/", "URI::normalize: "
                "normalized URI %s", uri_.c_str());
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::validate_scheme_name()
{
    static const char* log = "/oasys/util/uri/";

    std::string::iterator iter = scheme_.begin();

    if (!isalpha(*iter)) {
        log_debug_p(log, "URI::validate_scheme_name: "
                    "first character is not a letter %c", (*iter));
        return URI_PARSE_BAD_SCHEME;
    }
    ++iter;

    for(; iter != scheme_.end(); ++iter) {
        char c = *iter;
        if (isalnum(c) || (c == '+') || (c == '-') || (c == '.'))
            continue;

        log_debug_p(log, "URI::validate_scheme_name: "
                    "invalid character in scheme name %c", c);
        return URI_PARSE_BAD_SCHEME;
    }

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
void
URI::normalize_scheme()
{
    // alpha characters should be lowercase within the scheme name
    for (unsigned int i = 0; i < scheme_.length(); ++i) {
        char c = scheme_.at(i);
        if (isalpha(c) && isupper(c)) {
            scheme_.replace(i, 1, 1, tolower(c));
        }
    }
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::validate_userinfo()
{
    static const char* log = "/oasys/util/uri/";

    if (userinfo_.empty()) {
        return URI_PARSE_OK;
    }

    ASSERT(userinfo_.at(userinfo_.length() - 1) == '@');

    // check for valid characters
    for (unsigned int i = 0; i < (userinfo_.length() - 1); ++i) {
        char c = userinfo_.at(i);
        if (is_unreserved(c) ||
            is_sub_delim(c)  ||
            (c == ':')) {
            continue;
        }

        // check for percent-encoded characters
        if (c == '%') {
            if (i + 2 >= (userinfo_.length() - 1)) {
                log_debug_p(log, "URI::validate_userinfo: "
                            "invalid percent-encoded length in userinfo");
                return URI_PARSE_BAD_PERCENT;
            }

            if (!is_hexdig(userinfo_.at(i + 1)) ||
                !is_hexdig(userinfo_.at(i + 2))) {
                log_debug_p(log, "URI::validate_userinfo: "
                            "invalid percent-encoding in userinfo");
                return URI_PARSE_BAD_PERCENT;
            }

            i += 2;
            continue;
        }

        log_debug_p(log, "URI::validate_userinfo: "
                    "invalid character in userinfo %c", c);
        return URI_PARSE_BAD_USERINFO;
    }

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::validate_host()
{
    static const char* log = "/oasys/util/uri/";

    if (host_.empty()) {
        return URI_PARSE_OK;
    }

    // check for IP-literal
    if (host_.at(0) == '[') {
        ASSERT(host_.at(host_.length() - 1) == ']');

        std::string host(host_, 1, (host_.length() - 2));
        return validate_ip_literal(host);
    }

    // check for valid characters
    for (unsigned int i = 0; i < host_.length(); ++i) {
        char c = host_.at(i);
        if (is_unreserved(c) ||
            is_sub_delim(c)) {
            continue;
        }

        // check for percent-encoded characters
        if (c == '%') {
            if (i + 2 >= host_.length()) {
                log_debug_p(log, "URI::validate_host: "
                            "invalid percent-encoded length in host");
                return URI_PARSE_BAD_PERCENT; 
            }

            if (!is_hexdig(host_.at(i + 1)) ||
                !is_hexdig(host_.at(i + 2))) {
                log_debug_p(log, "URI::validate_host: "
                            "invalid percent-encoding in host");
                return URI_PARSE_BAD_PERCENT;
            }

            i += 2;
            continue;
        }

        log_debug_p(log, "URI::validate_host: "
                    "invalid character in host %c", c);
        return URI_PARSE_BAD_HOST;
    }

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::validate_port()
{
    static const char* log = "/oasys/util/uri/";

    if (port_.empty()) {
        return URI_PARSE_OK;
    }

    // check for valid characters
    for (unsigned int i = 0; i < port_.length(); ++i) {
        char c = port_.at(i);
        if (isdigit(c)) {
            continue;
        }

        log_debug_p(log, "URI::validate_port: "
                    "invalid character in port %c", c);
        return URI_PARSE_BAD_PORT;
    }

    port_num_ = atoi(port_.c_str());

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
void
URI::normalize_authority()
{
    decode_authority();

    // alpha characters should be lowercase within the host subcomponent
    for (unsigned int i = 0; i < host_.length(); ++i) {
        char c = host_.at(i);

        if (c == '%') { 
            i += 2;
            continue;
        }
	
        if (isalpha(c) && isupper(c)) {
            host_.replace(i, 1, 1, tolower(c));
        }
    }

    if (!authority_.empty()) {
        if (port_include_) {
            authority_ = "//" + userinfo_ + host_ + ":" + port_;
        } else {
            authority_ = "//" + userinfo_ + host_;
        }
    }
}

//----------------------------------------------------------------------
void
URI::decode_authority()
{

    // decode percent-encoded characters within userinfo subcomponent 
    size_t p = 0, curr_pos = 0;
    std::string decoded_userinfo;
    while ((p < userinfo_.length()) &&
          ((p = userinfo_.find('%', p)) != std::string::npos)) {

        ASSERT((p + 2) < userinfo_.length());
        std::string hex_string = userinfo_.substr(p + 1, 2);

        int hex_value;
        sscanf(hex_string.c_str(), "%x", &hex_value);
        char c = (char)hex_value;

        // skip "unallowed" characters
        if (!is_unreserved(c) &&
            !is_sub_delim(c)  && 
            (c != ':')) {

            c = userinfo_.at(p + 1);
            if (isalpha(c) && islower(c)) {
                userinfo_.replace(p + 1, 1, 1, toupper(c));
            }
            c = userinfo_.at(p + 2);
            if (isalpha(c) && islower(c)) {
                userinfo_.replace(p + 2, 1, 1, toupper(c));
            }
		
            p += 3;
            continue;
        }

        // change "allowed" characters from hex value to alpha character
        decoded_userinfo.append(userinfo_, curr_pos, p - curr_pos);
        decoded_userinfo.append(1, c);

        p += 3;
        curr_pos = p;
    }

    if (!decoded_userinfo.empty()) {
        ASSERT(curr_pos <= userinfo_.length());
        decoded_userinfo.append(userinfo_, curr_pos,
                                userinfo_.length() - curr_pos);
        userinfo_ = decoded_userinfo;
    }

    // decode percent-encoded characters within host subcomponent 
    p = 0; curr_pos = 0;
    std::string decoded_host;
    while ((p < host_.length()) &&
          ((p = host_.find('%', p)) != std::string::npos)) {

        ASSERT((p + 2) < host_.length());
        std::string hex_string = host_.substr(p + 1, 2);

        int hex_value;
        sscanf(hex_string.c_str(), "%x", &hex_value);
        char c = (char)hex_value;
	
        // skip "unallowed" characters
        if (!is_unreserved(c) &&
            !is_sub_delim(c)) {

            c = host_.at(p + 1);
            if (isalpha(c) && islower(c)) {
                host_.replace(p + 1, 1, 1, toupper(c));
            }
            c = host_.at(p + 2);
            if (isalpha(c) && islower(c)) {
                host_.replace(p + 2, 1, 1, toupper(c));
            }
		
            p += 3;
            continue;
        }

        // change "allowed" characters from hex value to alpha character
        decoded_host.append(host_, curr_pos, p - curr_pos);
        decoded_host.append(1, c);

        p += 3;
        curr_pos = p;
    }

    if (!decoded_host.empty()) {
        ASSERT(curr_pos <= host_.length());
        decoded_host.append(host_, curr_pos, host_.length() - curr_pos);
        host_ = decoded_host;
    }
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::validate_ip_literal(const std::string& host)
{
    static const char* log = "/oasys/util/uri/";

    if (host.empty()) {
        log_debug_p(log, "URI::validate_ip_literal: empty host");
        return URI_PARSE_BAD_IP_LITERAL;
    }

    size_t curr_pos = 0;

    if ((host.at(curr_pos) == 'v') || (host.at(curr_pos) == 'V')) {
        ++curr_pos; // skip version character

        if ((curr_pos == host.length()) || !is_hexdig(host.at(curr_pos))) {
            log_debug_p(log, "URI::validate_ip_literal: "
                        "hexidecimal version expected");
            return URI_PARSE_BAD_IP_LITERAL;
        }
        ++curr_pos; // skip first hexidecimal character

        for ( ; curr_pos != host.length(); ++curr_pos) {
            if (!is_hexdig(host.at(curr_pos))) {
                break;
            }
        }

        if ((curr_pos == host.length()) || (host.at(curr_pos) != '.')) {
            log_debug_p(log, "URI::validate_ip_literal: "
                        "period character expected");
            return URI_PARSE_BAD_IP_LITERAL;
        }
        ++curr_pos; // skip period character

        if (curr_pos == host.length()) {
            log_debug_p(log, "URI::validate_ip_literal: "
                        "additional character expected");
            return URI_PARSE_BAD_IP_LITERAL;
        }

        for ( ; curr_pos < host.length(); ++curr_pos) {
            char c = host.at(curr_pos);
            if (is_unreserved(c) ||
                is_sub_delim(c)  ||
                (c == ':')) {
                continue;
            }

            log_debug_p(log, "URI::validate_ip_literal: "
                        "invalid character in IP literal %c", c);
            return URI_PARSE_BAD_IP_LITERAL;
        }

        ASSERT(curr_pos == host.length());
        return URI_PARSE_OK;
    }

    int num_pieces = 0;
    bool double_colon = false, prev_double_colon = false;
    size_t decimal_start;

    while (true) {
        decimal_start = curr_pos;
        unsigned int num_hexdig;
        for (num_hexdig = 0;
             num_hexdig < 4 && curr_pos < host.length();
             ++num_hexdig, ++curr_pos) {
            if (!is_hexdig(host.at(curr_pos))) {
                break;
            }
        }
        ++num_pieces;

        if (curr_pos == host.length()) {
            if (num_hexdig == 0) {
                if (!prev_double_colon) {
                    log_debug_p(log, "URI::validate_ip_literal: "
                                "ip literal must not end in single colon");
                    return URI_PARSE_BAD_IPV6;
                }
                --num_pieces;
            }
            break;
        }

        prev_double_colon = false;

        if (host.at(curr_pos) == ':') {
            if (num_hexdig == 0) {
                if (num_pieces == 1) {
                    if (((curr_pos + 1) == host.length()) ||
                        (host.at(curr_pos + 1) != ':')) {
                        log_debug_p(log, "URI::validate_ip_literal: "
                                    "double colon or hexidecimal "
                                    "character expected");
                        return URI_PARSE_BAD_IPV6;
                    }
                    ++curr_pos; // skip first colon character
                }

                if (double_colon) {
                    log_debug_p(log, "URI::validate_ip_literal: "
                                "multiple double colon's not allowed");
                    return URI_PARSE_BAD_IPV6;
                }

                double_colon = true;
                prev_double_colon = true;
            }

            ++curr_pos; // skip colon character

        } else if (host.at(curr_pos) == '.') {
            if (num_hexdig == 0) {
                log_debug_p(log, "URI::validate_ip_literal: "
                            "period must only follow decimal character");
                return URI_PARSE_BAD_IPV6;
            }
            --num_pieces;
            break;

        } else {
            log_debug_p(log, "URI::validate_ip_literal: "
                        "colon or period character expected");
            return URI_PARSE_BAD_IPV6;
        }
    }

    if (curr_pos != host.length()) {
        if ((num_pieces <= 0) ||
            (double_colon && (num_pieces > 6)) ||
            (!double_colon && (num_pieces != 6))) {
            log_debug_p(log, "URI::validate_ip_literal: "
                        "invalid number of hexidecimal encoded pieces, "
                        "cannot read IPv4 address");
            return URI_PARSE_BAD_IPV6;
        }

        curr_pos = decimal_start;

        for (unsigned int i = 0; i < 4; ) {

            char digit[4] = {0};
            unsigned int num_digit = 0;
            for ( ; num_digit < 3 && curr_pos < host.length();
                    ++num_digit, ++curr_pos) {
                if (!isdigit(host.at(curr_pos))) {
                    break;
                }
                digit[num_digit] = host.at(curr_pos);
            }
            digit[num_digit] = '\0';

            if (num_digit == 0) {
                log_debug_p(log, "URI::validate_ip_literal: "
                            "decimal character expected");
                return URI_PARSE_BAD_IPV6;
            }

            if ((num_digit > 1) && (digit[0] == '0')) {
                log_debug_p(log, "URI::validate_ip_literal: "
                            "leading zeros not permitted");
                return URI_PARSE_BAD_IPV6;
            }

            int num = atoi(digit);
            if ((num < 0) || (num > 255)) {
                log_debug_p(log, "URI::validate_ip_literal: "
                            "invalid decimal octet");
                return URI_PARSE_BAD_IPV6;
            }

	    if (++i == 4) {
                if (curr_pos != host.length()) {
                    log_debug_p(log, "URI::validate_ip_literal: "
                                "end of host expected");
                    return URI_PARSE_BAD_IPV6;
                }
                break;
            }

	    if ((curr_pos == host.length()) || (host.at(curr_pos) != '.')) {
                log_debug_p(log, "URI::validate_ip_literal: "
                            "period character expected");
                return URI_PARSE_BAD_IPV6;
            }
            ++curr_pos; // skip period character
        }

	num_pieces += 2;
    }

    ASSERT(curr_pos == host.length());

    if ((num_pieces <= 0) ||
        (double_colon && (num_pieces > 8)) ||
        (!double_colon && (num_pieces != 8))) {
        log_debug_p(log, "URI::validate_ip_literal: "
                    "invalid number of hexidecimal encoded pieces %d",
                    num_pieces);
        return URI_PARSE_BAD_IPV6;
    }

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::validate_path()
{
    static const char* log = "/oasys/util/uri/";

    if (path_.empty()) {
        return URI_PARSE_OK;
    }

    if (!authority_.empty()) {
        ASSERT(path_.at(0) == '/');
    }

    if (authority_.empty() && (path_.length() >= 2)) {
        ASSERT(path_.substr(0, 2) != "//");
    }

    // check for valid characters
    for (unsigned int i = 0; i < path_.length(); ++i) {
        char c = path_.at(i);
        if (is_unreserved(c) ||
            is_sub_delim(c)  ||
            (c == '/')       ||
            (c == ':')       ||
            (c == '@')) {
            continue;
        }

        // check for percent-encoded characters
        if (c == '%') {
            if (i + 2 >= path_.length()) {
                log_debug_p(log, "URI::validate_path: "
                            "invalid percent-encoded length in path");
                return URI_PARSE_BAD_PERCENT;
            }

            if (!is_hexdig(path_.at(i + 1)) ||
                !is_hexdig(path_.at(i + 2))) {
                log_debug_p(log, "URI::validate_path: "
                            "invalid percent-encoding in path");
                return URI_PARSE_BAD_PERCENT;
            }

            i += 2;
            continue;
        }

        log_debug_p(log, "URI:validate_path: "
                    "invalid character in path component %c", c);
        return URI_PARSE_BAD_PATH;
    }

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
void
URI::normalize_path()
{
    decode_path();

    // resolve '.' and '..' path segments

    size_t checkpoint = 0, dot_segment;
    while ((dot_segment = path_.find("./", checkpoint)) != std::string::npos) {

        if ((dot_segment == 0) || (path_.at(dot_segment - 1) == '/')) {
            path_.erase(dot_segment, 2);
            continue;
        }

        ASSERT(dot_segment >= 1);
        if (path_.at(dot_segment - 1) == '.') {
            if (dot_segment == 1) {
                path_.erase(dot_segment - 1, 3);
                continue;
            }

            ASSERT(dot_segment >= 2);
            if (path_.at(dot_segment - 2) == '/') {
                if (dot_segment == 2) {
                    path_.erase(dot_segment - 1, 3);
                    continue;
                }

                ASSERT(dot_segment >= 3);
                size_t prev_seg = path_.find_last_of('/', dot_segment - 3);
                if (prev_seg == std::string::npos) {
                    prev_seg = 0;
                }

                size_t erase_length = (dot_segment + 1) - prev_seg;
                path_.erase(prev_seg, erase_length);
                checkpoint = prev_seg;
                continue;
            }
        }

        checkpoint = dot_segment + 2;
    }

    if ((path_.length() == 1) && (path_.at(0) == '.')) {
        path_.erase(0, 1);

    } else if ((path_.length() == 2) && (path_.substr(0, 2) == "..")) {
        path_.erase(0, 2);

    } else if ((path_.length() >= 2) &&
               (path_.substr(path_.length() - 2, 2) == "/.")) {
        path_.erase(path_.length() - 1, 1);

    } else if ((path_.length() >= 3) &&
               (path_.substr(path_.length() - 3, 3) == "/..")) {
        if (path_.length() == 3) {
            path_.erase(path_.length() - 2, 2);
        } else {
            size_t prev_seg = path_.find_last_of('/', path_.length() - 4);
            if (prev_seg == std::string::npos) {
                prev_seg = 0;
            }

            size_t erase_length = path_.length() - prev_seg;
            path_.erase(prev_seg, erase_length);
            path_.append(1, '/');
        }
    }
}

//----------------------------------------------------------------------
void
URI::decode_path()
{
    std::string decoded_path;

    size_t p = 0, curr_pos = 0;
    while ((p < path_.length()) &&
          ((p = path_.find('%', p)) != std::string::npos)) {

        ASSERT((p + 2) < path_.length());
        std::string hex_string = path_.substr(p + 1, 2);

        int hex_value;
        sscanf(hex_string.c_str(), "%x", &hex_value);
        char c = (char)hex_value;

        // skip "unallowed" character
        if (!is_unreserved(c) &&
            !is_sub_delim(c)  && 
            (c != ':')        && 
            (c != '@')) {

            c = path_.at(p + 1);
            if (isalpha(c) && islower(c)) {
                path_.replace(p + 1, 1, 1, toupper(c));
            }
            c = path_.at(p + 2);
            if (isalpha(c) && islower(c)) {
                path_.replace(p + 2, 1, 1, toupper(c));
            }
		
            p += 3;
            continue;
        }

        // change "allowed" character from hex value to alpha character
        decoded_path.append(path_, curr_pos, p - curr_pos);
        decoded_path.append(1, c);

        p += 3;
        curr_pos = p;
    }

    if (!decoded_path.empty()) {
        ASSERT(curr_pos <= path_.length());
        decoded_path.append(path_, curr_pos, path_.length() - curr_pos);
        path_ = decoded_path;
    }
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::validate_query()
{
    static const char* log = "/oasys/util/uri/";

    if (query_.empty()) {
        return URI_PARSE_OK;
    }

    ASSERT(query_.at(0) == '?');

    // check for valid characters
    unsigned int i = 1; // skip initial question mark character
    for ( ; i < query_.length(); ++i) {
        char c = query_.at(i);
        if (is_unreserved(c) ||
            is_sub_delim(c)  ||
            (c == ':')       ||
            (c == '@')       ||
            (c == '/')       ||
            (c == '?')) {
            continue;
        }

        // check for percent-encoded characters
        if (c == '%') {
            if (i + 2 >= query_.length()) {
                log_debug_p(log, "URI::validate_query: "
                            "invalid percent-encoded length in query");
                return URI_PARSE_BAD_PERCENT;
            }

            if (!is_hexdig(query_.at(i + 1)) ||
                !is_hexdig(query_.at(i + 2))) {
                log_debug_p(log, "URI::validate_query: "
                            "invalid percent-encoding in query");
                return URI_PARSE_BAD_PERCENT;
            }

            i += 2;
            continue;
        }

        log_debug_p(log, "URI::validate_query: "
                    "invalid character in query component %c", c);
        return URI_PARSE_BAD_QUERY;
    }

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
void
URI::normalize_query()
{
    decode_query();
}

//----------------------------------------------------------------------
void
URI::decode_query()
{
    std::string decoded_query;

    size_t p = 0, curr_pos = 0;
    while ((p < query_.length()) &&
          ((p = query_.find('%', p)) != std::string::npos)) {

        ASSERT((p + 2) < query_.length());
        std::string hex_string = query_.substr(p + 1, 2);

        int hex_value;
        sscanf(hex_string.c_str(), "%x", &hex_value);
        char c = (char)hex_value;

        // skip "unallowed" character
        if (!is_unreserved(c) &&
            !is_sub_delim(c)  && 
            (c != ':')        && 
            (c != '@')        && 
            (c != '/')        && 
            (c != '?')) {

            c = query_.at(p + 1);
            if (isalpha(c) && islower(c)) {
                query_.replace(p + 1, 1, 1, toupper(c));
            }
            c = query_.at(p + 2);
            if (isalpha(c) && islower(c)) {
                query_.replace(p + 2, 1, 1, toupper(c));
            }
		
            p += 3;
            continue;
        }

        // change "allowed" character from hex value to alpha character
        decoded_query.append(query_, curr_pos, p - curr_pos);
        decoded_query.append(1, c);

        p += 3;
        curr_pos = p;
    }

    if (!decoded_query.empty()) {
        ASSERT(curr_pos <= query_.length());
        decoded_query.append(query_, curr_pos, query_.length() - curr_pos);
        query_ = decoded_query;
    }
}

//----------------------------------------------------------------------
uri_parse_err_t
URI::validate_fragment()
{
    static const char* log = "/oasys/util/uri/";

    if (fragment_.empty()) {
        return URI_PARSE_OK;
    }

    ASSERT(fragment_.at(0) == '#');

    // check for valid characters
    unsigned int i = 1; // skip initial number character    
    for ( ; i < fragment_.length(); ++i) {
        char c = fragment_.at(i);
        if (is_unreserved(c) ||
            is_sub_delim(c)  ||
            (c == ':')       ||
            (c == '@')       ||
            (c == '/')       ||
            (c == '?')) {
            continue;
        }

        // check for percent-encoded characters
        if (c == '%') {
            if (i + 2 >= fragment_.length()) {
                log_debug_p(log, "URI::validate_fragment: "
                            "invalid percent-encoded length in fragment");
                return URI_PARSE_BAD_PERCENT;
            }

            if (!is_hexdig(fragment_.at(i + 1)) ||
                !is_hexdig(fragment_.at(i + 2))) {
                log_debug_p(log, "URI::validate_fragment: "
                            "invalid percent-encoding in fragment");
                return URI_PARSE_BAD_PERCENT;
            }

            i += 2;
            continue;
        }

        log_debug_p(log, "URI::validate_fragment: "
                    "invalid character in fragment component %c", c);
        return URI_PARSE_BAD_FRAGMENT;
    }

    return URI_PARSE_OK;
}

//----------------------------------------------------------------------
void
URI::normalize_fragment()
{
    decode_fragment();
}

//----------------------------------------------------------------------
void
URI::decode_fragment()
{
    std::string decoded_fragment;

    size_t p = 0, curr_pos = 0;
    while ((p < fragment_.length()) &&
          ((p = fragment_.find('%', p)) != std::string::npos)) {

        ASSERT((p + 2) < fragment_.length());
        std::string hex_string = fragment_.substr(p + 1, 2);

        int hex_value;
        sscanf(hex_string.c_str(), "%x", &hex_value);
        char c = (char)hex_value;

        // skip unallowed character
        if (!is_unreserved(c) &&
            !is_sub_delim(c)  && 
            (c != ':')        && 
            (c != '@')        && 
            (c != '/')        && 
            (c != '?')) {

            c = fragment_.at(p + 1);
            if (isalpha(c) && islower(c)) {
                fragment_.replace(p + 1, 1, 1, toupper(c));
            }
            c = fragment_.at(p + 2);
            if (isalpha(c) && islower(c)) {
                fragment_.replace(p + 2, 1, 1, toupper(c));
            }
		
            p += 3;
            continue;
        }

        // change "allowed" character from hex value to alpha character
        decoded_fragment.append(fragment_, curr_pos, p - curr_pos);
        decoded_fragment.append(1, c);

        p += 3;
        curr_pos = p;
    }

    if (!decoded_fragment.empty()) {
        ASSERT(curr_pos <= fragment_.length());
        decoded_fragment.append(fragment_, curr_pos,
                                fragment_.length() - curr_pos);
        fragment_ = decoded_fragment;
    }
}

//----------------------------------------------------------------------
bool
URI::is_unreserved(char c)
{
    return (isalnum(c) ||
            (c == '-') ||
            (c == '.') ||
            (c == '_') ||
            (c == '~'));
}

//----------------------------------------------------------------------
bool
URI::is_sub_delim(char c)
{
    return ((c == '!') ||
            (c == '$') ||
            (c == '&') ||
            (c == '\'') ||
            (c == '(') ||
            (c == ')') ||
            (c == '*') ||
            (c == '+') ||
            (c == ',') ||
            (c == ';') ||
            (c == '='));
}

//----------------------------------------------------------------------
bool
URI::is_hexdig(char c)
{
    return (isdigit(c) ||
            (c == 'a') || (c == 'A') ||
            (c == 'b') || (c == 'B') ||
            (c == 'c') || (c == 'C') ||
            (c == 'd') || (c == 'D') ||
            (c == 'e') || (c == 'E') ||
            (c == 'f') || (c == 'F'));
}

} // namespace oasys
