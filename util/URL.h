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
// XXX/demmer add copyright
#ifndef _URL_H_
#define _URL_H_

// XXX/namespace
class URL;
namespace oasys {
typedef ::URL URL;
}

#include <list>
#include <string>

typedef enum urlerr_t {
    URLPARSE_OK,	/* parsed ok */
    URLPARSE_UNPARSED,	/* not parsed yet */
    URLPARSE_NOURL,	/* no url in object */
    URLPARSE_BADSEP,	/* bad or missing separator char */
    URLPARSE_BADPROTO,	/* bad protocol */
    URLPARSE_BADPORT,	/* bad port */
    URLPARSE_NOHOST,	/* no host */
};


/**
 * A simple class for managing urls that supports parsing, merging,
 * copying, etc.
 */
class URL {
public:
    /**
     * Default constructor
     */
    URL()
    {
        clear();
    }

    /**
     * Construct the url from the given std::string.
     */
    URL(const std::string& url)
    {
        url_.assign(url.data(), url.length());
        parse();
    }

    /**
     * Construct the url from the given std::string.
     */
    URL(const char* url)
    {
        url_.assign(url);
        parse();
    }

    /**
     * Deep copy constructor.
     */
    URL(const URL& copy)
        : url_(copy.url_),
          proto_(copy.proto_),
          host_(copy.host_),
          port_(copy.port_),
          path_(copy.path_),
          err_(copy.err_)
    {
    }

    /**
     * Clear out this url.
     */
    void clear()
    {
        url_.erase();
        err_ = URLPARSE_UNPARSED;
    }

    /**
     * Parse the internal url_ into its constituent parts.
     */
    urlerr_t parse();

    /**
     * Parse the internal url_ into its constituent parts.
     */
    urlerr_t parse(const std::string& url)
    {
        url_.assign(url);
        return parse();
    }

    /**
     * Cons up this url from constituent pieces.
     */
    void format(const std::string& proto, const std::string& host, u_int16_t port,
                const std::string& path);
    
    /**
     * Return the result of the parse operation.
     */
    urlerr_t status() const
    {
        return err_;
    }

    /**
     * Return an indication of whether or not this url is valid.
     */
    bool valid() const
    {
        return (err_ == URLPARSE_OK);
    }

    /**
     * Wrappers around some basic std::string accessors to simplify
     * things.
     */
    ///@{
    const char* c_str()  const { return url_.c_str(); } 
    const char* data()   const { return url_.data(); }
    size_t      length() const { return url_.length(); }

    /*
     * The constituent fields are public to avoid the need for a bunch
     * of accessors. Assume that users of the class don't actually
     * muck with the objects or things may not work well.
     */
    std::string url_;	/* the url std::string */
    std::string proto_;	/* the protocol */
    std::string host_;	/* the hostname part */
    u_int16_t   port_;	/* the port (0 if doesn't exists) */
    std::string path_;	/* the path part */
    
    urlerr_t err_;	/* parse status */

protected:
    urlerr_t parse_internal();
};

#endif /* _URL_H_ */

