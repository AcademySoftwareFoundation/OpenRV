//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __LUT__rvchlut__h__
#define __LUT__rvchlut__h__
#include <LUT/ReadLUT.h>

namespace LUT
{

    void readShakeLUT(const std::string& filename, const std::string& type,
                      LUTData& data);

} // namespace LUT

#endif // __LUT__rvchlut__h__
