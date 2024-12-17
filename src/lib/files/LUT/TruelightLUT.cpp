//******************************************************************************
// Copyright (c) 2015 Autodesk Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/TruelightLUT.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/File.h>
#include <fstream>
#include <iostream>
#include <LUT/lut_utils.h>

#define WHITESPACE " \t\r\n"

namespace LUT
{
    using namespace std;

    namespace
    {

        string nextline(istream& in)
        {
            if (!in)
            {
                TWK_THROW_EXC_STREAM("CUB: premature end-of-file");
            }

            string s;
            while (1)
            {
                if (!std::getline(in, s))
                    return "";

                //
                //  Skip white space or empty lines or lines which begin with a
                //  whitespace (weird stuff).
                //

                if (s.find_first_not_of(WHITESPACE) == string::npos)
                    continue;

                //
                //  get rid of DOS return char
                //

                if (s.size() && s[s.size() - 1] == '\r')
                    s.resize(s.size() - 1);

                //
                //  Found a non-blank, non-metadata line, so exit loop
                //

                break;
            }

            return s;
        }

        string oneof(istream& in, const string& line1, const string& line2)
        {
            string s = nextline(in);

            if (s != line1 && s != line2)
            {
                TWK_THROW_EXC_STREAM("CUB: expected " << line1 << " or "
                                                      << line2);
            }

            return s;
        }
    } // namespace

    void readTruelightLUT(const string& filename, const string& type,
                          LUTData& lut)
    {
        ifstream file(UNICODE_C_STR(filename.c_str()), ios::binary);
        lut.data.resize(0);
        vector<string> buffer;
        string line;

        //
        //  File should begin with "# Truelight Cube v2.0/v2.1"
        //

        string last =
            oneof(file, "# Truelight Cube v2.0", "# Truelight Cube v2.1");

        //
        //  Look for header info
        //

        line = nextline(file);

        // Variables for input
        int inputLength = 0;
        int inputCount = 0;

        //
        //  Walk through the lines until we hit the "# end"
        //

        while (line != "")
        {
            if (line.substr(0, 7) == "# iDims"
                || line.substr(0, 7) == "# oDims")
            {
                int dims = atoi(line.substr(8, line.size() - 1).c_str());
                if (dims != 3)
                {
                    TWK_THROW_EXC_STREAM(
                        "CUB: Only 3 dimensions supported. Found " << dims);
                }
            }

            if (line.substr(0, 11) == "# lutLength")
            {
                inputLength = atoi(line.substr(12, line.size() - 1).c_str());
            }
            else if (line.substr(0, 7) == "# width")
            {
                lut.dimensions.resize(3);
                string dims = line.substr(8, line.size() - 1);
                int firstNWS = dims.find_first_not_of(WHITESPACE);
                if (firstNWS != string::npos)
                {
                    dims = dims.substr(firstNWS, dims.size() - firstNWS);
                    int lastPos = 0;
                    int spacePos = dims.find_first_of(WHITESPACE, lastPos);
                    for (int i = 0; i < 3; i++)
                    {
                        if (spacePos != string::npos || i == 2)
                        {
                            string dimStr =
                                dims.substr(lastPos, (spacePos - lastPos));
                            lut.dimensions[i] = atoi(dimStr.c_str());
                            lastPos = spacePos + 1;
                        }
                        spacePos = dims.find_first_of(WHITESPACE, lastPos);
                    }
                }
            }
            else if (line.substr(0, 10) == "# InputLUT")
            {
                if (inputLength == 0)
                {
                    TWK_THROW_EXC_STREAM("CUB: Unknown InputLut lutLength");
                }

                lut.prelut[0].resize(inputLength);
                lut.prelut[1].resize(inputLength);
                lut.prelut[2].resize(inputLength);

                for (int l = 0; l < inputLength; l++)
                {
                    line = nextline(file);
                    if (line == "")
                    {
                        TWK_THROW_EXC_STREAM("CUB: Incomplete InputLut");
                    }

                    float step = double(l) / double(inputLength - 1);
                    int lastPos = line.find_first_not_of(WHITESPACE);
                    int spacePos = line.find_first_of(WHITESPACE, lastPos);
                    for (int i = 0; i < 3; i++)
                    {
                        if (spacePos != string::npos || i == 2)
                        {
                            string iLutStr =
                                line.substr(lastPos, (spacePos - lastPos));
                            lut.prelut[i][l].first = step;
                            lut.prelut[i][l].second =
                                atof(iLutStr.c_str()) / (lut.dimensions[i] - 1);
                            lastPos = spacePos + 1;
                        }
                        spacePos = line.find_first_of(WHITESPACE, lastPos);
                    }
                }
            }
            else if (line.substr(0, 6) == "# Cube")
            {
                if (!Read3DLUTData(file, lut.dimensions[0], lut.dimensions[1],
                                   lut.dimensions[2], FLOAT_DEPTH, false, 0,
                                   lut.data))
                {
                    TWK_THROW_EXC_STREAM("CUB: data parsing error "
                                         << filename);
                }
            }
            else if (line.substr(0, 5) == "# end")
            {
                line = "";
                break;
            }
            last = line;
            line = nextline(file);
        }
    }

} // namespace LUT
