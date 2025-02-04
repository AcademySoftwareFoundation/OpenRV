//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkUtil__FourCC__h__
#define __TwkUtil__FourCC__h__
#include <iostream>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    //
    //  To use:
    //
    //  unsigned int fourCC = TwkUtil::FourCC<'A','B','C','D'>::value;
    //

    typedef unsigned int PackedFourCC;

    template <int d, int c, int b, int a> struct FourCC
    {
        static const PackedFourCC value =
            (((((d << 8) | c) << 8) | b) << 8) | a;
    };

    TWKUTIL_EXPORT std::string packedFourCCAsString(PackedFourCC x);

} // namespace TwkUtil

#endif // __TwkUtil__FourCC__h__
