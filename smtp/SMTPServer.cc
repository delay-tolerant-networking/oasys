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

#include "SMTPServer.h"

namespace oasys {

//----------------------------------------------------------------------------
SMTPServer::SMTPServer(const SMTP::Config& config,
                       SMTPHandlerFactory* handler_factory,
                       Notifier*           session_done,
                       int                 accept_timeout)
    : TCPServerThread("TCPServerThread", "/smtp/server", 0, accept_timeout),
      config_(config),
      handler_factory_(handler_factory),
      session_done_(session_done)
{
    logpathf("/smtp/server/%s:%d", intoa(config.addr_), config.port_);
    bind_listen_start(config.addr_, config.port_);
}

//----------------------------------------------------------------------------
void
SMTPServer::accepted(int fd, in_addr_t addr, u_int16_t port)
{
    SMTPHandler* handler = handler_factory_->new_handler();
    SMTPHandlerThread* thread =
        new SMTPHandlerThread(handler, fd, fd, config_, session_done_);
    thread->start();
}

//----------------------------------------------------------------------------
SMTPHandlerThread::SMTPHandlerThread(SMTPHandler* handler,
                                     int fd_in, int fd_out,
                                     const SMTP::Config& config,
                                     Notifier* session_done)
    : Thread("/smtp/server", DELETE_ON_EXIT),
      handler_(handler),
      fdio_in_(fd_in), fdio_out_(fd_out),
      in_(&fdio_in_), out_(&fdio_out_),
      smtp_(&in_, &out_, config, "/smtp/server"),
      session_done_(session_done)
{
}

//----------------------------------------------------------------------------
SMTPHandlerThread::~SMTPHandlerThread()
{
    delete handler_;
    handler_ = 0;
}

//----------------------------------------------------------------------------
void
SMTPHandlerThread::run()
{
    smtp_.server_session(handler_);
    if (session_done_) {
        session_done_->notify();
    }
}

} // namespace oasys
