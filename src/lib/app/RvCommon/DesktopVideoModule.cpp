//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/DesktopVideoModule.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <RvCommon/GLView.h>
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
#include <RvCommon/VulkanDesktopVideoDevice.h>
#endif
#include <IPCore/ImageRenderer.h>
#if defined(PLATFORM_DARWIN) && defined(USE_METAL)
#include <RvCommon/MetalDesktopVideoDevice.h>
#endif
#include <stl_ext/string_algo.h>
#include <QtGui/QtGui>
#include <map>
#include <boost/algorithm/string.hpp>

#include <QtWidgets/QApplication>
#include <QScreen>

namespace Rv
{
    using namespace std;
    using namespace boost;

    //----------------------------------------------------------------------

    // force intel code path for this commit
    static bool isDarwinArm() { return true; }

    static bool isDarwinIntel() { return true; }

    static bool useQtOnDarwinArm() { return true; }

    DesktopVideoModule::DesktopVideoModule(NativeDisplayPtr np, TwkGLF::GLVideoDevice* shareDevice)
        : VideoModule()
    {
        m_devices = DesktopVideoDevice::createDesktopVideoDevices(this, shareDevice);
    }

    DesktopVideoModule::~DesktopVideoModule() {}

    bool DesktopVideoModule::rebuildDevices(const TwkGLF::GLVideoDevice* shareDevice, bool targetNative)
    {
#if !defined(PLATFORM_LINUX) && !defined(PLATFORM_WINDOWS) && !(defined(PLATFORM_DARWIN) && defined(USE_METAL))
        targetNative = false;
#endif

        bool currentNative = false;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS) || (defined(PLATFORM_DARWIN) && defined(USE_METAL))
        for (TwkApp::VideoDevice* device : m_devices)
        {
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
            if (dynamic_cast<VulkanDesktopVideoDevice*>(device))
#else
            if (dynamic_cast<MetalDesktopVideoDevice*>(device))
#endif
            {
                currentNative = true;
                break;
            }
        }
#endif

        if (!m_devices.empty() && currentNative == targetNative)
        {
            return false;
        }

        //  Retire rather than destroy: closing a device destroys a native window,
        //  which pumps the event loop and repaints through display groups that
        //  still hold these devices. The caller re-points the graph, then calls
        //  purgeRetiredDevices().
        m_retiredDevices.insert(m_retiredDevices.end(), m_devices.begin(), m_devices.end());
        m_devices.clear();

        m_devices = DesktopVideoDevice::createDesktopVideoDevices(this, shareDevice, targetNative);

        return true;
    }

    void DesktopVideoModule::purgeRetiredDevices()
    {
        //  Swap out first: close() pumps the event loop and can re-enter this module.
        VideoDevices retired;
        retired.swap(m_retiredDevices);

        for (TwkApp::VideoDevice* device : retired)
        {
            if (device->isOpen())
            {
                device->close();
            }
            delete device;
        }
    }

    string DesktopVideoModule::name() const { return "Desktop"; }

    void DesktopVideoModule::open() {}

    void DesktopVideoModule::close() {}

    bool DesktopVideoModule::isOpen() const { return true; }

    TwkApp::VideoDevice* DesktopVideoModule::deviceFromPosition(int x, int y) const
    {
        TwkApp::VideoDevice* device = 0;

        const QList<QScreen*> screens = QGuiApplication::screens();
        for (int screen = 0; screen < screens.size(); ++screen)
        {
            // Check if the point is part of the screen.
            if (screens[screen]->geometry().contains(QPoint(x, y)))
            {
                for (int i = 0; i < m_devices.size(); ++i)
                {
                    //
                    //  These devices may be NVDesktopVideoDevices or
                    //  DeskTopVideoDevices.
                    //
                    TwkApp::VideoDevice* d = m_devices[i];

                    if (DesktopVideoDevice* dd = dynamic_cast<DesktopVideoDevice*>(d))
                    {
                        if (dd->qtScreen() == screen)
                        {
                            device = d;
                            break;
                        }
                    }
                }
            }
        }

        return device;
    }
} // namespace Rv
