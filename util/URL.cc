/*
 *    Copyright 2004-2006 Intel Corporation
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


#include "URL.h"
#include "StringBuffer.h"
#include "debug/DebugUtils.h"

namespace oasys {

urlerr_t
URL::parse_internal()
{
    size_t beg, end;

    // reset all fields
    proto_.erase();
    host_.erase();
    port_ = 0;
    path_.erase();

    // extract protocol
    if ((end = url_.find(':')) == std::string::npos)
        return URLPARSE_BADSEP;
    
    // skip the separator
    if (url_[end+1] != '/' || url_[end+2] != '/')
        return URLPARSE_BADPROTO;

    // valid protocol -- assign it and skip
    proto_.assign(url_, 0, end);
    beg = end + 3;

    // check for no host
    if (beg == url_.length())
        return URLPARSE_NOHOST;

    // search for the hostname/path separator
    end = url_.find_first_of(":/", beg, 2);

    if (end == std::string::npos) {
        // there's no port or pathname, it's just a host name, but
        // append a / for the canonical form
        host_.assign(url_, beg, url_.length());
        url_.push_back('/');
        return URLPARSE_OK;
    }

    // grab the hostname
    host_.assign(url_, beg, end - beg);

    // parse the port if it exists
    if (url_[end] == ':') {
        char* endstr;
        beg = end + 1;
        end = url_.find('/', beg);
        if (end == std::string::npos) {
            end = url_.length();

            // again, adjust for no trailing slash
            url_.push_back('/');
        }
        
        std::string portstr(url_, beg, end - beg);
        u_int32_t portval = strtoul(portstr.c_str(), &endstr, 10);
        
        if (portval > 65535)
            return URLPARSE_BADPORT;

        if (endstr != (portstr.c_str() + portstr.length()))
            return URLPARSE_BADPORT;

        port_ = portval;

        beg = end + 1;
    } else {
        // otherwise just skip past the slash separator
        ASSERT(url_[end] == '/');
        beg = end + 1;
    }

    // check for an empty path
    if (beg >= url_.length())
        return URLPARSE_OK;

    // find a ? to see if we need to parse params
    end = url_.find('?', beg);
    if (end == std::string::npos) {
        // if there are no parameters, then just store the path 
        path_.assign(url_, beg, url_.length() - beg);
        return URLPARSE_OK;
    }

    // assign the path part
    path_.assign(url_, beg, end - beg);

    // start parsing parameters
    beg = end + 1;
    while (1) {
        end = url_.find('=', beg);
        if (end == std::string::npos) {
            return URLPARSE_NOPARAMVAL;
        }
        
        params_.push_back(url_.substr(beg, end - beg));
        beg = end + 1;
        end = url_.find('&', beg);
        if (end == std::string::npos) {
            params_.push_back(url_.substr(beg, url_.length() - beg));
            break;
        } else {
            params_.push_back(url_.substr(beg, end - beg));
        }
        beg = end + 1;
    }

    return URLPARSE_OK;
}

urlerr_t
URL::parse()
{
    err_ = parse_internal();
    return err_;
}

void
URL::format(const std::string& proto,
            const std::string& host,
            const u_int16_t port,
            const std::string& path,
            const std::vector<std::string>& params)
{
    StringBuffer buf;
    
    proto_ = proto;
    host_ = host;
    port_ = port;
    path_ = path;
    params_.insert(params_.begin(), params.begin(), params.end());

    buf.append(proto);
    buf.append("://");
    buf.append(host);

    if (port != 0) {
        buf.appendf(":%d", port);
    }

    if (path[0] != '/')
        buf.append('/');

    buf.append(path);

    for (size_t i = 0; i < params.size(); i += 2) {
        buf.appendf("%c%s=%s", (i == 0) ? '?' : '&',
                    params[i].c_str(), params[i + 1].c_str());
    }
    
    url_.assign(buf.data(), buf.length());

    err_ = URLPARSE_OK;
}

} // namespace oasys
