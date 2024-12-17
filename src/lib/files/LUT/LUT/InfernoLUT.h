//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __LUT__InfernoLUT__h__
#define __LUT__InfernoLUT__h__
#include <LUT/ReadLUT.h>

namespace LUT
{

    //
    //  NOTE: this lut format (3dl) is actually lustre not inferno
    //

    void readInfernoLUT(const std::string& filename, const std::string& type,
                        LUTData& lut);

} // namespace LUT

#endif // __LUT__InfernoLUT__h__
