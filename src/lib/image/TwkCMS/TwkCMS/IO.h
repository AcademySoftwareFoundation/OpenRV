//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkCMS__IO__h__
#define __TwkCMS__IO__h__
#include <string>
#include <vector>
#include <TwkCMS/ColorManagementSystem.h>

namespace TwkCMS
{

    class IO
    {
    public:
        typedef std::vector<IO*> Plugins;

        IO(const char* cmsPath);
        ~IO();

    private:
        static Plugins m_plugins;
    };

} // namespace TwkCMS

#endif // __TwkCMS__IO__h__
