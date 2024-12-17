//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkUtil/FourCC.h>
#include <sstream>

namespace TwkUtil
{
    using namespace std;

    string packedFourCCAsString(PackedFourCC x)
    {
        ostringstream str;
        const char* c = (const char*)&x;

#ifdef TWK_LITTLE_ENDIAN
        str << c[3] << c[2] << c[1] << c[0];
#endif

#ifdef TWK_BIG_ENDIAN
        str << c[0] << c[1] << c[2] << c[3];
#endif

#if !defined(TWK_LITTLE_ENDIAN) && !defined(TWK_BIG_ENDIAN)
#error Makefile for this target is missing ENIDAN defines in SWITCHES
#endif

        return str.str();
    }

} // namespace TwkUtil
