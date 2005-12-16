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
#ifndef __OASYS_SMTP_H__
#define __OASYS_SMTP_H__

#include "../debug/Logger.h"
#include "../io/BufferedIO.h"
#include "../io/NetUtils.h"

namespace oasys {

class BasicSMTPMsg;
class SMTPHandler;
class SMTPSender;

//----------------------------------------------------------------------------
/*!
 * Basic class that implements the SMTP protocol.
 */
class SMTP : public Logger {
public:
    struct Config {
        /// Default config constructor
        Config()
            : addr_(htonl(INADDR_LOOPBACK)),
              port_(25),
              timeout_(-1),
              domain_("default.domain.com") {}

        /// Specific config constructor
        Config(in_addr_t addr, u_int16_t port,
               int timeout, const std::string& domain)
            : addr_(addr), port_(port),
              timeout_(timeout),
              domain_(domain) {}

        in_addr_t   addr_;      // listening address
        u_int16_t   port_;      // listening port
        int         timeout_;   // timeout used for IO
        std::string domain_;    // domain for HELO requests
    };

    static Config DEFAULT_CONFIG;

    SMTP(BufferedInput*  in,
         BufferedOutput* out,
         const Config&   config,
         const char*     logpath);

    int client_session(SMTPSender*  sender, bool first_session);
    int server_session(SMTPHandler* handler);

private:
    static const char* nl_; // newline char

    BufferedInput*  in_;
    BufferedOutput* out_;
    Config          config_;

    //! Send sign on message
    int send_signon();

    //! Process a command.
    int process_cmd(SMTPHandler* handler);

    //! Process a response.
    int process_response(int expected_code);
    
    //! Send back a response
    int send_response(int code);

    //! Response code may include a %s for the domain name
    const char* response_code(int code) const;
};

//----------------------------------------------------------------------------
/*!
 * Interface for class to send an outgoing SMTP message.
 * XXX/demmer 
 */
class SMTPSender {
public:
    virtual ~SMTPSender() {}
    
    //! @{ @return -1 to disconnect, otherwise error code given in
    //!    response code.
    virtual void get_HELO_domain(std::string* domain) = 0;
    virtual void get_MAIL_from(std::string* from) = 0;
    virtual void get_RCPT_list(std::vector<std::string>* to) = 0;
    virtual void get_RECEIVED(std::string* received) {}
    virtual void get_DATA(const std::string** data) = 0;
    //! @}

    //! handle unexpected return code from server
    virtual int smtp_error(int code) = 0;
};

//----------------------------------------------------------------------------
/*!
 * Interface for a handler to process incoming SMTP messages.
 */
class SMTPHandler {
public:
    virtual ~SMTPHandler() {}

    //! @{ @return -1 to disconnect, otherwise error code given in
    //!    response code.
    virtual int smtp_HELO(const char* domain) = 0;
    virtual int smtp_MAIL(const char* from)   = 0;
    virtual int smtp_RCPT(const char* to)     = 0;
    virtual int smtp_RSET() = 0;
    virtual int smtp_QUIT() = 0;
    //! @}

    //! @{
    /*!
     * @{ @return -1 to disconnect, 0 on no error, otherwise error
     * code given in response code. smtp_DATA_end always returns a
     * response code, never 0.
     */
    virtual int smtp_DATA_begin()                = 0;
    virtual int smtp_DATA_line(const char* line) = 0;
    virtual int smtp_DATA_end()                  = 0;
    //! @}
};

} // namespace oasys

#endif /* __OASYS_SMTP_H__ */
