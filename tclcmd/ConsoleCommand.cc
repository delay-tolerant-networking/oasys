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


#include "ConsoleCommand.h"
#include <netinet/in.h>

namespace oasys {

ConsoleCommand::ConsoleCommand(const char* default_prompt)
    : TclCommand("console"), prompt_(default_prompt)
{
    bind_b("stdio", &stdio_, true, "spawn interpreter on stdin/stdout");
    bind_addr("addr", &addr_,  htonl(INADDR_LOOPBACK),
              "console listening address");
    bind_i("port", &port_, 0, "console listening port (default 0)");
    bind_s("prompt", &prompt_, 0, "console prompt string");
}

} // namespace oasys
