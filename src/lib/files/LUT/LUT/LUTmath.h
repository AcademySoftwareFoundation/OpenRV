//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __LUT__LUTmath__h__
#define __LUT__LUTmath__h__

#include <vector>

namespace LUT
{

    //
    // some handy int math functions
    //

    int CubeRoot(int in);   // out = in^(1/3)
    int Pow2(int in);       // out = 2^in
    int Log2(int in);       // 2^out = in
    int Log2strict(int in); // returns 0 if (in) isn't a square

    //
    // LUT processing functions
    //

    void IdentityLUT(std::vector<float>& data, std::vector<int>& sizes);

    void ApplyLUTtoPixel(std::vector<float>& data, std::vector<int>& sizes,
                         float& r, float& g, float& b);

} // namespace LUT

#endif // __LUT__LUTmath__h__
