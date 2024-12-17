//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/LUTmath.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec3.h>
#include <math.h>

namespace LUT
{
    using namespace std;
    using namespace TwkMath;

    typedef TwkMath::Vec3f Pixel3f;

    int CubeRoot(int in)
    {
        for (int i = 0; i <= 128; i++)
        {
            if (in == i * i * i)
                return i;
        }

        return 0;
    }

    int Pow2(int in)
    {
        int n = 1;

        // n = 2 ^ i
        for (int j = 1; j <= in; j++)
        {
            n *= 2;
        }

        return n;
    }

    //
    // Log2() will round up if a non 2^n number is given
    // Log2strict() will return 0
    //

    int Log2(int in)
    {
        for (int i = 0; i <= 128; i++)
        {
            if (Pow2(i) >= in)
                return i;
        }

        return 0;
    }

    int Log2strict(int in)
    {
        for (int i = 0; i <= 128; i++)
        {
            if (Pow2(i) == in)
                return i;
        }

        return 0;
    }

    void IdentityLUT(vector<float>& data, vector<int>& sizes)
    {
        if (sizes.size() == 3)
        {
            //
            // 3D LUT
            //

            data.resize(3 * sizes[0] * sizes[1] * sizes[2]);

            int n = 0;

            for (int b = 0; b < sizes[2]; b++)
            {
                for (int g = 0; g < sizes[1]; g++)
                {
                    for (int r = 0; r < sizes[0]; r++)
                    {
                        data[n++] = (float)r / (float)(sizes[0] - 1);
                        data[n++] = (float)g / (float)(sizes[1] - 1);
                        data[n++] = (float)b / (float)(sizes[2] - 1);
                    }
                }
            }
        }
        else if (sizes.size() == 1)
        {
            //
            // 1D RGB LUT
            //

            data.resize(3 * sizes[0]);

            int n = 0;

            for (int i = 0; i < sizes[0]; i++)
            {
                float val = (float)i / (float)(sizes[0] - 1);

                data[n++] = val;
                data[n++] = val;
                data[n++] = val;
            }
        }
    }

    static Pixel3f AccessLUTitem(vector<float>& data, vector<int>& sizes, int x,
                                 int y, int z)
    {
        size_t place = 0;

        place += z * sizes[2] * sizes[1];
        place += y * sizes[1];
        place += x;

        place *= 3;

        float r = data[place++];
        float g = data[place++];
        float b = data[place++];

        Pixel3f rgb(r, g, b);

        return rgb;
    }

    void ApplyLUTtoPixel(vector<float>& data, vector<int>& sizes, float& r,
                         float& g, float& b)
    {
        if (sizes.size() == 3)
        {
            const Vec3f* lut = reinterpret_cast<const Vec3f*>(&data.front());
            const Vec3f p(r, g, b);

            const size_t xs = sizes[0];
            const size_t ys = sizes[1];
            const size_t zs = sizes[2];
            const size_t xl = xs - 1;
            const size_t yl = ys - 1;
            const size_t zl = zs - 1;

            Vec3f ip = Vec3f(clamp(p[0], 0.0f, 1.0f), clamp(p[1], 0.0f, 1.0f),
                             clamp(p[2], 0.0f, 1.0f));

            Vec3f vn = ip * Vec3f(xl, yl, zl);

            const size_t x0 = size_t(vn.x);
            const size_t y0 = size_t(vn.y);
            const size_t z0 = size_t(vn.z);
            const size_t x1 = x0 == xl ? xl : x0 + 1;
            const size_t y1 = y0 == yl ? yl : y0 + 1;
            const size_t z1 = z0 == zl ? zl : z0 + 1;

            Vec3f corners[2][2][2];

            corners[0][0][0] = lut[xs * ys * z0 + xs * y0 + x0];
            corners[0][0][1] = lut[xs * ys * z0 + xs * y0 + x1];
            corners[0][1][0] = lut[xs * ys * z0 + xs * y1 + x0];
            corners[0][1][1] = lut[xs * ys * z0 + xs * y1 + x1];
            corners[1][0][0] = lut[xs * ys * z1 + xs * y0 + x0];
            corners[1][0][1] = lut[xs * ys * z1 + xs * y0 + x1];
            corners[1][1][0] = lut[xs * ys * z1 + xs * y1 + x0];
            corners[1][1][1] = lut[xs * ys * z1 + xs * y1 + x1];

            float t[3] = {vn.x - float(x0), vn.y - float(y0), vn.z - float(z0)};

            Vec3f op = trilinear(corners, t);

            r = op[0];
            g = op[1];
            b = op[2];
        }
    }

} // namespace LUT
