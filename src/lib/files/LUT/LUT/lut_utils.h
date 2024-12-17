//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __LUT__lut_utils__h__
#define __LUT__lut_utils__h__
#include <fstream>
#include <vector>
#include <sstream>
#include <LUT/ReadLUT.h>

namespace LUT
{

    enum
    {
        FLOAT_DEPTH = 32,
        NO_PADDING = 0
    };

    //
    // gets the next line that isn't empty or commented out
    //

    bool nextline(std::ifstream& file, char* buf, int len);

    //
    // searches for line beginning with *match string
    //

    bool findline(std::ifstream& file, const char* match, char* buf, int len);

    // reads LUT data into memory based on parameters
    //
    // file: the ifstream
    // size: dimensions of the lut
    // depth: bit depth of samples in file
    //          all values will be converted to float
    //          32 here means the samples are already floar
    // reverse_order: for 3D LUTs, means the RGB order is reversed
    // in_padding: how many words we have to skip over before the actual data is
    // read lut: the float buffer we're reading into - this function will resize
    // it for you

    bool ReadChannelLUTData(std::ifstream& file, int size, int depth,
                            bool reverse_order, int in_padding,
                            std::vector<float>& lut);

    bool Read3DLUTData(std::ifstream& file, int xsize, int ysize, int zsize,
                       int depth, bool reverse_order, int in_padding,
                       std::vector<float>& lut);

} // namespace LUT

#endif // __LUT__lut_utils__h__
