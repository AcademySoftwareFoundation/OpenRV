//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/AppleColorLUT.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/File.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>

#include <LUT/lut_utils.h>
#include <LUT/LUTmath.h>

namespace LUT
{
    using namespace std;

    void readAppleColorLUT(const string& filename, const string& type,
                           LUTData& lut)
    {
        ifstream file(UNICODE_C_STR(filename.c_str()), ios::binary);
        lut.data.resize(0);
        vector<string> buffer;

        char buf[1024];
        int len = 0;
        int depth = 0;

        //
        // find beginning
        //

        if (!findline(file, "channel 3d", buf, 1023))
        {
            TWK_THROW_EXC_STREAM("reading 3D LUT file " << filename);
        }

        //
        // find "in" size (in = len ^ 3)
        //

        if (findline(file, "in", buf, 1023))
        {
            buffer.resize(0);
            stl_ext::tokenize(buffer, buf, " \t\n\r");

            if (buffer.size() >= 2)
            {
                len = CubeRoot(atoi(buffer[1].c_str()));
            }
            else
            {
                TWK_THROW_EXC_STREAM("format error " << filename);
            }
        }
        else
        {
            TWK_THROW_EXC_STREAM("format error " << filename);
        }

        //
        // find out
        //

        if (findline(file, "out", buf, 1023))
        {
            buffer.resize(0);
            stl_ext::tokenize(buffer, buf, " \t\n\r");

            if (buffer.size() >= 2)
            {
                depth = Log2(atoi(buffer[1].c_str()));
            }
            else
            {
                TWK_THROW_EXC_STREAM("format error " << filename);
            }
        }
        else
        {
            TWK_THROW_EXC_STREAM("format error " << filename);
        }

        //
        // got size and depth, so set up sizes
        //

        lut.dimensions.resize(3);
        lut.dimensions[0] = len;
        lut.dimensions[1] = len;
        lut.dimensions[2] = len;

        //
        // find line before data starts (starts with "values")
        //

        if (!findline(file, "values", buf, 1023))
        {
            TWK_THROW_EXC_STREAM("reading 3D LUT file " << filename);
        }

        //
        // this LUT starts with the "values" field
        // we're ASSuming it's going to always be in reverse order
        // if that's wrong, we'll have to take the values field into account
        // for now we ignore it and just set padding to 1
        //

        int padding = 1;

        //
        // read data
        //

        if (!Read3DLUTData(file, len, len, len, depth, true, padding, lut.data))
        {
            TWK_THROW_EXC_STREAM("data parsing error " << filename);
        }
    }

} // namespace LUT
