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

    DesktopVideoModule::DesktopVideoModule(NativeDisplayPtr np, QTGLVideoDevice* shareDevice)
        : VideoModule()
    {
        m_devices = DesktopVideoDevice::createDesktopVideoDevices(this, shareDevice);
    }

    DesktopVideoModule::~DesktopVideoModule() {}

    bool DesktopVideoModule::rebuildDevices(const QTGLVideoDevice* shareDevice, bool targetVulkan)
    {
#if !defined(PLATFORM_LINUX) && !defined(PLATFORM_WINDOWS)
        targetVulkan = false;
#endif

        bool currentVulkan = false;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
        for (size_t i = 0; i < m_devices.size(); ++i)
        {
            if (dynamic_cast<VulkanDesktopVideoDevice*>(m_devices[i]))
            {
                currentVulkan = true;
                break;
            }
        }
#endif

        if (!m_devices.empty() && currentVulkan == targetVulkan)
        {
            return false;
        }

        // close() releases the Vulkan swapchain or GL ScreenView before the delete.
        for (size_t i = 0; i < m_devices.size(); ++i)
        {
            if (m_devices[i]->isOpen())
            {
                m_devices[i]->close();
            }
            delete m_devices[i];
        }
        m_devices.clear();

        m_devices = DesktopVideoDevice::createDesktopVideoDevices(this, shareDevice, targetVulkan);

        return true;
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
