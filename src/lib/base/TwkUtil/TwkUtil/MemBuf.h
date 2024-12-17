//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkUtil__MemBuf__h__
#define __TwkUtil__MemBuf__h__
#include <iostream>

namespace TwkUtil
{

    //
    //  streambuf that you can hand an existing region of memory.
    //  e.g.:
    //
    //      char *mybuffer;
    //      size_t length;
    //      MemBuf mb(mybuffer, length);
    //      istream reader(&mb);
    //

    class MemBuf : public std::basic_streambuf<char>
    {
    public:
        MemBuf(char* p, size_t n)
        {
            setg(p, p, p + n);
            setp(p, p + n);
        }
    };

} // namespace TwkUtil

#endif // __TwkUtil__MemBuf__h__
