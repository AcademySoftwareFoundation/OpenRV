//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/NukeVectorfield.h>
#include <TwkExc/Exception.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <LUT/lut_utils.h>
#include <TwkMath/Iostream.h>

namespace LUT
{
    using namespace std;

    void readNukeVectorfield(const string& filename, const string& type,
                             LUTData& lut)
    {
        ifstream file(filename.c_str(), ios::binary);
        lut.data.resize(0);
        vector<string> buffer;

        char buf[1024];

        //
        // find size
        //

        if (findline(file, "grid_size", buf, 1023))
        {
            buffer.clear();
            stl_ext::tokenize(buffer, buf, " \t\n\r");

            if (buffer.size() >= 4)
            {
                lut.dimensions.resize(3);
                lut.dimensions[0] = atoi(buffer[1].c_str());
                lut.dimensions[1] = atoi(buffer[2].c_str());
                lut.dimensions[2] = atoi(buffer[3].c_str());
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

#if 0
    if (findline(file, "global_transform", buf, 1023))
    {
        buffer.clear();
        stl_ext::tokenize(buffer, buf, " \t\n\r");

        LUTData::Mat44f& M = lut.inMatrix;

        if (buffer.size() >= 17)
        {
            for (int y = 0, i=1; y < 4; y++)
                for (int x = 0; x < 4; x++)
                    M(y,x) = atof(buffer[i++].c_str());

            //cout << " M = " << endl << M << endl;
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
#endif

        //
        // line before data starts
        //

        if (!findline(file, "data", buf, 1023))
        {
            TWK_THROW_EXC_STREAM("reading 3D LUT file " << filename);
        }

        //
        // read data
        //

        if (!Read3DLUTData(file, lut.dimensions[0], lut.dimensions[1],
                           lut.dimensions[2], FLOAT_DEPTH, true, 0, lut.data))
        {
            TWK_THROW_EXC_STREAM("data parsing error " << filename);
        }
    }

} // namespace LUT
