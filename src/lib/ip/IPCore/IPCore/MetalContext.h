//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once
#if defined(PLATFORM_DARWIN) && defined(USE_METAL)

namespace IPCore
{

    //
    //  MetalContext — simple singleton-like state holder that tracks whether
    //  the active render path is Metal and provides access to the MTLDevice.
    //
    //  Wrapped in void* to keep Objective-C types out of C++ headers.
    //

    class MetalContext
    {
    public:
        static bool isActive() { return s_active; }

        static void setActive(bool v) { s_active = v; }

        static void* device() { return s_device; } // id<MTLDevice>

        static void setDevice(void* d) { s_device = d; }

    private:
        static bool s_active;
        static void* s_device;
    };

} // namespace IPCore
#endif // PLATFORM_DARWIN && USE_METAL
