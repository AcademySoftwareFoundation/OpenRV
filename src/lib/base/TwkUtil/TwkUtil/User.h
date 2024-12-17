//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilUser_h_
#define _TwkUtilUser_h_

#include <string>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    // *****************************************************************************

    //  String that is uid on linux/mac, authenticated user of process on
    //  windows.
    TWKUTIL_EXPORT std::string uidString();

} // End namespace TwkUtil

#endif
