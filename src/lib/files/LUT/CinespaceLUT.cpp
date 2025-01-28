//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/CinespaceLUT.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/File.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <LUT/lut_utils.h>

#define WHITESPACE " \t\r\n"

namespace LUT
{
    using namespace std;

    string nextline(istream& in, float* gamma = 0)
    {
        if (!in)
        {
            TWK_THROW_EXC_STREAM("CSP: premature end-of-file");
        }

        string s;
        bool metadata = false;

        //
        //  Skip any non-whitespace lines that are meta data. This seems like a
        //  wacked part of the csp spec: the metadata is not constrained to any
        //  portion of the file. RSR's reader however assumes its after line 2.
        //

        while (1)
        {
            if (!std::getline(in, s))
                return "";

            //
            //  Skip "BEGIN METADATA" and "END METADATA" and everything
            //  in between.
            //

            if (s.find("END METADATA") == 0)
            {
                metadata = false;
                continue;
            }

            if (metadata)
            {
                if (gamma)
                {
                    string gammaString("conditioningGamma=");
                    if (s.substr(0, gammaString.size()) == gammaString)
                    {
                        *gamma = atof(s.substr(gammaString.size()).c_str());
                        if (*gamma <= 0.0)
                            *gamma = 1.0;
                    }
                }
                continue;
            }

            if (s.find("BEGIN METADATA") == 0)
            {
                metadata = true;
                continue;
            }

            //
            //  Skip white space or empty lines or lines which begin with a
            //  whitespace (weird stuff).
            //

            if (s.find_first_not_of(WHITESPACE) == string::npos
                || (s.size() && (s[0] == ' ' || s[0] == '\t')))
            {
                continue;
            }

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

    void expect(istream& in, const string& line)
    {
        if (nextline(in) != line)
        {
            TWK_THROW_EXC_STREAM("CSP: expected " << line);
        }
    }

    string oneof(istream& in, const string& line1, const string& line2)
    {
        string s = nextline(in);

        if (s != line1 && s != line2)
        {
            TWK_THROW_EXC_STREAM("CSP: expected " << line1 << " or " << line2);
        }

        return s;
    }

    void readCinespaceLUT(const string& filename, const string& type,
                          LUTData& lut)
    {
        ifstream file(UNICODE_C_STR(filename.c_str()), ios::binary);
        lut.data.resize(0);
        vector<string> buffer;
        string line;

        //
        //  File should begin with "CSPLUTV100"
        //

        expect(file, "CSPLUTV100");

        //
        // the next line tells us if it's 1D or 3D
        //

        bool is3d = oneof(file, "3D", "1D") == "3D";

        //
        // pre-LUT
        //

        float conditioningGamma = 1.0;

        for (int i = 0; i < 3; i++)
        {
            //
            //  Read the size
            //

            line = nextline(file, &conditioningGamma);
            if (conditioningGamma != 1.0)
            {
                lut.conditioningGamma = conditioningGamma;
            }
            size_t s = atoi(line.c_str());
            lut.prelut[i].resize(s);

            //
            //  Read the inputs
            //

            line = nextline(file);
            buffer.clear();
            stl_ext::tokenize(buffer, line, " \t\n\r");

            if (buffer.size() != s)
            {
                TWK_THROW_EXC_STREAM("prelut size mismatch, expected "
                                     << s << " items, found " << buffer.size()
                                     << " while reading " << filename);
            }

            //
            //  convert the inputs
            //

            for (size_t q = 0; q < s; q++)
            {
                lut.prelut[i][q].first =
                    powf(atof(buffer[q].c_str()), lut.conditioningGamma);
            }

            //
            //  read the outputs
            //

            line = nextline(file);
            buffer.clear();
            stl_ext::tokenize(buffer, line, " \t\n\r");

            if (buffer.size() != s)
            {
                TWK_THROW_EXC_STREAM("prelut size mismatch, expected "
                                     << s << " items, found " << buffer.size()
                                     << " while reading " << filename);
            }

            //
            //  Convert the outputs
            //

            for (size_t q = 0; q < s; q++)
            {
                lut.prelut[i][q].second = atof(buffer[q].c_str());
            }
        }

        //
        // the next (non-empty) line should have sizes
        //

        line = nextline(file);

        buffer.clear();
        stl_ext::tokenize(buffer, line, " \t\n\r");

        if (buffer.size() == 3 && is3d)
        {
            lut.dimensions.resize(3);
            lut.dimensions[0] = atoi(buffer[0].c_str());
            lut.dimensions[1] = atoi(buffer[1].c_str());
            lut.dimensions[2] = atoi(buffer[2].c_str());
        }
        else if (buffer.size() == 1 && !is3d)
        {
            lut.dimensions.resize(1);
            lut.dimensions[0] = atoi(buffer[0].c_str());
        }
        else
        {
            TWK_THROW_EXC_STREAM("CSP: unexpected number of lut dimensions in "
                                 << filename);
        }

        if (is3d)
        {
            if (!Read3DLUTData(file, lut.dimensions[0], lut.dimensions[1],
                               lut.dimensions[2], FLOAT_DEPTH, false, 0,
                               lut.data))
            {
                TWK_THROW_EXC_STREAM("data parsing error " << filename);
            }
        }
        else
        {
            if (!ReadChannelLUTData(file, lut.dimensions[0], FLOAT_DEPTH, false,
                                    0, lut.data))
            {
                TWK_THROW_EXC_STREAM("data parsing error " << filename);
            }
        }
    }

} // namespace LUT
