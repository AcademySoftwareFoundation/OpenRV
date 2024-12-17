//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/rvchlut.h>
#include <TwkExc/Exception.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <TwkMath/Vec4.h>

namespace LUT
{
    using namespace std;
    using namespace TwkMath;

    void readShakeLUT(const string& name, const string& type, LUTData& lut)
    {
        ifstream file(name.c_str(), ios::binary);
        LUTData::Data& data = lut.data;
        data.resize(0);
        lut.dimensions.resize(1);

        if (!file)
        {
            TWK_THROW_EXC_STREAM("Unable to open LUT file " << name);
        }

        vector<string> buffer;

        while (!file.eof())
        {
            string instr;
            bool ok = !getline(file, instr).fail();
            buffer.clear();

            stl_ext::tokenize(buffer, instr, " \t\n\r");

            Vec4f c;

            switch (buffer.size())
            {
            case 0:
                break;
            case 1:
                data.push_back(atof(buffer[0].c_str()));
                data.push_back(data.size());
                data.push_back(c[1]);
                break;
            case 2:
                data.push_back(atof(buffer[0].c_str()));
                data.push_back(atof(buffer[1].c_str()));
                data.push_back(data.size());
                break;
            default:
            case 3:
                data.push_back(atof(buffer[0].c_str()));
                data.push_back(atof(buffer[1].c_str()));
                data.push_back(atof(buffer[2].c_str()));
                break;
            }
        }

        float max = (data.size() / 3.0) - 1.0;

        for (int i = 0; i < data.size(); i++)
        {
            data[i] /= max;
        }

        lut.dimensions[0] = data.size() / 3;
    }

} // namespace LUT
