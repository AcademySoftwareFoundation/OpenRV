//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/ReadLUT.h>
#include <LUT/LUTmath.h>
#include <LUT/rv3dlut.h>
#include <LUT/cubelut.h>
#include <LUT/AppleColorLUT.h>
#include <LUT/NukeVectorfield.h>
#include <LUT/InfernoLUT.h>
#include <LUT/CinespaceLUT.h>
#include <LUT/PanavisionLUT.h>
#include <LUT/TruelightLUT.h>
#include <LUT/rvchlut.h>
#include <TwkUtil/File.h>
#include <TwkExc/Exception.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Function.h>
#include <limits>
#include <math.h>

namespace LUT
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;
#define POW2_ROUND(NUM) (Pow2(Log2(NUM)))

    bool isLUTFile(const std::string& filename)
    {
        const string ext = extension(filename);

        //
        //  Add the extensions that we support.
        //

        static const char* exts[] = {"rvchlut", "rv3dlut", "lt", "3dl", "cube",
                                     "shakelt", "mga",     "vf", "3dl", "csp",
                                     "a3d",     "txt",     0};

        for (const char** p = exts; *p; p++)
        {
            if (ext == *p)
                return true;
        }

        return false;
    }

    static int comp(const LUTData::MappingPair& a,
                    const LUTData::MappingPair& b)
    {
        return a.first < b.first;
    }

    void readFile(const string& filename, LUTData& data)
    {
        const string ext = extension(filename);

        if (!fileExists(filename.c_str()))
        {
            TWK_THROW_EXC_STREAM("LUT file " << filename << " does not exist");
        }

        if (ext == "rv3dlut")
        {
            readRV3DLUT(filename, ext, data);
        }
        else if (ext == "rvchlut" || ext == "lt" || ext == "shakelt"
                 || ext == "txt")
        {
            readShakeLUT(filename, ext, data);
        }
        else if (ext == "cube")
        {
            readCubeLUT(filename, ext, data);
        }
        else if (ext == "mga")
        {
            readAppleColorLUT(filename, ext, data);
        }
        else if (ext == "vf")
        {
            readNukeVectorfield(filename, ext, data);
        }
        else if (ext == "3dl")
        {
            readInfernoLUT(filename, ext, data);
        }
        else if (ext == "csp")
        {
            readCinespaceLUT(filename, ext, data);
        }
        else if (ext == "cub")
        {
            readTruelightLUT(filename, ext, data);
        }
        else if (ext == "a3d")
        {
            readPanavisionLUT(filename, ext, data);
        }
        else
        {
            TWK_THROW_EXC_STREAM("Unknown LUT file format " << ext);
        }

        //
        //  Sort the pre-luts, just in case
        //

        for (int i = 0; i < 3; i++)
        {
            sort(data.prelut[i].begin(), data.prelut[i].end(), comp);
        }
    }

    static void ResampleLUT(LUTData& lut, int xsize, int ysize, int zsize)
    {
        vector<int> sizes(3);
        sizes[0] = lut.dimensions[0];
        sizes[1] = lut.dimensions[1];
        sizes[2] = lut.dimensions[2];

        //
        // copy the LUT data
        //

        vector<float> original_data(lut.data);
        vector<int> original_sizes(sizes);

        //
        // make the current LUT into an identity,
        // setting sizes to the new size
        //

        LUTData::Data& data = lut.data;
        data.resize(xsize * ysize * zsize * 3);

        sizes[0] = xsize;
        sizes[1] = ysize;
        sizes[2] = zsize;

        lut.dimensions[0] = xsize;
        lut.dimensions[1] = ysize;
        lut.dimensions[2] = zsize;

        IdentityLUT(data, sizes);

        //
        // apply the original LUT to the new Identity LUT pixels
        //

        for (int i = 0; i < data.size(); i += 3)
        {
            ApplyLUTtoPixel(original_data, original_sizes, data[i], data[i + 1],
                            data[i + 2]);
        }
    }

    void resamplePowerOfTwo(LUTData& lut)
    {
        if (lut.dimensions.size() > 1)
        {
            //
            // resizing 3D LUTs that aren't a power of 2 on each side
            //

            if (!Log2strict(lut.dimensions[0]) || !Log2strict(lut.dimensions[1])
                || !Log2strict(lut.dimensions[2]))
            {
                //
                // Set new_sizes to a power of 2.
                // There's a bug in our interpolation code where rounding up is
                // creating quantization, which is why we're rounding down
                // instead. (our Log2 function rounds up)
                //
                //  UPDATE: its not our code --- its the graphics card
                //  that's freaking out. (ati) I'm putting it back to
                //  round up. It definitely looks better with film luts
                //  going from 17^3 to 32^3
                //

                int xs = POW2_ROUND(lut.dimensions[0]);
                int ys = POW2_ROUND(lut.dimensions[1]);
                int zs = POW2_ROUND(lut.dimensions[2]);

                if (xs == 1)
                    xs = 2;
                if (ys == 1)
                    ys = 2;
                if (zs == 1)
                    zs = 2;

                ResampleLUT(lut, xs, ys, zs);
            }
        }
    }

    static bool mappingPair2Comp(const LUTData::MappingPair& a,
                                 const LUTData::MappingPair& b)
    {
        return a.first < b.first;
    }

    void compilePreLUT(LUTData& inlut, size_t nsamples)
    {
        const size_t s0 = inlut.prelut[0].size();
        const size_t s1 = inlut.prelut[1].size();
        const size_t s2 = inlut.prelut[2].size();

        if (s0 && s1 && s2)
        {
            inlut.prelutData.resize(nsamples * 3);

            float scales[3] = {1.0, 1.0, 1.0};
            float offsets[3] = {0.0, 0.0, 0.0};

            //
            //  First normalize the input values and record the scale.
            //

            for (size_t q = 0; q < 3; q++)
            {
                LUTData::ChannelMap& plut = inlut.prelut[q];
                float max = -numeric_limits<float>::max();
                float min = numeric_limits<float>::max();

                for (size_t i = 0; i < plut.size(); i++)
                {
                    if (plut[i].first > max)
                        max = plut[i].first;
                    if (plut[i].first < min)
                        min = plut[i].first;
                }

                scales[q] = (max - min);
                offsets[q] = min;

                for (size_t i = 0; i < plut.size(); i++)
                {
                    plut[i].first = (plut[i].first - offsets[q]) / scales[q];
                }
            }

            //
            //  Next resample them so they're all the same size
            //

            for (size_t i = 0; i < nsamples; i++)
            {
                const float t = float(i) / float(nsamples - 1);

                for (size_t q = 0; q < 3; q++)
                {
                    const LUTData::ChannelMap& plut = inlut.prelut[q];

                    LUTData::ChannelMap::const_iterator p = lower_bound(
                        plut.begin(), plut.end(), LUTData::MappingPair(t, 0),
                        mappingPair2Comp);

                    size_t ns = plut.size();
                    size_t i1;
                    size_t i0;

                    if (p == plut.end())
                    {
                        i1 = plut.size() - 1;
                        i0 = i1;
                    }
                    else
                    {
                        i1 = p - plut.begin();
                        i0 = (i1 == 0) ? i1 : (i1 - 1);
                    }

                    const float t0 = plut[i0].first;
                    const float t1 = plut[i1].first;
                    const float v0 = plut[i0].second;
                    const float v1 = plut[i1].second;

                    float v;

                    if (t1 != t0)
                    {
                        const float u = (t - t0) / (t1 - t0);
                        v = lerp(v0, v1, u);
                    }
                    else
                    {
                        v = v0;
                    }

                    inlut.prelutData[i * 3 + q] = v;
                }
            }

            Mat44f M(scales[0], 0, 0, offsets[0], 0, scales[1], 0, offsets[1],
                     0, 0, scales[2], offsets[2], 0, 0, 0, 1);

            inlut.inMatrix = M.inverted();
        }
    }

    bool simplifyPreLUT(LUTData& lut)
    {
        const size_t size0 = lut.prelut[0].size();
        const size_t size1 = lut.prelut[1].size();
        const size_t size2 = lut.prelut[2].size();
        bool linear = true;

        if (lut.prelut[0].size() < 2 && lut.prelut[1].size() < 2
            && lut.prelut[2].size() < 2)
        {
            lut.prelut[0].resize(0);
            lut.prelut[1].resize(0);
            lut.prelut[2].resize(0);
            return true;
        }

        typedef LUTData::MappingPair Pair;

        //
        //  See if they're colinear
        //

        bool colinear[3] = {false, false, false};

        for (size_t i = 0; i < 3; i++)
        {
            if (lut.prelut[i].size() >= 2)
            {
                const Pair& a = lut.prelut[i].front();
                const Pair& b = lut.prelut[i].back();
                const Vec3f va(a.first, a.second, 0);
                const Vec3f vb(b.first, b.second, 0);

                colinear[i] = true;

                for (size_t q = 1; q < lut.prelut[i].size() - 1; q++)
                {
                    Vec3f v(lut.prelut[i][q].first, lut.prelut[i][q].second, 0);

                    if (cross(vb - va, v - va) != Vec3f(0.0f))
                    {
                        colinear[i] = false;
                        linear = false;
                    }
                }
            }
        }

        if (colinear[0] && colinear[1] && colinear[2])
        {
            //
            //  If all channels are colinear we can make a matrix with a
            //  scale and a translate. This is done by finding the
            //  slope-intercept form of the line. The slope will be the
            //  scale and the intercept will be the offset.
            //
            //  find y = m x + b
            //

            Mat44f M;

            for (size_t i = 0; i < 3; i++)
            {
                const Pair& p0 = lut.prelut[i].front();
                const Pair& p1 = lut.prelut[i].back();
                const float m = (p1.second - p0.second) / (p1.first - p0.first);
                const float b = p0.second - m * p0.first;

                lut.prelut[i].clear();

                M(i, i) = m;
                M(i, 3) = b;
            }

            lut.inMatrix = M;
        }

        return linear;
    }

    struct Div
    {
        Div(float v)
            : val(v)
        {
        }

        float operator()(float in) { return in / val; }

        float val;
    };

#ifdef WIN32
#define log2(X) (std::log(double(X)) / std::log(double(2)))
#endif

    float normalize(LUTData& lut, bool powerOfTwo)
    {
        LUTData::Data::iterator i =
            max_element(lut.data.begin(), lut.data.end());
        float m = *i;

        if (powerOfTwo)
        {
            float p = pow(double(2), int(log2(m)));
            while (p < m)
                p = pow(double(2), int(log2(m) + 1));
            m = p;
        }

        transform(lut.data.begin(), lut.data.end(), lut.data.begin(), Div(m));
        return m;
    }

} // namespace LUT
