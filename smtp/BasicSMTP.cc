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

#include "BasicSMTP.h"

namespace oasys {

//----------------------------------------------------------------------------
BasicSMTPSender::BasicSMTPSender(const std::string& helo_domain,
                                 BasicSMTPMsg* msg)
    : helo_domain_(helo_domain), msg_(msg)
{
}

//----------------------------------------------------------------------------
void
BasicSMTPSender::get_HELO_domain(std::string* domain)
{
    domain->assign(helo_domain_);
}

//----------------------------------------------------------------------------
void
BasicSMTPSender::get_MAIL_from(std::string* from)
{
    from->assign(msg_->from_);
}

//----------------------------------------------------------------------------
void
BasicSMTPSender::get_RCPT_list(std::vector<std::string>* to)
{
    to->insert(to->begin(), msg_->to_.begin(), msg_->to_.end());
}

//----------------------------------------------------------------------------
void
BasicSMTPSender::get_DATA(const std::string** data)
{
    *data = &msg_->msg_;
}

//----------------------------------------------------------------------------
int
BasicSMTPSender::smtp_error(int code)
{
    log_err("/oasys/smtp-sender", "unexpected error %d", code);
    return -1;
}

//----------------------------------------------------------------------------
BasicSMTPHandler::BasicSMTPHandler()
{
}

//----------------------------------------------------------------------------
int
BasicSMTPHandler::smtp_HELO(const char* domain)
{
    return 250;
}

//----------------------------------------------------------------------------
int
BasicSMTPHandler::smtp_MAIL(const char* from)
{
    if (strlen(from) == 0) {
        return 501;
    }

    cur_msg_.from_ = from;
    return 250;
}

//----------------------------------------------------------------------------
int
BasicSMTPHandler::smtp_RCPT(const char* to)
{
    if (strlen(to) == 0) {
        return 501;
    }

    cur_msg_.to_.push_back(to);
    return 250;
}

//----------------------------------------------------------------------------
int
BasicSMTPHandler::smtp_RSET()
{
    return 250;
}

//----------------------------------------------------------------------------
int
BasicSMTPHandler::smtp_QUIT()
{
    return 221;
}

//----------------------------------------------------------------------------
int
BasicSMTPHandler::smtp_DATA_begin()
{
    ASSERT(cur_msg_.msg_.size() == 0);
    return 0;
}

//----------------------------------------------------------------------------
int
BasicSMTPHandler::smtp_DATA_line(const char* line)
{
    cur_msg_.msg_.append(line);
    cur_msg_.msg_.append("\r\n");

    return 0;
}

//----------------------------------------------------------------------------
int
BasicSMTPHandler::smtp_DATA_end()
{
    if (cur_msg_.valid()) {
        message_recvd(cur_msg_);
    }
    cur_msg_.clear();

    return 250;
}

} // namespace oasys
