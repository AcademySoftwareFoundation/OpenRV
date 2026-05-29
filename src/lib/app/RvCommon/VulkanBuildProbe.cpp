//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <vulkan/vulkan.h>

namespace
{
    // Build-time probe only: keep one direct Vulkan symbol reference so opt-in
    // Linux builds prove both headers and link-time loader availability.
    void rvVulkanBuildProbeNoOp()
    {
        PFN_vkVoidFunction fn = vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
        (void)fn;
    }
} // namespace
