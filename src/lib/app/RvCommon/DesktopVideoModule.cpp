//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/DesktopVideoModule.h>
#include <RvCommon/QTDesktopVideoDevice.h>
#include <RvCommon/GLView.h>
#include <IPCore/ImageRenderer.h>
#include <stl_ext/string_algo.h>
#include <QtGui/QtGui>
#include <map>
#include <boost/algorithm/string.hpp>

#ifdef PLATFORM_DARWIN
#include <RvCommon/CGDesktopVideoDevice.h>
#include <RvCommon/CGDesktopVideoDeviceArm.h>
#endif

#include <QtWidgets/QApplication>
#include <QScreen>

namespace Rv
{
    using namespace std;
    using namespace boost;

    //----------------------------------------------------------------------

    /*
        static bool isDarwinArm() {
            #ifdef __aarch64__
                return true;
            #elif defined(__x86_64__)
                // Running in Rosetta 2?
                static int ret = -1;
                if (ret < 0)
                {
                    size_t size = sizeof(ret);
                    if (sysctlbyname("sysctl.proc_translated", &ret, &size,
       nullptr, 0) == -1) { ret = 0;
                    }
                }
                return ret > 0;
            #else
                return false;
            #endif
        }
    */

    // force intel code path for this commit
    static bool isDarwinArm() { return true; }

    static bool isDarwinIntel() { return true; }

    static bool useQtOnDarwinArm() { return true; }

    DesktopVideoModule::DesktopVideoModule(NativeDisplayPtr np,
                                           QTGLVideoDevice* shareDevice)
        : VideoModule()
    {

#ifdef PLATFORM_DARWIN
        if (isDarwinArm()) // Apple Arm
        {
            if (useQtOnDarwinArm())
            {
                m_devices = QTDesktopVideoDevice::createDesktopVideoDevices(
                    this, shareDevice);
            }
            else
            {
                m_devices = CGDesktopVideoDeviceArm::createDesktopVideoDevices(
                    this, shareDevice);
            }
        }
        else // Apple intel
        {
            m_devices = CGDesktopVideoDevice::createDesktopVideoDevices(
                this, shareDevice);
        }
#else
        // On Windows and Linux, use the QT Path always.
        m_devices =
            QtDesktopVideoDevice::createDesktopVideoDevices(this, shareDevice);
#endif
    }

    DesktopVideoModule::~DesktopVideoModule() {}

    string DesktopVideoModule::name() const { return "Desktop"; }

    void DesktopVideoModule::open() {}

    void DesktopVideoModule::close() {}

    bool DesktopVideoModule::isOpen() const { return true; }

    TwkApp::VideoDevice* DesktopVideoModule::deviceFromPosition(int x,
                                                                int y) const
    {
        TwkApp::VideoDevice* device = 0;

#if defined(PLATFORM_DARWIN)

        const int maxDisplays = 64;
        CGDirectDisplayID displays[maxDisplays];
        CGDisplayCount displayCount;
        const CGPoint cgPoint = CGPointMake(x, y);
        const CGDisplayErr err = CGGetDisplaysWithPoint(
            cgPoint, maxDisplays, displays, &displayCount);

        if (err != kCGErrorSuccess)
            cerr << "ERROR: CGGetDisplaysWithPoint returns " << err << endl;
        else if (displayCount > 0)
        {
            for (int i = 0; i < m_devices.size(); ++i)
            {
                CGDesktopVideoDevice* d =
                    dynamic_cast<CGDesktopVideoDevice*>(m_devices[i]);

                if (d && d->cgDisplay() == displays[0])
                {
                    device = d;
                    break;
                }
            }
        }

#else // PLATFORM_LINUX or PLATFORM_WINDOWS

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
                    //  QTDeskTopVideoDevices.
                    //
                    TwkApp::VideoDevice* d = m_devices[i];

                    if (DesktopVideoDevice* dd =
                            dynamic_cast<DesktopVideoDevice*>(d))
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
#endif

        return device;
    }

} // namespace Rv
