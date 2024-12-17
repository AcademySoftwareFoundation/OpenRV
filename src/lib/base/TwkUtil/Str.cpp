//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <string>
#include <algorithm>
#include <ctype.h>

#include <TwkUtil/Str.h>

namespace TwkUtil
{
    using namespace std;

    // Returns true if s1 ends with s2
    bool endswith(string s1, string s2)
    {
        return s1.substr(s1.length() - s2.length(), s2.length()) == s2;
    }

    // Returns true if s1 starts with s2
    bool startswith(string s1, string s2)
    {
        return s1.substr(0, s2.length()) == s2;
    }

    static unsigned char upit(unsigned char c)
    {
        return (unsigned char)toupper(c);
    }

    static unsigned char lowit(unsigned char c)
    {
        return (unsigned char)tolower(c);
    }

    // Case conversion
    string uc(string str)
    {
        string uc_str = str;
        string::iterator i = uc_str.begin();
#ifdef PLATFORM_APPLE_MACH_BSD
        transform(uc_str.begin(), uc_str.end(), i, __toupper);
#else
        transform(uc_str.begin(), uc_str.end(), i, upit);
#endif
        return uc_str;
    }

    string lc(string str)
    {
        string lc_str = str;
        string::iterator i = lc_str.begin();
#ifdef PLATFORM_APPLE_MACH_BSD
        transform(lc_str.begin(), lc_str.end(), i, __tolower);
#else
        transform(lc_str.begin(), lc_str.end(), i, lowit);
#endif
        return lc_str;
    }

} // End namespace TwkUtil
