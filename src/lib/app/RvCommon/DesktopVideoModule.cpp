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
#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/graphics/IOFramebufferShared.h>
#include <IOKit/graphics/IOGraphicsInterface.h>
#endif

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QApplication>

namespace Rv
{
    using namespace std;
    using namespace boost;

    //----------------------------------------------------------------------

#ifdef PLATFORM_DARWIN
    static void KeyArrayCallback(const void* key, const void* value,
                                 void* context)
    {
        CFMutableArrayRef langKeys = (CFMutableArrayRef)context;
        CFArrayAppendValue(langKeys, key);
    }
#endif

    DesktopVideoModule::DesktopVideoModule(NativeDisplayPtr np,
                                           QTGLVideoDevice* shareDevice)
        : VideoModule()
    {
        bool useQt = true;
        QDesktopWidget* desktop = QApplication::desktop();
        int nscreens = desktop->screenCount();

#ifdef PLATFORM_DARWIN
        useQt = false;

        //
        //  Use the CoreGraphics devices
        //

        CGDirectDisplayID idArray[20];
        uint32_t idNum;
        CGGetOnlineDisplayList(20, idArray, &idNum);
        // CGDirectDisplayID mainID = CGMainDisplayID();

        for (size_t i = 0; i < idNum; i++)
        {
            CGDirectDisplayID aID = idArray[i];

            CFStringRef localName = NULL;
            io_connect_t displayPort = CGDisplayIOServicePort(aID);
            CFDictionaryRef dict =
                (CFDictionaryRef)IODisplayCreateInfoDictionary(
                    displayPort, kIODisplayOnlyPreferredName);
            CFDictionaryRef names = (CFDictionaryRef)CFDictionaryGetValue(
                dict, CFSTR(kDisplayProductName));

            if (names)
            {
                CFArrayRef langKeys = CFArrayCreateMutable(
                    kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
                CFDictionaryApplyFunction(names, KeyArrayCallback,
                                          (void*)langKeys);
                CFArrayRef orderLangKeys =
                    CFBundleCopyPreferredLocalizationsFromArray(langKeys);
                CFRelease(langKeys);

                if (orderLangKeys && CFArrayGetCount(orderLangKeys))
                {
                    CFStringRef langKey =
                        (CFStringRef)CFArrayGetValueAtIndex(orderLangKeys, 0);
                    localName =
                        (CFStringRef)CFDictionaryGetValue(names, langKey);
                    CFRetain(localName);
                }

                CFRelease(orderLangKeys);
            }

            //
            //  Above process seems to fail to find the display name on some mac
            //  minis, so invent a name if we didn't find one.
            //

            ostringstream nameStr;
            const char* nameP = 0;
            vector<char> localBuffer;

            if (localName == NULL)
            {
                nameStr << "CG Display " << (i + 1);
                nameP = nameStr.str().c_str();
            }
            else
            {
                localBuffer =
                    vector<char>(CFStringGetLength(localName) * 4 + 1);

                CFStringGetCString(localName, &localBuffer.front(),
                                   localBuffer.size(), kCFStringEncodingUTF8);

                nameP = &localBuffer.front();
            }

            //
            //  In use of 'i' below, we're relying on the order of the devices
            //  here being the same as in the objc [NSScreen screens] call, but
            //  that
            //  seems to be true.  The [NSScreen screens] call is used in Qt
            //  core\ code to define Qt screens.
            //
            CGDesktopVideoDevice* sd =
                new CGDesktopVideoDevice(this, nameP, aID, i, shareDevice);
            m_devices.push_back(sd);

            CFRelease(dict);
        }
#endif

        if (useQt)
        {
            for (int screen = 0; screen < desktop->screenCount(); screen++)
            {
                QWidget* w = desktop->screen(screen);
                ostringstream name;
                name << w->screen()->manufacturer().toUtf8().data();
                name << " " << w->screen()->model().toUtf8().data();
                name << " " << w->screen()->name().toUtf8().data();
                QTDesktopVideoDevice* sd = new QTDesktopVideoDevice(
                    this, name.str(), screen, shareDevice);
                m_devices.push_back(sd);
            }
        }
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

        int screen = QApplication::desktop()->screenNumber(QPoint(x, y));

        for (int i = 0; i < m_devices.size(); ++i)
        {
            //
            //  These devices may be NVDesktopVideoDevices or
            //  QTDeskTopVideoDevices.
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

#endif

        return device;
    }

} // namespace Rv
