//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <LUT/lut_utils.h>

#include <string>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stl_ext/string_algo.h>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <LUT/LUTmath.h>

#define BUF_LEN 1024
#define WHITESPACE " \t\r\n"

#define string2float(STR, DEPTH, DEN) \
    ((DEPTH) == 32 ? atof(STR) : float(double(atoi(STR)) / double(DEN)))

namespace LUT
{
    using namespace std;

    //
    // gets the next line that isn't empty or commented out
    //

    bool nextline(std::ifstream& file, char* buf, int len)
    {
        string s;

        //
        //  This std::getline() is a global function in iostream
        //

        while (std::getline(file, s))
        {
            //
            //  This is brutal, but resilient. Tokenize the line and then
            //  test whether its a "good" next line. This will allow for
            //  some cases that might be outside the file spec, but are
            //  none-the-less semantically ok. For example it will ignore
            //  whitespace preceding a comment or a line of only spaces
            //  and tabs.
            //

            vector<string> tokens;
            stl_ext::tokenize(tokens, s, WHITESPACE);

            if (tokens.size() && tokens.front()[0] != '#')
            {
                if (s.size() < len)
                {
                    strcpy(buf, s.c_str());
                    return true;
                }
            }
        }

        return false;
    }

    //
    // searches for line beginning with *match string
    //

    bool findline(std::ifstream& file, const char* match, char* buf, int len)
    {
        while (nextline(file, buf, len))
        {
            if (!strncmp(buf, match, strlen(match)))
            {
                return true;
            }
        }

        return false;
    }

    //
    // reads LUT data into memory based on parameters
    //

    bool ReadChannelLUTData(std::ifstream& file, int size, int depth,
                            bool reverse_order, int in_padding,
                            std::vector<float>& lut)
    {
        bool no_eof = true;

        char buf[BUF_LEN + 1];

        vector<string> vals;

        //
        // find the max value for this bit depth
        //

        int den = Pow2(depth) - 1;

        int entries = size;

        lut.resize(entries * 3);
        int i = 0;

        for (i = 0; i < entries && (no_eof = nextline(file, buf, BUF_LEN)); i++)
        {
            vals.clear();
            stl_ext::tokenize(vals, buf, WHITESPACE);

            if (vals.size() < (3 + in_padding))
            {
                return false;
            }

            int place = i * 3;

            lut[place + 0] =
                string2float(vals[0 + in_padding].c_str(), depth, den);
            lut[place + 1] =
                string2float(vals[1 + in_padding].c_str(), depth, den);
            lut[place + 2] =
                string2float(vals[2 + in_padding].c_str(), depth, den);
        }

        if (i != entries)
        {
            cerr << "ERROR: reading 1D lut data: expected " << entries
                 << " values but found " << i << endl;
        }

        return no_eof;
    }

    bool Read3DLUTData(std::ifstream& file, int dx, int dy, int dz, int depth,
                       bool reverse_order, int in_padding,
                       std::vector<float>& lut)
    {
        bool no_eof = true;
        char buf[BUF_LEN + 1];
        vector<string> vals;

        //
        // find the max value for this bit depth
        //

        int den = Pow2(depth) - 1;
        int entries = dx * dy * dz;

        lut.resize(entries * 3);

        for (int i = 0; i < entries && (no_eof = nextline(file, buf, BUF_LEN));
             i++)
        {
            vals.clear();
            stl_ext::tokenize(vals, buf, WHITESPACE);

            if (vals.size() < (3 + in_padding))
            {
                return false;
            }

            int place;

            if (reverse_order)
            {
                int x_place = i / (dy * dz);
                int y_place = (i - (x_place * (dy * dz))) / dz;
                int z_place = i - ((x_place * (dy * dz)) + (y_place * dz));

                place = x_place + (y_place * dx) + (z_place * (dx * dy));

                place *= 3;
            }
            else
            {
                place = i * 3;
            }

            lut[place + 0] =
                string2float(vals[0 + in_padding].c_str(), depth, den);
            lut[place + 1] =
                string2float(vals[1 + in_padding].c_str(), depth, den);
            lut[place + 2] =
                string2float(vals[2 + in_padding].c_str(), depth, den);
        }

        return no_eof;
    }

} // namespace LUT
