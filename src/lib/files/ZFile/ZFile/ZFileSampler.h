//******************************************************************************
// Copyright (c) 2001 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __ZFile__ZFileSampler__h__
#define __ZFile__ZFileSampler__h__
#include <ZFile/ZFileReader.h>

namespace ZFile
{

    //
    //  class Sampler
    //
    //  Looks up a value in a ZFile file. You supply the ZFile reader
    //  object. This was shamelessly copied from the Dpz::Sampler class.
    //

    class Sampler
    {
    public:
        Sampler(const Reader& r)
            : m_reader(&r)
        {
        }

        Sampler(const Reader* r)
            : m_reader(r)
        {
        }

        //
        //	Returns the depth value for a given NDC value in the
        //	ZFile. This is the raw depth value.
        //

        float depthAtNDC(float x, float y) const;

        //
        //  Return the depth at a pixel
        //

        float depthAtPixel(uint16 x, uint16 y) const
        {
            return m_reader->depthAt(x, y);
        }

        //
        //	Returns depth for a given world value in the ZFile. This is
        //	the depth straight from the ZFile. For relative depth use the
        //	relativeDepthAtWorldPoint() function.
        //

        float depthAtWorldPoint(float x, float y, float z) const;

        //
        //	Returns the depth value of the world point (not the depth that
        //	stored in the file, the depth of the point if it were stored in
        // the 	file.)
        //

        float depthOfWorldPoint(float x, float y, float z) const;

        //
        //	Returns the relative depth for a given point in world space,
        //	this is a signed value which indicates the depth of the input
        //	point relative to the ZFile value. The depth will negative
        //	*outside* of the shadowed volume (between the volume and the
        //	light).
        //

        float relativeDepthAtWorldPoint(float x, float y, float z) const;

        //
        //  Access to the reader
        //

        const Reader* reader() const { return m_reader; }

    protected:
        Sampler() {}

        void matrixMultiply(const float*, float&, float&, float&) const;

    private:
        const Reader* m_reader;
    };

    //----------------------------------------------------------------------

    inline void Sampler::matrixMultiply(const float* M, float& x, float& y,
                                        float& z) const
    {
        float nx = x * M[0] + y * M[4] + z * M[8] + M[12];
        float ny = x * M[1] + y * M[5] + z * M[9] + M[13];
        float nz = x * M[2] + y * M[6] + z * M[10] + M[14];
        float w = x * M[3] + y * M[7] + z * M[11] + M[15];

        x = nx / w;
        y = ny / w;
        z = nz / w;
    }

    inline float Sampler::depthAtNDC(float x, float y) const
    {
        // uint16 px = uint16( float(m_reader->header().imageWidth) * x + 0.5f
        // ); uint16 py = uint16( float(m_reader->header().imageHeight) * y +
        // 0.5f );
        uint16 px = uint16(float(m_reader->header().imageWidth - 1) * x);
        uint16 py = uint16(float(m_reader->header().imageHeight - 1) * y);
        return m_reader->depthAt(px, py);
    }

    inline float Sampler::depthAtWorldPoint(float x, float y, float z) const
    {
        matrixMultiply(m_reader->header().worldToScreen, x, y, z);
        return depthAtNDC((1.0f + x) / 2.0f, (1.0f - y) / 2.0f);
    }

    inline float Sampler::depthOfWorldPoint(float x, float y, float z) const
    {
        matrixMultiply(m_reader->header().worldToCamera, x, y, z);
        return z;
    }

    inline float Sampler::relativeDepthAtWorldPoint(float x, float y,
                                                    float z) const
    {
        return depthOfWorldPoint(x, y, z) - depthAtWorldPoint(x, y, z);
    }

} // namespace ZFile

#endif // __ZFile__ZFileSampler__h__
