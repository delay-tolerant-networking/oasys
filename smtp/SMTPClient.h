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
#ifndef _OASYS_SMTP_CLIENT_H_
#define _OASYS_SMTP_CLIENT_H_

#include "../io/FdIOClient.h"
#include "../io/TCPClient.h"
#include "SMTP.h"

namespace oasys {

/**
 * Class to wrap a client socket to an smtp server.
 */
class SMTPClient : public TCPClient {
public:
    /// Default constructor
    SMTPClient(const char* logpath = "/oasys/smtp/client");

    /// Send a message using the SMTPSender interface. Returns 0 on
    /// success, an SMTP error code on failure.
    int send_message(SMTPSender* sender);
    
protected:
    BufferedInput  in_;
    BufferedOutput out_;
    SMTP           smtp_;
    bool           first_session_;
};

/**
 * Debugging class to provide basically the same functionality only
 * using a pair of file descriptors, not a socket
 */
class SMTPFdClient {
public:
    /// Default constructor
    SMTPFdClient(int fd_in, int fd_out,
                 const char* logpath = "/oasys/smtp/client");
     
    /// Send a message using the SMTPSender interface. Returns 0 on
    /// success, an SMTP error code on failure.
    int send_message(SMTPSender* sender);
     
protected:
    FdIOClient     fdio_in_;
    FdIOClient     fdio_out_;
    BufferedInput  in_;
    BufferedOutput out_;
    SMTP           smtp_;
    bool           first_session_;
};


} // namespace oasys

#endif /* _OASYS_SMTP_CLIENT_H_ */
