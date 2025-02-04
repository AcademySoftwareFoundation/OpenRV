//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef _TwkQtBaseQtUtil_h_
#define _TwkQtBaseQtUtil_h_

#include <string>

namespace TwkQtBase
{

    std::string encode(std::string s, int key = 0);
    std::string decode(std::string s, int key = 0);

}; // namespace TwkQtBase

#endif //_TwkQtBaseQtUtil_h_
