//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkUtil__ThreadName__h__
#define __TwkUtil__ThreadName__h__
#include <string>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    TWKUTIL_EXPORT void setThreadName(const std::string&);
    TWKUTIL_EXPORT std::string getThreadName();

} // namespace TwkUtil

#endif // __TwkUtil__ThreadName__h__
