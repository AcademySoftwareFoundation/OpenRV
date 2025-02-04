//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/cubelut.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/File.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <LUT/lut_utils.h>

namespace LUT
{
    using namespace std;

    void readCubeLUT(const string& filename, const string& type, LUTData& lut)
    {
        ifstream file(UNICODE_C_STR(filename.c_str()), ios::binary);
        lut.data.resize(0);
        vector<string> buffer;

        char buf[1024];

        //
        // get size from LUT_3D_SIZE
        //

        if (findline(file, "LUT_3D_SIZE", buf, 1023))
        {
            buffer.resize(0);
            stl_ext::tokenize(buffer, buf, " \t\n\r");

            if (buffer.size() >= 2)
            {
                lut.dimensions.resize(3);
                lut.dimensions[0] = atoi(buffer[1].c_str());
                lut.dimensions[1] = atoi(buffer[1].c_str());
                lut.dimensions[2] = atoi(buffer[1].c_str());
            }
            else
            {
                TWK_THROW_EXC_STREAM("format error " << filename);
            }
        }
        else
        {
            TWK_THROW_EXC_STREAM("reading 3D LUT file " << filename);
        }

        //
        // read data
        //

        if (!Read3DLUTData(file, lut.dimensions[0], lut.dimensions[1],
                           lut.dimensions[2], FLOAT_DEPTH, false, NO_PADDING,
                           lut.data))
        {
            TWK_THROW_EXC_STREAM("data parsing error " << filename);
        }
    }

} // namespace LUT
