//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <NDI/NDIModule.h>
#include <NDI/NDIVideoDevice.h>

#include <TwkExc/Exception.h>

#include <Processing.NDI.Lib.h>

namespace NDI
{

    NDIModule::NDIModule()
        : VideoModule()
    {
        open();

        if (!isOpen())
        {
            TWK_THROW_EXC_STREAM("Cannot run NDI");
        }
    }

    NDIModule::~NDIModule() { close(); }

    std::string NDIModule::name() const { return "NDIModule"; }

    std::string NDIModule::SDKIdentifier() const
    {
        std::ostringstream str;
        str << "NDI SDK Version " << NDIlib_version();
        return str.str();
    }

    std::string NDIModule::SDKInfo() const { return ""; }

    void NDIModule::open()
    {
        if (isOpen())
        {
            return;
        }

        m_NDIlib_initialized = NDIlib_initialize();
        if (!m_NDIlib_initialized)
        {
            return;
        }

        NDIVideoDevice* device = new NDIVideoDevice(this, "NDIVideoDevice");
        if (device->numVideoFormats() != 0)
        {
            m_devices.push_back(device);
        }
        else
        {
            delete device;
            device = nullptr;
        }

#ifdef PLATFORM_WINDOWS
        glewInit(NULL);
#endif
    }

    void NDIModule::close()
    {
        for (const auto& device : m_devices)
        {
            delete device;
        }
        m_devices.clear();

        if (m_NDIlib_initialized)
        {
            NDIlib_destroy();
            m_NDIlib_initialized = false;
        }
    }

    bool NDIModule::isOpen() const { return !m_devices.empty(); }

} // namespace NDI
