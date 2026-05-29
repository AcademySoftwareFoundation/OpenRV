//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifdef PLATFORM_LINUX

#include <RvCommon/QTVulkanVideoDevice.h>

namespace Rv
{
    QTVulkanVideoDevice::QTVulkanVideoDevice() = default;

    QTVulkanVideoDevice::~QTVulkanVideoDevice() = default;

    std::unique_ptr<QTVulkanVideoDevice> makeNoOpQTVulkanVideoDevice()
    {
        return std::make_unique<QTVulkanVideoDevice>();
    }
} // namespace Rv

#endif // PLATFORM_LINUX
