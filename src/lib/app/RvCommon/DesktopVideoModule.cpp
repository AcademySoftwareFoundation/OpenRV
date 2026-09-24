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
        //
        //  targetNative is the live main-view backend, decided by the caller.
        //  Compare it to the backend the current devices were built with. On
        //  platforms without a native presentation backend it is always false,
        //  so this is a no-op.
        //
#if !defined(PLATFORM_LINUX) && !defined(PLATFORM_WINDOWS) && !(defined(PLATFORM_DARWIN) && defined(USE_METAL))
        targetNative = false;
#endif

        bool currentNative = false;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS) || (defined(PLATFORM_DARWIN) && defined(USE_METAL))
        for (size_t i = 0; i < m_devices.size(); ++i)
        {
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
            if (dynamic_cast<VulkanDesktopVideoDevice*>(m_devices[i]))
#else
            if (dynamic_cast<MetalDesktopVideoDevice*>(m_devices[i]))
#endif
            {
                currentNative = true;
                break;
            }
        }
#endif

        //
        //  Backend unchanged: leave the devices in place so the second display
        //  does not go through a needless teardown. The caller still re-binds
        //  the share device on the existing devices.
        //
        if (!m_devices.empty() && currentNative == targetNative)
        {
            return false;
        }

        //
        //  Backend changed (or this is the first build after an empty list):
        //  swap in the new devices and RETIRE the old ones rather than
        //  destroying them here.
        //
        //  Destroying them now would be a use-after-free: closing an open device
        //  deletes its top-level presentation window, and destroying a visible
        //  native window pumps the event loop (the newly uncovered main window
        //  repaints synchronously). That repaint walks the IP graph, whose
        //  DisplayGroupIPNodes still hold pointers to these very devices --
        //  DisplayGroupIPNode::imageDevice() then dereferences freed memory.
        //
        //  So the caller (RvApplication::rebuildDesktopVideoDevices) refreshes
        //  the graph's display groups against the new device pointers first, and
        //  only then calls purgeRetiredDevices().
        //
        m_retiredDevices.insert(m_retiredDevices.end(), m_devices.begin(), m_devices.end());
        m_devices.clear();

        m_devices = DesktopVideoDevice::createDesktopVideoDevices(this, shareDevice, targetNative);

        return true;
    }

    void DesktopVideoModule::purgeRetiredDevices()
    {
        //
        //  Destroy the devices retired by the last rebuildDevices(). close()
        //  frees the Vulkan swapchain, IOSurface ring or GL ScreenView before
        //  the device is destroyed, mirroring the normal exit path, so nothing
        //  leaks.
        //
        //  Take a local copy first: close() destroys a native window, which
        //  pumps the event loop and can re-enter this module.
        //
        VideoDevices retired;
        retired.swap(m_retiredDevices);

        for (size_t i = 0; i < retired.size(); ++i)
        {
            if (retired[i]->isOpen())
            {
                retired[i]->close();
            }
            delete retired[i];
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
