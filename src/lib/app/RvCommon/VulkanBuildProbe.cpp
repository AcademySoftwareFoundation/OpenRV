//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <vulkan/vulkan.h>

#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
#include <RvCommon/QTVulkanVideoDevice.h>
#endif

namespace
{
    // Build-time probe only: keep one direct Vulkan symbol reference so opt-in
    // Linux/Windows builds prove both headers and link-time loader availability.
    void rvVulkanBuildProbeNoOp()
    {
        PFN_vkVoidFunction fn = vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
        (void)fn;
    }
} // namespace
