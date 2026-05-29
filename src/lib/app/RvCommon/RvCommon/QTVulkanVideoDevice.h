//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#pragma once

#ifdef PLATFORM_LINUX

#include <memory>

namespace Rv
{
    // Build-time placeholder for the future Linux Vulkan presentation device.
    class QTVulkanVideoDevice
    {
    public:
        QTVulkanVideoDevice();
        ~QTVulkanVideoDevice();

        bool isNoOp() const { return true; }
    };

    std::unique_ptr<QTVulkanVideoDevice> makeNoOpQTVulkanVideoDevice();
} // namespace Rv

#endif // PLATFORM_LINUX
