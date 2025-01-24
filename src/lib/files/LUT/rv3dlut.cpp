//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/rv3dlut.h>
#include <TwkExc/Exception.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <LUT/lut_utils.h>

namespace LUT
{
    using namespace std;

    void readRV3DLUT(const string& filename, const string& type, LUTData& lut)
    {
        ifstream file(filename.c_str(), ios::binary);
        lut.data.resize(0);
        vector<string> buffer;

        char buf[1024];
        lut.dimensions.resize(3);

        //
        // get size from first line
        //

        if (nextline(file, buf, 1023))
        {
            buffer.resize(0);
            stl_ext::tokenize(buffer, buf, " \t\n\r");

            if (buffer.size() >= 3)
            {
                lut.dimensions[0] = atoi(buffer[0].c_str());
                lut.dimensions[1] = atoi(buffer[1].c_str());
                lut.dimensions[2] = atoi(buffer[2].c_str());
            }
            else
            {
                TWK_THROW_EXC_STREAM("rv3dlut: format error " << filename);
            }
        }
        else
        {
            TWK_THROW_EXC_STREAM("rv3dlut: error reading 3D LUT file "
                                 << filename);
        }

        //
        // read data
        //

        if (!Read3DLUTData(file, lut.dimensions[0], lut.dimensions[1],
                           lut.dimensions[2], FLOAT_DEPTH, false, NO_PADDING,
                           lut.data))
        {
            TWK_THROW_EXC_STREAM("rv3dlut: data parsing error " << filename);
        }
    }

} // namespace LUT
