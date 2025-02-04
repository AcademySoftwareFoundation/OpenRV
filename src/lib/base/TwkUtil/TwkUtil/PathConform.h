//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkUtil__PathConform__h__
#define __TwkUtil__PathConform__h__
#include <string>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    TWKUTIL_EXPORT std::string pathConform(const std::string& path);
    TWKUTIL_EXPORT bool pathIsURL(const std::string& path);

} // namespace TwkUtil

#endif // __TwkUtil__PathConform__h__
