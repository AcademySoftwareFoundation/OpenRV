#ifndef __TWKUTILSTR_H__
#define __TWKUTILSTR_H__
//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <string>
#include <TwkUtil/dll_defs.h>

//
// This file contains miscellaneous string functions
//

namespace TwkUtil
{

    // Returns true if s1 ends with s2
    TWKUTIL_EXPORT bool endswith(std::string s1, std::string s2);

    // Returns true if s1 starts with s2
    TWKUTIL_EXPORT bool startswith(std::string s1, std::string s2);

    // Case conversion
    TWKUTIL_EXPORT std::string uc(std::string str);
    TWKUTIL_EXPORT std::string lc(std::string str);

} // End namespace TwkUtil

#endif // End #ifdef __TWKUTILSTR_H__
