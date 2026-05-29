//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#pragma once

#ifdef PLATFORM_LINUX

#include <QWidget>

namespace Rv
{
    // Build-time placeholder for the future Linux Vulkan presentation view.
    class VulkanView : public QWidget
    {
    public:
        explicit VulkanView(QWidget* parent = nullptr);
        ~VulkanView() override = default;

        bool isNoOp() const { return true; }
    };
} // namespace Rv

#endif // PLATFORM_LINUX
