//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/CGDesktopVideoDevice.h>
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

    namespace
    {

        string tostring(CFStringRef s)
        {
            vector<char> str(CFStringGetLength(s) * 4 + 1);
            CFStringGetCString(s, &str.front(), str.size(),
                               kCFStringEncodingUTF8);
            return string(&str.front());
        }

        int depthFromPixelEncoding(CFStringRef pixEnc)
        {
            int depth = 0;

            if (CFStringCompare(pixEnc, CFSTR(IO32BitDirectPixels),
                                kCFCompareCaseInsensitive)
                == kCFCompareEqualTo)
                depth = 8;
            else if (CFStringCompare(pixEnc, CFSTR(IO16BitDirectPixels),
                                     kCFCompareCaseInsensitive)
                     == kCFCompareEqualTo)
                depth = 5;
            else if (CFStringCompare(pixEnc, CFSTR(IO8BitIndexedPixels),
                                     kCFCompareCaseInsensitive)
                     == kCFCompareEqualTo)
                depth = 2;
            else if (CFStringCompare(pixEnc, CFSTR(kIO30BitDirectPixels),
                                     kCFCompareCaseInsensitive)
                     == kCFCompareEqualTo)
                depth = 10;

            CFRelease(pixEnc);
            return depth;
        }

        double modeHz(CGDisplayModeRef mode, CFMutableArrayRef ioModesArray,
                      int ioModeCount, double defaultHz)
        {
            //
            //  CGDisplayModeGetRefreshRate can return 0.0 which is not
            //  helpful.  So we look up the same mode in IOKit's mode list to
            //  get the a better rate number if it exists.
            //

            const int32_t modeID = CGDisplayModeGetIODisplayModeID(mode);
            const size_t width = CGDisplayModeGetWidth(mode);
            const size_t height = CGDisplayModeGetHeight(mode);
            double hz = CGDisplayModeGetRefreshRate(mode);

            for (CFIndex i = 0; i < ioModeCount; ++i)
            {
                if (CFMutableDictionaryRef dict =
                        (CFMutableDictionaryRef)CFArrayGetValueAtIndex(
                            ioModesArray, i))
                {
                    CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(
                        dict, CFSTR(kIOFBModeIDKey));
                    int32_t n = 0;
                    CFNumberGetValue(num, kCFNumberSInt32Type, &n);

                    if (n == modeID)
                    {
                        if (const CFDataRef data =
                                (CFDataRef)CFDictionaryGetValue(
                                    dict, CFSTR(kIOFBModeDMKey)))
                        {
                            IODisplayModeInformation* ioModeInfo =
                                (IODisplayModeInformation*)CFDataGetBytePtr(
                                    data);

                            if (ioModeInfo && ioModeInfo->nominalWidth == width
                                && ioModeInfo->nominalHeight == height)
                            {
                                hz = ioModeInfo->refreshRate / 65536.0;
                                break;
                            }
                        }
                    }
                }
            }

            //
            //  If IOKit gives us crap for the mode Hz just assume the
            //  DisplayLink hz is applied to all modes. This seems to happen
            //  when dynamic gpu switching is on in a laptop. In that case it
            //  appears that all the laptop hz are the same anyway so just
            //  make that assumption. It will be correct at least for the
            //  current mode
            //

            if (hz == 0.0)
                hz = defaultHz;

            return hz;
        }

        CGDisplayModeRef displayModeRefFromIOMode(int32_t ioMode,
                                                  CGDirectDisplayID cgScreen)
        {
            io_service_t ioFB = CGDisplayIOServicePort(cgScreen);
            CFMutableDictionaryRef ioFBConfigDict =
                (CFMutableDictionaryRef)IORegistryEntryCreateCFProperty(
                    ioFB, CFSTR(kIOFBConfigKey), kCFAllocatorDefault,
                    kNilOptions);
            CFArrayRef array = CGDisplayCopyAllDisplayModes(cgScreen, NULL);

            for (CFIndex i = 0, n = CFArrayGetCount(array); i < n; i++)
            {
                const CGDisplayModeRef mode =
                    (CGDisplayModeRef)CFArrayGetValueAtIndex(array, i);
                const int32_t id = CGDisplayModeGetIODisplayModeID(mode);

                if (id == ioMode)
                    return mode;
            }

            return CGDisplayModeRef(0);
        }

    } // namespace

    CGDesktopVideoDevice::CGDesktopVideoDevice(VideoModule* m,
                                               const std::string& name,
                                               CGDirectDisplayID cgScreen,
                                               int qtScreen,
                                               const QTGLVideoDevice* share)
        : DesktopVideoDevice(m, name, share)
        , m_cgScreen(cgScreen)
        , m_qtScreen(qtScreen)
        , m_context(0)
        , m_modes(0)
        , m_savedMode(0)
    {
        createFormats();
    }

    void CGDesktopVideoDevice::createFormats()
    {
        m_videoFormats.clear();
        m_videoFormatIndex = -1;
        m_ioModeMap.clear();

        //
        //  Create a temporary CVDisplayLink and get the hz from
        //  there. This seems to always work even in the case of Dynamic
        //  Switching between GPUs. Usually in that case we just 0.0 for
        //  all the hz. Forcing the GPU to a specific one will result in
        //  correct results.
        //
        //  NOTE: you'd think CVDisplayLinkGetActualOutputVideoRefreshPeriod
        //  wouldn't return 0 but that's what it does rendering it useless.
        //

        CVDisplayLinkRef cvLinkRef = 0;
        CVDisplayLinkCreateWithCGDisplay(m_cgScreen, &cvLinkRef);
        CVTime cvRef =
            CVDisplayLinkGetNominalOutputVideoRefreshPeriod(cvLinkRef);
        double cvRefHz = double(cvRef.timeScale) / double(cvRef.timeValue);
        CFRelease(cvLinkRef);

        //
        //  10.6 SDK or better
        //
        //  NOTE: on machines with 2 GPUs (discreet and integrated) you
        //  can get in a funny situation where we find the modes for the
        //  wrong GPU. Basically the integrated GPU modes are different
        //  than the discreet GPU's. This happens because we haven't
        //  switched to the discreet GPU yet when this is made?
        //

        io_service_t ioFB = CGDisplayIOServicePort(m_cgScreen);
        CFMutableDictionaryRef ioFBConfigDict =
            (CFMutableDictionaryRef)IORegistryEntryCreateCFProperty(
                ioFB, CFSTR(kIOFBConfigKey), kCFAllocatorDefault, kNilOptions);
        CFMutableArrayRef ioModesArray =
            (ioFBConfigDict) ? (CFMutableArrayRef)CFDictionaryGetValue(
                                   ioFBConfigDict, CFSTR(kIOFBModesKey))
                             : 0;
        int ioModeCount = (ioModesArray) ? CFArrayGetCount(ioModesArray) : 0;

        CFArrayRef array = CGDisplayCopyAllDisplayModes(m_cgScreen, NULL);
        CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(m_cgScreen);
        int32_t currentModeID = CGDisplayModeGetIODisplayModeID(currentMode);
        io_connect_t displayPort = CGDisplayIOServicePort(m_cgScreen);
        int uniqueModes = 0;

        m_modes = array;
        CFRetain(array);

        for (CFIndex i = 0, n = CFArrayGetCount(array); i < n; i++)
        {
            const CGDisplayModeRef mode =
                (CGDisplayModeRef)CFArrayGetValueAtIndex(array, i);
            const int32_t id = CGDisplayModeGetIODisplayModeID(mode);
            const size_t flags = CGDisplayModeGetIOFlags(mode);
            const size_t depth =
                depthFromPixelEncoding(CGDisplayModeCopyPixelEncoding(mode));

            if ((flags & kDisplayModeSafetyFlags) && depth >= 8)
            {
                const size_t width = CGDisplayModeGetWidth(mode);
                const size_t height = CGDisplayModeGetHeight(mode);
                const int32_t ioID = CGDisplayModeGetIODisplayModeID(mode);
                const double hz =
                    modeHz(mode, ioModesArray, ioModeCount, cvRefHz);

                ostringstream str;
                str << width << " x " << height;
                str.precision(4);
                if (hz != 0.0)
                    str << " " << hz << "Hz ";
                if (flags & kDisplayModeStretchedFlag)
                    str << " (stretched)";
                if (depth > 8)
                    str << " " << depth << " bits/channel";

                // cerr << m_name << ":  adding mode " << m_videoFormats.size()
                //      << " " << str.str()
                //      << ", (" << id << ")"
                //      << endl;

                m_videoFormats.push_back(
                    DesktopVideoFormat(width, height, 1.0, 1.0, hz, str.str(),
                                       (void*)(size_t)ioID));
            }
        }

        addDefaultDataFormats();
        sortVideoFormatsByWidth();

        //
        //  Locate the current mode index and create the IOModeMap going
        //  from IOKit display mode to video format index
        //

        for (size_t i = 0; i < m_videoFormats.size(); i++)
        {
            int32_t mid = int32_t((size_t)m_videoFormats[i].data);
            if (mid == currentModeID)
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

    CGDesktopVideoDevice::~CGDesktopVideoDevice() { close(); }

    void CGDesktopVideoDevice::makeCurrent() const
    {
        m_viewDevice->makeCurrent();
    }

    bool CGDesktopVideoDevice::isSyncing() const
    {
        ScopedLock lock(m_syncTestMutex);
        return m_syncing;
    }

    void CGDesktopVideoDevice::syncBuffers() const
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

        CGLFlushDrawable(m_context);

        {
            ScopedLock lock(m_syncTestMutex);
            m_syncing = false;
        }
    }

    void CGDesktopVideoDevice::open(const StringVector& args)
    {
        CGDisplayCapture(m_cgScreen);
        const DesktopVideoFormat& format = m_videoFormats[m_videoFormatIndex];

        //
        //  Get the mac CGLContextObj from the shared QTGLVideoDevice
        //  context NOTE: we're using "pre 10.6" method because we want to
        //  capture the display (this lets us change the refresh rate,
        //  etc).
        //

        shareDevice()->makeCurrent();
        CGLContextObj shareContext = CGLGetCurrentContext();

        vector<unsigned long> attrs;

        attrs.push_back(kCGLPFAAccelerated);
        attrs.push_back(kCGLPFADoubleBuffer);
        attrs.push_back(kCGLPFAColorSize);
        attrs.push_back(3 * 8);
        attrs.push_back(kCGLPFAStencilSize);
        attrs.push_back(8);
        attrs.push_back(kCGLPFADisplayMask);
        attrs.push_back(CGDisplayIDToOpenGLDisplayMask(m_cgScreen));
        if (m_dataFormats[m_dataFormatIndex].stereoMode == QuadBufferStereo)
        {
            attrs.push_back(kCGLPFAStereo);
        }
        attrs.push_back(0);

        if (CGLError err = CGLChoosePixelFormat(
                (CGLPixelFormatAttribute*)&attrs.front(), &m_pfo, &m_npfo))
        {
            cout << "ERROR: choosing pixel format: " << CGLErrorString(err)
                 << endl;
            exit(-1);
        }

        if (CGLError err = CGLCreateContext(m_pfo, shareContext, &m_context))
        {
            cout << "ERROR: create context: " << CGLErrorString(err) << endl;
            exit(-1);
        }

        // CGDisplayHideCursor(m_cgScreen);
        m_savedMode = CGDisplayCopyDisplayMode(m_cgScreen);

        CGDisplayModeRef mode =
            displayModeRefFromIOMode(int32_t((size_t)format.data), m_cgScreen);
        CGDisplaySetDisplayMode(m_cgScreen, mode, NULL);
        CGDisplayCapture(m_cgScreen);

        CGLSetFullScreenOnDisplay(m_context,
                                  CGDisplayIDToOpenGLDisplayMask(m_cgScreen));
        GLint one = 1;
        GLint zero = 0;
        CGLSetParameter(m_context, kCGLCPSwapInterval, m_vsync ? &one : &zero);
        CGLSetParameter(m_context, kCGLCPSurfaceOrder, &one);

        m_viewDevice = new CGGLVideoDevice(0, format.width, format.height,
                                           false, m_context, false);
        m_viewDevice->makeCurrent();
    }

    bool CGDesktopVideoDevice::isOpen() const { return m_context != 0; }

    void CGDesktopVideoDevice::close()
    {
        //  unbind() should be called by the renderer::setOutputVideoDevice, but
        //  just in case, call here too.
        //
        unbind();

        ScopedLock lock(m_mutex);

        if (m_savedMode)
            CGDisplaySetDisplayMode(m_cgScreen, m_savedMode, NULL);
        if (m_context)
            CGLReleaseContext(m_context);
        if (m_cgScreen)
            CGDisplayRelease(m_cgScreen);
        if (m_savedMode)
            CFRelease(m_savedMode);
        m_savedMode = 0;
        delete m_viewDevice;
        m_viewDevice = 0;
        m_context = 0;
    }

    VideoDevice::Resolution CGDesktopVideoDevice::resolution() const
    {
        return format();
    }

    size_t CGDesktopVideoDevice::width() const { return resolution().width; }

    size_t CGDesktopVideoDevice::height() const { return resolution().height; }

    float CGDesktopVideoDevice::pixelScale() const
    {
        return resolution().pixelScale;
    }

    VideoDevice::VideoFormat CGDesktopVideoDevice::format() const
    {
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(m_cgScreen);
        int32_t id = CGDisplayModeGetIODisplayModeID(mode);
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
    }

    VideoDevice::Offset CGDesktopVideoDevice::offset() const
    {
        return Offset(0, 0);
    }

    VideoDevice::Timing CGDesktopVideoDevice::timing() const
    {
        return format();
    }

    TwkApp::VideoDevice::ColorProfile CGDesktopVideoDevice::colorProfile() const
    {
        //
        //  Get the display's color sync profile
        //

        if (ColorSyncProfileRef iccRef =
                ColorSyncProfileCreateWithDisplayID(m_cgScreen))
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

} // namespace Rv
