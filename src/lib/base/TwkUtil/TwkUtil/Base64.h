//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkUtil__Base64__h__
#define __TwkUtil__Base64__h__
#include <iostream>
#include <string>
#include <vector>

#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    //
    //  id64 is base64 encoding which can be embedded in a filename. It allows
    //  swaping '/', '+', and '=' characters (62nd, 63rd, and pad values) for
    //  something else. The default is to use '-', '_', and '~' which don't
    //  require encoding in URLs.
    //

    TWKUTIL_EXPORT std::string base64Encode(const char* data, size_t size);

    inline std::string base64Encode(const std::string& data)
    {
        return base64Encode(data.c_str(), data.size());
    }

    TWKUTIL_EXPORT std::string id64Encode(const char* data, size_t size,
                                          char t62 = '-', char t63 = '_',
                                          char pad = '~');

    inline std::string id64Encode(const std::string& data, char t62 = '-',
                                  char t63 = '_', char pad = '~')
    {
        return id64Encode(data.c_str(), data.size(), t62, t63, pad);
    }

    TWKUTIL_EXPORT void base64Decode(const char* s, size_t size,
                                     std::vector<char>& data);

    inline void base64Decode(const std::string& s, std::vector<char>& data)
    {
        return base64Decode(s.data(), s.size(), data);
    }

    TWKUTIL_EXPORT void id64Decode(const char* s, size_t size,
                                   std::vector<char>& data, char t62 = '-',
                                   char t63 = '_', char pad = '~');

    inline void id64Decode(const std::string& s, std::vector<char>& data,
                           char t62 = '-', char t63 = '_', char pad = '~')
    {
        return id64Decode(s.data(), s.size(), data, t62, t63, pad);
    }

} // namespace TwkUtil

#endif // __TwkUtil__Base64__h__
