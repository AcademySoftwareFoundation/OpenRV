//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#if defined(PLATFORM_DARWIN) && defined(USE_METAL)

#include <IPCore/MetalContext.h>

namespace IPCore
{

    bool MetalContext::s_active = false;
    void* MetalContext::s_device = nullptr;

} // namespace IPCore

#endif // PLATFORM_DARWIN && USE_METAL
