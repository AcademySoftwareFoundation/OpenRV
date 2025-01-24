//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <AJADevices/AJAModule.h>
#include <AJADevices/KonaVideoDevice.h>
#include <TwkExc/Exception.h>
#include <TwkGLF/GLFBO.h>
#ifdef PLATFORM_DARWIN
#include <TwkGLF/GL.h>
#endif
#ifdef PLATFORM_WINDOWS
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#endif
#include <sstream>
#include "ajatypes.h"
#include "ntv2devicescanner.h"
#include <ntv2card.h>

namespace AJADevices
{
    using namespace std;

    AJAModule::AJAModule(NativeDisplayPtr p, unsigned int appID,
                         OperationMode mode)
        : VideoModule()
        , m_mode(mode)
        , m_appID(appID)
    {
        open();

        if (!isOpen())
        {
            TWK_THROW_EXC_STREAM("AJA: no devices found");
        }
    }

    AJAModule::~AJAModule() { close(); }

    string AJAModule::name() const
    {
        return m_mode == OperationMode::SimpleMode ? "AJA (Control Panel)"
                                                   : "AJA";
    }

    string AJAModule::SDKIdentifier() const
    {
        ostringstream str;
        str << "AJA NTV2 SDK Version " << AJA_NTV2_SDK_VERSION_MAJOR << "."
            << AJA_NTV2_SDK_VERSION_MINOR << "." << AJA_NTV2_SDK_VERSION_POINT;
        return str.str();
    }

    string AJAModule::SDKInfo() const { return ""; }

    void AJAModule::open()
    {
        if (isOpen())
        {
            return;
        }

        CNTV2Card device;
        ULWord deviceIndex = 0;

        while (CNTV2DeviceScanner::GetDeviceAtIndex(deviceIndex, device))
        {
            auto konaVideoDevice = std::make_unique<KonaVideoDevice>(
                this, device.GetDisplayName(), deviceIndex, m_appID,
                static_cast<KonaVideoDevice::OperationMode>(m_mode));

            m_devices.push_back(konaVideoDevice.release());

            deviceIndex++;
        }

#ifdef PLATFORM_WINDOWS
        GLenum error = glewInit(nullptr);
        if (error != GLEW_OK)
        {
            std::string message = "AJA: GLEW initialization failed: ";
            throw std::runtime_error(
                message
                + reinterpret_cast<const char*>(glewGetErrorString(error)));
        }
#endif
    }

    void AJAModule::close()
    {
        for (size_t i = 0; i < m_devices.size(); i++)
            delete m_devices[i];
        m_devices.clear();
    }

    bool AJAModule::isOpen() const { return !m_devices.empty(); }

} // namespace AJADevices
