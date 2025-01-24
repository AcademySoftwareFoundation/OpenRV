//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <iostream>
#include <TwkApp/VideoModule.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GL.h>
#ifdef PLATFORM_WINDOWS
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglew.h>
#endif

namespace AJADevices
{

    class AJAModule : public TwkApp::VideoModule
    {
    public:
        enum class OperationMode
        {
            ProMode,
            SimpleMode
        };

        AJAModule(NativeDisplayPtr, unsigned int appID, OperationMode);
        ~AJAModule() override;

        [[nodiscard]] std::string name() const override;
        [[nodiscard]] std::string SDKIdentifier() const override;
        [[nodiscard]] std::string SDKInfo() const override;
        void open() override;
        void close() override;
        [[nodiscard]] bool isOpen() const override;

    private:
        OperationMode m_mode{OperationMode::ProMode};
        unsigned int m_appID{0};
    };

} // namespace AJADevices
