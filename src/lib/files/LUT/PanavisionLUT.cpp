//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/PanavisionLUT.h>
#include <TwkExc/Exception.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>

#include <LUT/lut_utils.h>
#include <LUT/LUTmath.h>

namespace LUT
{
    using namespace std;

    void readPanavisionLUT(const string& filename, const string& type,
                           LUTData& lut)
    {
        ifstream file(filename.c_str(), ios::binary);
        lut.data.resize(0);
        vector<string> buffer;

        char buf[1024];

        //
        // get size from # entries=4913
        // note this line would usually be ignored as a comment
        //

        int len = 0;

        string s;

        while (getline(file, s))
        {
            if (!strncmp(s.c_str(), "# entries=", 10))
            {
                char buf[64];

                strcpy(buf, s.c_str());

                char* val_s = &buf[10];

                len = CubeRoot(atoi(val_s));

                break;
            }
        }

        if (len > 0)
        {
            lut.dimensions.resize(3);
            lut.dimensions[0] = len;
            lut.dimensions[1] = len;
            lut.dimensions[2] = len;
        }
        else
        {
            TWK_THROW_EXC_STREAM("format error " << filename);
        }

        //
        // read data
        //

        if (!Read3DLUTData(file, len, len, len, 10, true, NO_PADDING, lut.data))
        {
            TWK_THROW_EXC_STREAM("data parsing error " << filename);
        }
    }

} // namespace LUT
