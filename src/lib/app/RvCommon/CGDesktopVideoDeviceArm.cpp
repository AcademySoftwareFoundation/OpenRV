//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/CGDesktopVideoDeviceArm.h>
#include <OpenGL/OpenGL.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <QuartzCore/CVDisplayLink.h>
#include <IPCore/Session.h>

namespace Rv
{
    using namespace std;
    using namespace TwkGLF;
    using namespace TwkApp;
    using namespace boost;

    CGDesktopVideoDeviceArm::CGDesktopVideoDeviceArm(
        VideoModule* m, const CGDesktopVideoDeviceArm::Display& display,
        const QTGLVideoDevice* share)
        : DesktopVideoDevice(m, display.name, share)
        , m_display(display)
        , m_savedMode(nullptr)
        , m_savedScreen(0)
    {
        createFormats();
    }

    void CGDesktopVideoDeviceArm::createFormats()
    {
        m_videoFormats.clear();
        m_videoFormatIndex = -1;

        for (size_t i = 0; i < m_display.modes.size(); i++)
        {
            const CGDesktopVideoDeviceArm::Mode& mode = m_display.modes[i];

            if (mode.ioFlags & kDisplayModeSafetyFlags)
            {
                m_videoFormats.push_back(DesktopVideoFormat(
                    mode.width, mode.height, 1.0, 1.0, mode.refreshRate,
                    mode.description, (void*)(size_t)mode.ioModeId));
            }
        }

        addDefaultDataFormats();
        sortVideoFormatsByWidth();

        //
        //  Locate the current mode index and create the IOModeMap going
        //  from IOKit display mode to video format index
        //

#ifdef __PB
        int32_t currentIoModeId = getCurrentIoModeId(m_display.cgScreen);

        for (size_t i = 0; i < m_videoFormats.size(); i++)
        {
            int32_t mid = int32_t((size_t)m_videoFormats[i].data);
            if (mid == currentIoModeId)
                m_videoFormatIndex = i;
            m_ioModeMap[mid] = i;
        }

        if (m_videoFormatIndex == -1)
        {
            //
            //  We got here because somehow we were unable to find the
            //  current mode. This doesn't seem to happen anymore but just
            //  in case the code is still active.
            //

            if (IPCore::debugPlayback)
                cout << "INFO: found new video modes" << endl;

            int w = CGDisplayModeGetWidth(currentMode);
            int h = CGDisplayModeGetHeight(currentMode);
            ostringstream str;
            str << w << " x " << h;
            str.precision(4);
            str << " " << cvRefHz << "Hz ";

            m_ioModeMap[currentModeID] = m_videoFormats.size();

            m_videoFormats.push_back(
                DesktopVideoFormat(w, h, 1.0, 1.0, cvRefHz, str.str(),
                                   (void*)(size_t)currentModeID));

            //
            //  Then re-sort and re-determine index.
            //

            sortVideoFormatsByWidth();

            for (size_t i = 0; i < m_videoFormats.size(); i++)
            {
                int32_t mid = int32_t((size_t)m_videoFormats[i].data);
                if (mid == currentModeID)
                    m_videoFormatIndex = i;
            }
        }
        CFRelease(currentMode);
#endif

#if 0
    for (size_t i = 0; i < m_videoFormats.size(); i++)
    {
        const DesktopVideoFormat& f = m_videoFormats[i];

        cout << "INFO: format: " << i << " = "
             << f.width << " x " << f.height
             << " " << f.hz
             << " ID=" << (int32_t(size_t(f.data)))
             << endl;
    }
#endif
    }

    CGDesktopVideoDeviceArm::~CGDesktopVideoDeviceArm() { close(); }

    void CGDesktopVideoDeviceArm::makeCurrent() const
    {
        m_viewDevice->makeCurrent();
    }

    bool CGDesktopVideoDeviceArm::isSyncing() const
    {
        ScopedLock lock(m_syncTestMutex);
        return m_syncing;
    }

    void CGDesktopVideoDeviceArm::syncBuffers() const
    {
        ScopedLock lock(m_mutex);

        {
            ScopedLock lock(m_syncTestMutex);
            m_syncing = true;
        }

        makeCurrent();

        //
        //   Docs for CGFlushDrawable say it includes a glFlush and user should
        //   "should not call glFlush() prior", so don't call it here.
        //
        //   ARG!  This breaks presentation mode.  putting it back in.
        //

        glFlush();
#ifdef __PB
        CGLFlushDrawable(m_context);
#endif
        {
            ScopedLock lock(m_syncTestMutex);
            m_syncing = false;
        }
    }

    void CGDesktopVideoDeviceArm::open(const StringVector& args)
    {
        const DesktopVideoFormat& format = m_videoFormats[m_videoFormatIndex];

        QRect bounds;
#ifdef __PB
        switchToMode(mode, &bounds);
#endif
        // Next match the bounds of the screen to the appropriate QScreen
        QList<QScreen*> screens = QGuiApplication::screens();
        for (QScreen* screen : screens)
        {
            if (screen->geometry() == bounds)
            {
                m_glWindow.setScreen(screen);
                break;
            }
        }

        m_glWindow.setGeometry(bounds);
        m_glWindow.setVisible(true);
        m_glWindow.setWindowState(Qt::WindowFullScreen);
        //  setFlags(Qt::Window | Qt::CustomizeWindowHint |
        //  Qt::FramelessWindowHint);

        m_glWindow.showFullScreen();

        // Old code

        /*

        const DesktopVideoFormat& format = m_videoFormats[m_videoFormatIndex];

        shareDevice()->makeCurrent();
        CGLContextObj shareContext = CGLGetCurrentContext();

        m_savedMode = CGDisplayCopyDisplayMode(m_cgScreen);

        m_viewDevice = new CGGLVideoDevice(0, format.width, format.height,
                                           false, m_context, false);
        m_viewDevice->makeCurrent();
        */
    }

    bool CGDesktopVideoDeviceArm::isOpen() const
    {
        return m_glWindow.isVisible();
    }

    void CGDesktopVideoDeviceArm::close()
    {
        ScopedLock lock(m_mutex);

        //  unbind() should be called by the renderer::setOutputVideoDevice, but
        //  just in case, call here too.
        //
        if (m_savedMode)
        {
            CGDisplaySetDisplayMode(m_savedScreen, m_savedMode, nullptr);

            CGDisplayModeRelease(m_savedMode);
            m_savedMode = nullptr;
            m_savedScreen = 0;
        }

        m_glWindow.setVisible(false);

#ifdef __PB
        unbind();
#endif

        if (m_savedMode)
        {
            CGDisplaySetDisplayMode(m_savedScreen, m_savedMode, NULL);
            CGDisplayModeRelease(m_savedMode);
            m_savedMode = nullptr;
        }

#ifdef __PB
        delete m_viewDevice;
        m_viewDevice = 0;
#endif
    }

    VideoDevice::Resolution CGDesktopVideoDeviceArm::resolution() const
    {
        return format();
    }

    size_t CGDesktopVideoDeviceArm::width() const { return resolution().width; }

    size_t CGDesktopVideoDeviceArm::height() const
    {
        return resolution().height;
    }

    float CGDesktopVideoDeviceArm::pixelScale() const
    {
        return resolution().pixelScale;
    }

    VideoDevice::VideoFormat CGDesktopVideoDeviceArm::format() const
    {
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(m_display.cgScreen);
        int32_t ioModeId = CGDisplayModeGetIODisplayModeID(mode);

        // get the ioMode from the display modes.
        // modernize using lambda

        return VideoFormat();
#ifdef __PB
        for (int i = 0; i < m_display.modes.size(); i++)
        {
            if (m_display.modes[i].ioModeId == ioModeId)
                return m_videoFormats[i];
        }

        IOModeMap::const_iterator i = m_ioModeMap.find(id);

        if (i != m_ioModeMap.end())
        {
            return m_videoFormats[i->second];
        }
        else
        {
            const_cast<CGDesktopVideoDevice*>(this)->createFormats();
            i = m_ioModeMap.find(id);

            if (i == m_ioModeMap.end())
            {
                cout << "ERROR: unexpected video mode" << endl;
                return VideoFormat();
            }
            else
            {
                if (IPCore::debugPlayback)
                {
                    cout << "WARNING: recovered from unexpected video mode "
                         << endl;
                }
                return m_videoFormats[i->second];
            }
        }
#endif
    }

    VideoDevice::Offset CGDesktopVideoDeviceArm::offset() const
    {
        return Offset(0, 0);
    }

    VideoDevice::Timing CGDesktopVideoDeviceArm::timing() const
    {
        return format();
    }

    TwkApp::VideoDevice::ColorProfile
    CGDesktopVideoDeviceArm::colorProfile() const
    {
        //
        //  Get the display's color sync profile
        //

        if (ColorSyncProfileRef iccRef =
                ColorSyncProfileCreateWithDisplayID(m_display.cgScreen))
        {
            m_colorProfile.type = ICCProfile;

            CFStringRef desc = ColorSyncProfileCopyDescriptionString(iccRef);
            CFIndex n = CFStringGetLength(desc);
            vector<char> buffer(n * 4 + 1);
            CFStringGetCString(desc, &buffer.front(), buffer.size(),
                               kCFStringEncodingUTF8);
            m_colorProfile.description = &buffer.front();

            CFURLRef url = ColorSyncProfileGetURL(iccRef, NULL);
            CFStringRef urlstr = CFURLGetString(url);
            buffer.resize(CFStringGetLength(urlstr) * 4 + 1);
            CFStringGetCString(urlstr, &buffer.front(), buffer.size(),
                               kCFStringEncodingUTF8);

            m_colorProfile.url = &buffer.front();
        }
        else
        {
            m_colorProfile = ColorProfile();
        }

        return m_colorProfile;
    }

    //
    // Static helper functions
    //
    std::vector<CGDesktopVideoDeviceArm::Display>
    CGDesktopVideoDeviceArm::getDisplays()
    {
        uint32_t count = 0;
        CGDirectDisplayID ids[64] = {0};
        CGGetOnlineDisplayList(64, ids, &count);

        if (count <= 1)
        {
            std::cout << "ERROR: No external monitor detected\n"
                         "Please connect an external monitor either via USB-C "
                         "or HDMI to reproduce this issue.\n";
        }

        std::vector<CGDesktopVideoDeviceArm::Display> displays;
        for (uint32_t i = 0; i < count; i++)
        {
            CGDesktopVideoDeviceArm::Display display;
            display.cgScreen = ids[i];
            display.name = std::string("Display ") + std::to_string(i);
            display.modes = getDisplayModes(display.cgScreen);
            display.currentIoModeId = getCurrentIoModeId(display.cgScreen);

            displays.push_back(display);
        }

        return displays;
    }

    int32_t
    CGDesktopVideoDeviceArm::getCurrentIoModeId(CGDirectDisplayID cgScreen)
    {
        CGDisplayModeRef currentModeRef = CGDisplayCopyDisplayMode(cgScreen);

        int32_t currentIoModeId =
            CGDisplayModeGetIODisplayModeID(currentModeRef);

        CGDisplayModeRelease(currentModeRef);

        return currentIoModeId;
    }

    CGDisplayModeRef
    CGDesktopVideoDeviceArm::displayModeRefFromIoModeId(Mode mode)
    {
        CFArrayRef array = CGDisplayCopyAllDisplayModes(mode.cgScreen, NULL);

        for (CFIndex i = 0, n = CFArrayGetCount(array); i < n; i++)
        {
            const CGDisplayModeRef cgModeRef =
                (CGDisplayModeRef)CFArrayGetValueAtIndex(array, i);
            const int32_t id = CGDisplayModeGetIODisplayModeID(cgModeRef);

            if (id == mode.ioModeId)
            {
                CFRetain(cgModeRef);
                return cgModeRef;
            }
        }

        CFRelease(array);

        return CGDisplayModeRef(0);
    }

    bool
    CGDesktopVideoDeviceArm::switchToMode(CGDesktopVideoDeviceArm::Mode mode,
                                          QRect& bounds)
    {
        CGDirectDisplayID cgScreen = mode.cgScreen;

        // Set the new display mode.
        CGDisplayModeRef newDisplayModeRef = displayModeRefFromIoModeId(mode);
        if (!newDisplayModeRef)
        {
            std::cerr << "Failed to get display modes!" << std::endl;
            return false;
        }

        CGDisplaySetDisplayMode(mode.cgScreen, newDisplayModeRef, nullptr);

        bounds = cgScreenBounds(mode.cgScreen);

        CGDisplayModeRelease(newDisplayModeRef);

        return true;
    }

    QRect CGDesktopVideoDeviceArm::cgScreenBounds(CGDirectDisplayID cgScreen)
    {
        CGRect cgBounds = CGDisplayBounds(cgScreen);

        QRect qtBounds(cgBounds.origin.x, cgBounds.origin.y,
                       cgBounds.size.width, cgBounds.size.height);

        return qtBounds;
    }

    int32_t CGDesktopVideoDeviceArm::qtScreenFromCG(CGDirectDisplayID cgScreen)
    {
        QRect cgBounds = cgScreenBounds(cgScreen);

        QList<QScreen*> screens = QGuiApplication::screens();
        for (int32_t i = 0; i < screens.count(); i++)
        {
            const QScreen* screen = screens[i];
            QRect qtBounds = screen->geometry();

            if (qtBounds == cgBounds)
                return i;
        }

        return -1;
    }

    std::vector<CGDesktopVideoDeviceArm::Mode>
    CGDesktopVideoDeviceArm::getDisplayModes(CGDirectDisplayID screen)
    {
        std::vector<CGDesktopVideoDeviceArm::Mode> dms;

        CFArrayRef modes = CGDisplayCopyAllDisplayModes(screen, nullptr);
        if (!modes)
        {
            std::cerr << "Failed to get display modes!" << std::endl;
            return dms;
        }

        // Iterate through each display mode in the array
        CFIndex count = CFArrayGetCount(modes);
        int index = 0;
        for (CFIndex i = 0; i < count; i++, index++)
        {
            CGDisplayModeRef mode =
                (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

            Mode dm;

            dm.ioModeId = (int32_t)CGDisplayModeGetIODisplayModeID(mode);
            dm.ioFlags = (uint32_t)CGDisplayModeGetIOFlags(
                mode); // this... is... not necessary?
            dm.width = (int32_t)CGDisplayModeGetWidth(mode);
            dm.height = (int32_t)CGDisplayModeGetHeight(mode);
            dm.refreshRate = CGDisplayModeGetRefreshRate(mode);
            dm.isGUI = CGDisplayModeIsUsableForDesktopGUI(mode);
            dm.cgScreen = screen;

            // Get the description
            std::ostringstream ss;
            //                    ss << "[id:" << dm.cgScreen << "] ";
            //                    ss << "(ioMode:" << dm.ioMode << ") ";
            ss << dm.width << "x" << dm.height << " ";
            ss << "@" << dm.refreshRate << " hz";
            //                    ss << (dm.isGUI ? " (gui)" : "");
            ss << ((dm.ioFlags & kDisplayModeStretchedFlag) ? " (stretched)"
                                                            : "");

            // in the old code we could check pixel color depth. But
            // thesemethods are now deprecated. if (depth > 8)
            //    str << " " << depth << " bits/channel";

            dm.description = ss.str();

            dms.push_back(dm);
        }

        CFRelease(modes);

        return dms;
    }

    std::vector<VideoDevice*>
    CGDesktopVideoDeviceArm::createDesktopVideoDevices(
        TwkApp::VideoModule* module, const QTGLVideoDevice* shareDevice)
    {
        std::vector<VideoDevice*> devices;

        auto cgDisplays = getDisplays();

        for (size_t d = 0; d < cgDisplays.size(); d++)
        {
            auto display = cgDisplays[d];

            CGDesktopVideoDeviceArm* sd =
                new CGDesktopVideoDeviceArm(module, display, shareDevice);
            devices.push_back(sd);
        }

        return devices;
    }

} // namespace Rv
