//
// Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <RvCommon/CGDesktopVideoDeviceArm.h>
#include <OpenGL/OpenGL.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <QuartzCore/CVDisplayLink.h>
#include <IPCore/Session.h>

// NOTE: This file is not used yet and is WIP but should be part
// of the codebase. It's meant to be a resource to implement screen
// resolution enumeration and switching.

namespace Rv
{
    using namespace std;
    using namespace TwkGLF;
    using namespace TwkApp;
    using namespace boost;

    CGDesktopVideoDeviceArm::CGDesktopVideoDeviceArm(VideoModule* m, const CGDesktopVideoDeviceArm::Display& display,
                                                     const QTGLVideoDevice* share)
        : DesktopVideoDevice(m, display.name, share)
        , m_display(display)
        , m_savedMode(nullptr)
        , m_savedScreen(0)
        , m_view(nullptr) // create a QOpenGLWidget with no parent (fullscreen
                          // window)
    {
        createFormats();
    }

    void CGDesktopVideoDeviceArm::createFormats()
    {
        m_videoFormats.clear();
        m_videoFormatIndex = -1;

        int32_t currentIoModeId = getCurrentIoModeId(m_display.cgScreen);

        for (size_t i = 0; i < m_display.modes.size(); i++)
        {
            CGDesktopVideoDeviceArm::Mode& mode = m_display.modes[i];

            if (mode.ioFlags & kDisplayModeSafetyFlags)
            {
                m_videoFormats.push_back(
                    DesktopVideoFormat(mode.width, mode.height, 1.0, 1.0, mode.refreshRate, mode.description, (void*)(&mode)));
            }
            else
            {
                std::cerr << "DesktopVideoDevice: Screen " << m_display.cgScreen << " Mode " << mode.ioModeId
                          << " does not have safety flags" << std::endl;
            }
        }

        addDefaultDataFormats();
        sortVideoFormatsByWidth();

        // Finally, assign the videoFormatIndex to the modes fo the display
        // after it is sorted.
        for (size_t i = 0; i < m_videoFormats.size(); i++)
        {
            const DesktopVideoFormat& format = m_videoFormats[i];

            CGDesktopVideoDeviceArm::Mode& mode = *static_cast<CGDesktopVideoDeviceArm::Mode*>(format.data);
            mode.videoFormatIndex = i;

            if (mode.ioModeId == currentIoModeId)
                m_videoFormatIndex = i;
        }

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

    void CGDesktopVideoDeviceArm::makeCurrent() const { m_viewDevice->makeCurrent(); }

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

    bool CGDesktopVideoDeviceArm::isOpen() const
    {
        return m_viewDevice != nullptr;
        // could also use m_view.isVisible()
    }

    void CGDesktopVideoDeviceArm::open(const StringVector& args)
    {
        if (m_videoFormatIndex < 0)
            return;

        QRect bounds;
        const DesktopVideoFormat& format = m_videoFormats[m_videoFormatIndex];

        // Find the mode from the format's data, and switch the screen to that
        // video mode.
        const CGDesktopVideoDeviceArm::Mode& mode = *static_cast<const CGDesktopVideoDeviceArm::Mode*>(format.data);

        switchToMode(mode, bounds);

        m_view.setGeometry(bounds); // this will place the window on the correct screen.
        m_view.setWindowState(Qt::WindowFullScreen);

        // module()
        TwkApp::VideoModule* m = const_cast<TwkApp::VideoModule*>(module());

        m_viewDevice = new QTGLVideoDevice(m, "AppleSilicon Presentation Device", &m_view);
        m_viewDevice->makeCurrent();

        m_view.showFullScreen();
        /*
                // Next match the bounds of the screen to the appropriate
           QScreen QList<QScreen*> screens = QGuiApplication::screens(); for
           (QScreen* screen : screens)
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

                //        m_viewDevice = this;

                // Old code
        */
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

    void CGDesktopVideoDeviceArm::close()
    {
        ScopedLock lock(m_mutex);

        //  unbind() should be called by the renderer::setOutputVideoDevice, but
        //  just in case, call here too.
        //
        unbind();

        // Hide the QT Window, and change the resolution back to the original
        // mode.
        m_view.setWindowState(Qt::WindowNoState);
        m_view.hide();

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

        delete m_viewDevice;
        m_viewDevice = nullptr;
    }

    VideoDevice::Resolution CGDesktopVideoDeviceArm::resolution() const { return format(); }

    size_t CGDesktopVideoDeviceArm::width() const { return resolution().width; }

    size_t CGDesktopVideoDeviceArm::height() const { return resolution().height; }

    float CGDesktopVideoDeviceArm::pixelScale() const { return resolution().pixelScale; }

    VideoDevice::VideoFormat CGDesktopVideoDeviceArm::format() const
    {
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(m_display.cgScreen);
        int32_t ioModeId = CGDisplayModeGetIODisplayModeID(mode);

        for (int i = 0; i < m_display.modes.size(); i++)
        {
            const CGDesktopVideoDeviceArm::Mode& mode = m_display.modes[i];
            if (mode.ioModeId == ioModeId)
            {
                if (IPCore::debugPlayback)
                {
                    cout << "WARNING: recovered from unexpected video mode " << endl;
                }

                return m_videoFormats[mode.videoFormatIndex];
            }
        }

        cout << "ERROR: unexpected video mode" << endl;
        return VideoFormat();
    }

    VideoDevice::Offset CGDesktopVideoDeviceArm::offset() const { return Offset(0, 0); }

    VideoDevice::Timing CGDesktopVideoDeviceArm::timing() const { return format(); }

    TwkApp::VideoDevice::ColorProfile CGDesktopVideoDeviceArm::colorProfile() const
    {
        //
        //  Get the display's color sync profile
        //

        if (ColorSyncProfileRef iccRef = ColorSyncProfileCreateWithDisplayID(m_display.cgScreen))
        {
            m_colorProfile.type = ICCProfile;

            CFStringRef desc = ColorSyncProfileCopyDescriptionString(iccRef);
            CFIndex n = CFStringGetLength(desc);
            vector<char> buffer(n * 4 + 1);
            CFStringGetCString(desc, &buffer.front(), buffer.size(), kCFStringEncodingUTF8);
            m_colorProfile.description = &buffer.front();

            CFURLRef url = ColorSyncProfileGetURL(iccRef, NULL);
            CFStringRef urlstr = CFURLGetString(url);
            buffer.resize(CFStringGetLength(urlstr) * 4 + 1);
            CFStringGetCString(urlstr, &buffer.front(), buffer.size(), kCFStringEncodingUTF8);

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
    std::vector<CGDesktopVideoDeviceArm::Display> CGDesktopVideoDeviceArm::getDisplays()
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

    int32_t CGDesktopVideoDeviceArm::getCurrentIoModeId(CGDirectDisplayID cgScreen)
    {
        CGDisplayModeRef currentModeRef = CGDisplayCopyDisplayMode(cgScreen);

        int32_t currentIoModeId = CGDisplayModeGetIODisplayModeID(currentModeRef);

        CGDisplayModeRelease(currentModeRef);

        return currentIoModeId;
    }

    CGDisplayModeRef CGDesktopVideoDeviceArm::displayModeRefFromIoModeId(CGDirectDisplayID cgScreen, int32_t ioModeId)
    {
        CFArrayRef array = CGDisplayCopyAllDisplayModes(cgScreen, NULL);

        for (CFIndex i = 0, n = CFArrayGetCount(array); i < n; i++)
        {
            const CGDisplayModeRef cgModeRef = (CGDisplayModeRef)CFArrayGetValueAtIndex(array, i);
            const int32_t id = CGDisplayModeGetIODisplayModeID(cgModeRef);

            if (id == ioModeId)
            {
                CFRetain(cgModeRef);
                return cgModeRef;
            }
        }
        CFRelease(array);

        // It turns out it's actually possible on mac (when you have a scaled
        // display) that the ioModeId is not included in
        // CGDisplayCopyAllDisplayModes.

        // if all else fails, there's just no display mode.
        return CGDisplayModeRef(0);
    }

    bool CGDesktopVideoDeviceArm::switchToMode(const CGDesktopVideoDeviceArm::Mode& mode, QRect& bounds)
    {
        // Set the new display mode.
        CGDisplayModeRef newDisplayModeRef = displayModeRefFromIoModeId(mode.cgScreen, mode.ioModeId);
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

        QRect qtBounds(cgBounds.origin.x, cgBounds.origin.y, cgBounds.size.width, cgBounds.size.height);

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

    CGDesktopVideoDeviceArm::Mode CGDesktopVideoDeviceArm::constructModeFromDisplayModeRef(CGDirectDisplayID cgScreen,
                                                                                           CGDisplayModeRef modeRef)
    {
        CGDesktopVideoDeviceArm::Mode dm;

        dm.ioModeId = (int32_t)CGDisplayModeGetIODisplayModeID(modeRef);
        dm.ioFlags = (uint32_t)CGDisplayModeGetIOFlags(modeRef);
        dm.width = (int32_t)CGDisplayModeGetWidth(modeRef);
        dm.height = (int32_t)CGDisplayModeGetHeight(modeRef);
        dm.refreshRate = CGDisplayModeGetRefreshRate(modeRef);
        dm.isGUI = CGDisplayModeIsUsableForDesktopGUI(modeRef);
        dm.cgScreen = cgScreen;

        // Get the description
        std::ostringstream ss;
        // ss << "[id:" << dm.cgScreen << "] ";
        // ss << "(ioMode:" << dm.ioMode << ") ";
        ss << dm.width << "x" << dm.height << " ";
        ss << "@" << dm.refreshRate << " hz";
        // ss << (dm.isGUI ? " (gui)" : "");
        ss << ((dm.ioFlags & kDisplayModeStretchedFlag) ? " (stretched)" : "");

        // in the old code we could check pixel color depth. But
        // thesemethods are now deprecated.
        // if (depth > 8)
        //    str << " " << depth << " bits/channel";

        dm.description = ss.str();

        return dm;
    }

    std::vector<CGDesktopVideoDeviceArm::Mode> CGDesktopVideoDeviceArm::getDisplayModes(CGDirectDisplayID cgScreen)
    {
        std::vector<CGDesktopVideoDeviceArm::Mode> dms;

        CGDisplayModeRef currentModeRef = CGDisplayCopyDisplayMode(cgScreen);
        int32_t currentIoModeId = CGDisplayModeGetIODisplayModeID(currentModeRef);
        bool foundCurrentModeId = false;

        CFArrayRef modes = CGDisplayCopyAllDisplayModes(cgScreen, nullptr);
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
            CGDisplayModeRef modeRef = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

            CGDesktopVideoDeviceArm::Mode dm = constructModeFromDisplayModeRef(cgScreen, modeRef);

            dms.push_back(dm);

            if (currentIoModeId == dm.ioModeId)
            {
                foundCurrentModeId = true;
            }
        }

        // Did we find the current mode from the screen? It's possible
        // iwe did not, if macOS uses a scaled display mode.
        if (foundCurrentModeId == false)
        {
            CGDesktopVideoDeviceArm::Mode dm = constructModeFromDisplayModeRef(cgScreen, currentModeRef);
            dms.push_back(dm);
        }

        CGDisplayModeRelease(currentModeRef);

        CFRelease(modes);

        return dms;
    }

    std::vector<VideoDevice*> CGDesktopVideoDeviceArm::createDesktopVideoDevices(TwkApp::VideoModule* module,
                                                                                 const QTGLVideoDevice* shareDevice)
    {
        std::vector<VideoDevice*> devices;

        auto cgDisplays = getDisplays();

        for (size_t d = 0; d < cgDisplays.size(); d++)
        {
            auto display = cgDisplays[d];

            CGDesktopVideoDeviceArm* sd = new CGDesktopVideoDeviceArm(module, display, shareDevice);
            devices.push_back(sd);
        }

        return devices;
    }

} // namespace Rv
