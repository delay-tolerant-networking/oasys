/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to
 * this license. If you do not agree to this license, do not download,
 * install, copy or use the software.
 * 
 * Intel Open Source License 
 * 
 * Copyright (c) 2005 Intel Corporation. All rights reserved. 
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
#ifndef _OASYS_BASIC_SMTP_H_
#define _OASYS_BASIC_SMTP_H_

#include "SMTP.h"
#include "util/StringUtils.h"

namespace oasys {

//----------------------------------------------------------------------------
/*!
 * Simple struct for a mail message
 */
class BasicSMTPMsg {
public:
    std::string              from_;
    std::vector<std::string> to_;
    std::string              msg_;

    BasicSMTPMsg() {}
    BasicSMTPMsg(const std::string& from,
                 const std::string& to,
                 const std::string& msg)
    {
        from_ = from;
        tokenize(to, ", ", &to_);
        msg_  = msg;
    }
    
    bool valid() {
        return (from_.size() > 0 &&
                to_.size()   > 0 &&
                msg_.size()  > 0);
    }
    
    void clear() {
        from_.clear();
        to_.clear();
        msg_.clear();
    }
};

//----------------------------------------------------------------------------
/*!
 * Implements a simple smtp sender that works with the data in the
 * BasicSMTPMsg.
 */
class BasicSMTPSender : public SMTPSender {
public:
    BasicSMTPSender(const std::string& helo_domain, BasicSMTPMsg* msg);
    virtual ~BasicSMTPSender() {}
    
protected:
    //! @{ virtual from SMTPSender
    void get_HELO_domain(std::string* domain);
    void get_MAIL_from(std::string* from);
    void get_RCPT_list(std::vector<std::string>* to);
    void get_DATA(const std::string** data);
    int smtp_error(int code);
    //! @}

    std::string   helo_domain_;
    BasicSMTPMsg* msg_;
};

//----------------------------------------------------------------------------
/*!
 * Implements a not-so-efficient email consumer
 */
class BasicSMTPHandler : public SMTPHandler {
public:
    BasicSMTPHandler();

    //! @{ virtual from SMTPHandler
    int smtp_HELO(const char* domain);
    int smtp_MAIL(const char* from);
    int smtp_RCPT(const char* to);
    int smtp_DATA_begin();
    int smtp_DATA_line(const char* line);
    int smtp_DATA_end();
    int smtp_RSET();
    int smtp_QUIT();
    //! @}

    virtual void message_recvd(const BasicSMTPMsg& msg) = 0;

private:
    //! Current message that is being worked on
    BasicSMTPMsg cur_msg_;
};

} // namespace oasys 

#endif /* _OASYS_BASIC_SMTP_H_ */
