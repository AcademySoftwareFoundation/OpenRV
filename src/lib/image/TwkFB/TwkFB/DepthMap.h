//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkFB__DepthMap__h__
#define __TwkFB__DepthMap__h__
#include <TwkFB/dll_defs.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec2.h>

namespace TwkFB
{

    //
    //  class DepthMapSampler
    //
    //  This class wraps around a FrameBuffer and provides an API much
    //  like ZFile. (In fact, it should work with the Zfile I/O just like
    //  ZFile::Sampler, so in theory you replace it with this).
    //
    //  The depth data is assumed to be either in world or camera space
    //  and will be a single channel. The constructor indicates which
    //  channel and which space the data is in.
    //
    //  The FrameBuffer *must* have two attributes containing matrices:
    //  "worldToCamera" and "worldToNDC". Optionally, there can be a
    //  "worldToScreen" matrix will be converted to worldToNDC
    //  automatically.
    //

    class TWKFB_EXPORT DepthMap
    {
    public:
        enum DepthSpace
        {
            WorldSpace,
            CameraSpace
        };

        typedef TwkMath::Mat44f Mat44f;
        typedef TwkMath::Vec3f Point;
        typedef TwkMath::Vec2f Vec2f;

        DepthMap(const FrameBuffer*);
        DepthMap(const FrameBuffer*, int depthChannel = 0,
                 DepthSpace space = CameraSpace);
        ~DepthMap();

        float depthAtNDC(float x, float y) const;
        Vec2f gradAtNDC(float x, float y) const;

        float depthAtPixel(size_t x, size_t y) const;
        Vec2f gradAtPixel(size_t x, size_t y) const;

        float depthAtWorldPoint(Point) const;
        float depthOfWorldPoint(Point) const;

        float relativeDepthAtWorldPoint(Point) const;

        size_t width() const { return m_fb->width(); }

        size_t height() const { return m_fb->height(); }

        const Mat44f& worldToScreen() const { return m_worldToScreen; }

        const Mat44f& worldToCamera() const { return m_worldToCamera; }

    private:
        void init();

    private:
        const FrameBuffer* m_fb;
        Mat44f m_worldToScreen;
        Mat44f m_worldToCamera;
        int m_depthChannel;
        DepthSpace m_space;
    };

} // namespace TwkFB

#endif // __TwkFB__DepthMap__h__
