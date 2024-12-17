//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/ReadLUT.h>
#include <LUT/InfernoLUT.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/File.h>
#include <algorithm>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <stl_ext/string_algo.h>

#include <LUT/lut_utils.h>
#include <LUT/LUTmath.h>

namespace LUT
{
    using namespace std;

#ifdef PLATFORM_WINDOWS
    double log2(double x) { return log(x) / log(2.0); }
#endif

    struct Div
    {
        Div(float v)
            : val(v)
        {
        }

        float operator()(float in) { return in / val; }

        float val;
    };

    void readInfernoLUT(const string& filename, const string& type,
                        LUTData& lut)
    {
        ifstream file(UNICODE_C_STR(filename.c_str()), ios::binary);
        lut.data.resize(0);
        vector<string> buffer;
        char buf[1024];
        int depth = 0;

        //
        // get first un-commented line, which contains size and bit depth
        // information
        //

        while (nextline(file, buf, 1023))
        {
            buffer.clear();
            stl_ext::tokenize(buffer, buf, " \t\n\r");

            if (buffer.size())
            {
                //
                //  Skip the 3DMESH line if its there
                //

                if (buffer.front() == "3DMESH" || buffer.front() == "3DMESHHW")
                {
                    continue;
                }

                if (buffer.front() == "Mesh")
                {
                    depth = atoi(buffer.back().c_str());
                    continue;
                }

                //
                //  skip any lines which are in [A-Za-z].*
                //

                if (buffer.front().size())
                {
                    char ch = buffer.front()[0];
                    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
                        continue;
                }
            }

            if (buffer.size() < 2)
            {
                TWK_THROW_EXC_STREAM("format error " << filename);
            }

            int len = buffer.size(); // that's right, the number of words is the
                                     // dimensions

            //
            // it's a cube, dude
            //

            lut.dimensions.resize(3);
            lut.dimensions[0] = len;
            lut.dimensions[1] = len;
            lut.dimensions[2] = len;
            break;
        }

        // read data
        if (!Read3DLUTData(file, lut.dimensions[0], lut.dimensions[1],
                           lut.dimensions[2], 1, true, 0, lut.data))
        {
            TWK_THROW_EXC_STREAM("data parsing error " << filename);
        }

        LUTData::Data::iterator i =
            max_element(lut.data.begin(), lut.data.end());
        float m = *i;

        float p = pow(double(2), int(log2(m)));
        while (p < m)
            p = pow(double(2), int(log2(m) + 1));

        //
        //  Allow for over-range data by assuming that if maxval is "closer" to
        //  the 2N boundary beneath it, the LUT writer intended to target the
        //  smaller bit-depth, with some over-range data.
        //

        if (m != p)
        {
            float lower = pow(2.0, int(log2(m)));
            float boundary = (lower + pow(2.0, int(log2(p)))) / 2.0;
            if (fabs(m - lower) < fabs(m - p))
                p = lower;
        }
        m = p;

        transform(lut.data.begin(), lut.data.end(), lut.data.begin(), Div(m));
    }

} // namespace LUT
