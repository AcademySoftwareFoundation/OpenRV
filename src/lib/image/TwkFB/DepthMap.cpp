//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkFB/DepthMap.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Function.h>
#include <TwkUtil/File.h>
#include <string>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>

namespace TwkFB
{
    using namespace TwkMath;
    using namespace TwkUtil;
    using namespace std;

    static void parse(Mat44f& M, const string& value)
    {
        stringstream str;

        //
        //  Copy the value into str, but filter out any unwanted
        //  punctuation
        //

        for (int i = 0; i < value.size(); i++)
        {
            if (value[i] != ')' && value[i] != '(' && value[i] != '['
                && value[i] != ']' && value[i] != ',')
            {
                str << value[i];
            }
            else
            {
                str << " ";
            }
        }

        //
        //  Load the values into M
        //

        for (int i = 0; i < 16; i++)
            str >> M[0][i];

        //
        //  Guess if M needs to be transposed
        //

        if ((M(3, 0) > 1e-6 || M(3, 1) > 1e-6 || M(3, 2) > 1e-6)
            && (M(0, 3) < 1e-6 || M(1, 3) < 1e-6 || M(2, 3) < 1e-6))
        {
            // cout << "INFO: depth map transposing" << endl;
            M.transpose();
        }
    }

    DepthMap::DepthMap(const FrameBuffer* fb)
        : m_fb(fb)
        , m_depthChannel(fb->numChannels() == 1 ? 0 : 1)
        , m_space(fb->numChannels() == 1 ? CameraSpace : WorldSpace)
    {
        init();
    }

    DepthMap::DepthMap(const FrameBuffer* fb, int depthChannel,
                       DepthSpace space)
        : m_fb(fb)
        , m_depthChannel(depthChannel)
        , m_space(space)
    {
        init();
    }

    void DepthMap::init()
    {
        typedef TwkFB::TypedFBAttribute<Mat44f> Mat44fAttribute;

        const FrameBuffer::AttributeVector& attrs = m_fb->attributes();
        bool foundC = false;
        bool foundS = false;

        for (int i = 0; i < attrs.size(); i++)
        {
            FBAttribute* a = attrs[i];
            string name = basename(a->name());

            if (name == "worldToCamera")
            {
                parse(m_worldToCamera, a->valueAsString());
                foundC = true;
            }
            else if (name == "worldToScreen")
            {
                parse(m_worldToScreen, a->valueAsString());
                foundS = true;
            }
            else if (name == "worldToNDC")
            {
                parse(m_worldToScreen, a->valueAsString());
                Mat44f T, S;
                T.makeTranslation(Vec3f(-1, -1, -1));
                S.makeScale(Vec3f(2, 2, 2));
                m_worldToScreen = T * S * m_worldToScreen;
                foundS = true;
            }
        }

        if (!foundS || !foundC)
        {
            throw invalid_argument("missing matrix attribute in depth map");
        }
    }

    DepthMap::~DepthMap() {}

    float DepthMap::depthAtNDC(float ndcx, float ndcy) const
    {
        const float x = ndcx * float(width() - 1);
        const float y = ndcy * float(height() - 1);

        const size_t px0 = size_t(x);
        const size_t py0 = size_t(y);
        const size_t px1 = std::min(px0 + 1, width() - 1);
        const size_t py1 = std::min(py0 + 1, height() - 1);

        const float rx = x - float(px0);
        const float ry = y - float(py0);

        const float a = depthAtPixel(px0, py0);
        const float b = depthAtPixel(px1, py0);
        const float c = depthAtPixel(px1, py1);
        const float d = depthAtPixel(px0, py1);

        return lerp(lerp(a, b, rx), lerp(d, c, rx), ry);
    }

    Vec2f DepthMap::gradAtNDC(float ndcx, float ndcy) const
    {
        const float x = ndcx * float(width() - 1);
        const float y = ndcy * float(height() - 1);

        const size_t px0 = size_t(x);
        const size_t py0 = size_t(y);
        const size_t px1 = std::min(px0 + 1, width() - 1);
        const size_t py1 = std::min(py0 + 1, height() - 1);

        const float rx = x - float(px0);
        const float ry = y - float(py0);

        const Vec2f a = gradAtPixel(px0, py0);
        const Vec2f b = gradAtPixel(px1, py0);
        const Vec2f c = gradAtPixel(px1, py1);
        const Vec2f d = gradAtPixel(px0, py1);

        return lerp(lerp(a, b, rx), lerp(d, c, rx), ry);
    }

    Vec2f DepthMap::gradAtPixel(size_t x, size_t y) const
    {
        size_t x0 = x > 0 ? x - 1 : x;
        size_t x1 = x < width() - 1 ? x + 1 : x;

        size_t y0 = y > 0 ? y - 1 : y;
        size_t y1 = y < height() - 1 ? y + 1 : y;

        float dx = depthAtPixel(x1, y) - depthAtPixel(x0, y);
        float dy = depthAtPixel(x, y1) - depthAtPixel(x, y0);
        return Vec2f(dx / 2.0, dy / 2.0);
    }

    float DepthMap::depthAtWorldPoint(Point p) const
    {
        const Point q = m_worldToScreen * p;
        return depthAtNDC((1.0f + q.x) / 2.0f, (1.0f - q.y) / 2.0f);
    }

    float DepthMap::depthOfWorldPoint(Point p) const
    {
        return (m_worldToCamera * p).z;
    }

    float DepthMap::depthAtPixel(size_t x, size_t y) const
    {
        float p[4];
        if (x >= m_fb->width())
            x = m_fb->width() - 1;
        if (y >= m_fb->height())
            y = m_fb->height() - 1;
        m_fb->getPixel4f(x, y, p);

        if (m_space == CameraSpace)
        {
            return p[m_depthChannel];
        }
        else
        {
            Vec3f pc = m_worldToCamera * Vec3f(p[m_depthChannel]);
            return pc.x;
        }
    }

    float DepthMap::relativeDepthAtWorldPoint(Point p) const
    {
        return depthOfWorldPoint(p) - depthAtWorldPoint(p);
    }

} // namespace TwkFB
