//******************************************************************************
// Copyright (c) 2023 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#pragma once

#include <PyMediaLibrary/PyMediaLibrary.h>
#include <IPCore/Application.h>

namespace Rv
{
    class RvConsoleApplication : public IPCore::Application
    {
    public:
        RvConsoleApplication();
        virtual ~RvConsoleApplication() = default;

    private:
        TwkMediaLibrary::PyMediaLibrary m_pyLibrary;
    };
} // namespace Rv
