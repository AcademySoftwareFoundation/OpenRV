//******************************************************************************
// Copyright (c) 2014 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __CDL__cdl_utils__h__
#define __CDL__cdl_utils__h__
#include <vector>
#include <TwkMath/Vec3.h>
#include <string>

namespace CDL
{

    struct ColorCorrection
    {
        ColorCorrection()
            : slope(TwkMath::Vec3f(1.0, 1.0, 1.0))
            , offset(TwkMath::Vec3f(0.0, 0.0, 0.0))
            , power(TwkMath::Vec3f(1.0, 1.0, 1.0))
            , saturation(1.0) {};
        ~ColorCorrection() {};

        TwkMath::Vec3f slope;
        TwkMath::Vec3f offset;
        TwkMath::Vec3f power;
        float saturation;
    };

    typedef std::vector<ColorCorrection> ColorCorrectionCollection;

    bool isCDLFile(const std::string& filepath);

    ColorCorrectionCollection readCDL(std::string filepath);

} // namespace CDL

#endif // __CDL__cdl_utils__h__
