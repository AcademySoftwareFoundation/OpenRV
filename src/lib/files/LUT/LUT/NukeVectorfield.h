//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __LUT__NukeVectorfield__h__
#define __LUT__NukeVectorfield__h__
#include <LUT/ReadLUT.h>

namespace LUT
{

    void readNukeVectorfield(const std::string& filename,
                             const std::string& type, LUTData& lut);

} // namespace LUT

#endif // __LUT__NukeVectorfield__h__
