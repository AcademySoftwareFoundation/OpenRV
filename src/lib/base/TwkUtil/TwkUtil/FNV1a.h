//
//  Copyright (c) 2016 Autodesk.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkUtil__FNV1a__h__
#define __TwkUtil__FNV1a__h__
#include <iostream>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    //
    //  Fowler–Noll–Vo hash function (1a)
    //

    TWKUTIL_EXPORT size_t FNV1a64(const void* data, size_t sizeInBytes);

} // namespace TwkUtil

#endif // __TwkUtil__FNV1a__h__
