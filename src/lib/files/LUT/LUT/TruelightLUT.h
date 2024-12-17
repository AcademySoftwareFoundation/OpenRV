//******************************************************************************
// Copyright (c) 2015 Autodesk Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __LUT__TruelightLUT__h__
#define __LUT__TruelightLUT__h__
#include <LUT/ReadLUT.h>

namespace LUT
{

    void readTruelightLUT(const std::string& filename, const std::string& type,
                          LUTData& data);

} // namespace LUT

#endif // __LUT__TruelightLUT__h__
