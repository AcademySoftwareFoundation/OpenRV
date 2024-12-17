//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __LUT__PanavisionLUT__h__
#define __LUT__PanavisionLUT__h__
#include <LUT/ReadLUT.h>

namespace LUT
{

    void readPanavisionLUT(const std::string& filename, const std::string& type,
                           LUTData& data);

} // namespace LUT

#endif // __LUT__PanavisionLUT__h__
