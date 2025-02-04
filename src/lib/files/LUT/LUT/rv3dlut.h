//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __LUT__rv3dlut__h__
#define __LUT__rv3dlut__h__
#include <LUT/ReadLUT.h>

namespace LUT
{

    void readRV3DLUT(const std::string& filename, const std::string& type,
                     LUTData& data);

} // namespace LUT

#endif // __LUT__rv3dlut__h__
