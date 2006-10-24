/*
 *    Copyright 2006 Intel Corporation
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

#ifndef __DEBUGDUMPBUF_H__
#define __DEBUGDUMPBUF_H__

#include <stdlib.h>

namespace oasys {

struct DebugDumpBuf {
    static const size_t size_ = 1024 * 8;
    static char buf_[size_];
};

} // namespace oasys

#endif /* __DEBUGDUMPBUF_H__ */
