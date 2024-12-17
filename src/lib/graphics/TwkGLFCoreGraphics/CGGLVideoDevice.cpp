//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkGLFCoreGraphics/CGGLVideoDevice.h>
#include <iostream>

namespace TwkGLF
{
    using namespace std;
    using namespace TwkApp;

    CGGLVideoDevice::CGGLVideoDevice(VideoModule* m, int w, int h, bool alpha,
                                     CGLContextObj context, bool owner)
        : TwkGLF::GLVideoDevice(m, "CoreGraphics", VideoDevice::ImageOutput)
        , m_width(w)
        , m_height(h)
        , m_alpha(alpha)
        , m_context(context)
        , m_owner(owner)
    {
    }

    CGGLVideoDevice::~CGGLVideoDevice()
    {
        if (m_owner)
            CGLDestroyContext(m_context);
    }

    size_t CGGLVideoDevice::width() const { return m_width; }

    size_t CGGLVideoDevice::height() const { return m_height; }

    void CGGLVideoDevice::redraw() const {}

    void CGGLVideoDevice::redrawImmediately() const {}

    void CGGLVideoDevice::makeCurrent() const
    {
        CGLSetCurrentContext(m_context);
    }

    string CGGLVideoDevice::hardwareIdentification() const
    {
        return "CoreGraphics";
    }

} // namespace TwkGLF
