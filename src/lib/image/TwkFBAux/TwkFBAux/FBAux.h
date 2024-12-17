//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkFBAux__FBAux__h__
#define __TwkFBAux__FBAux__h__
#include <TwkMath/Vec2.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFBAux/dll_defs.h>

namespace TwkFBAux
{

    typedef TwkFB::FrameBuffer FrameBuffer;

    enum Interpolation
    {
        LinearInterpolation,
        AreaInterpolation
    };

    TWKFBAUX_EXPORT void resize(const FrameBuffer* src, FrameBuffer* dst);

} // namespace TwkFBAux

#endif // __TwkFBAux__FBAux__h__
