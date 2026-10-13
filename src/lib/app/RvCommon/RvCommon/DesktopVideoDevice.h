
//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__DesktopVideoDevice__h__
#define __RvCommon__DesktopVideoDevice__h__
#include <iostream>
#include <memory>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/GLPipeline.h>
#include <TwkApp/VideoModule.h>
#include <boost/thread/mutex.hpp>

#include <RvCommon/QTGLVideoDevice.h>
#include <QtWidgets/QWidget>
#include <QOpenGLWindow>
#include <QOpenGLContext>
#include <QGuiApplication>

namespace Rv
{

    //
    //  DesktopVideoDevice
    //
    //  This is the base class for desktop video devices. Currently there
    //  is one, but we mean to have three:
    //
    //      DesktopVideoDevice:
    //           Generic Qt only device uses QScreen.
    //           Works for all platforms but cannot enumerate
    //           screens and resolutions, and effectuate a
    //           resolution switch (eg: not supported by Qt)
    //
    //      MacDesktopVideoDevice:
    //      LinuxDesktopVideoDevice:
    //      WindowsDesktopVideoDevice:
    //           Not implemented yet (pushed to later) but they
    //           are meant to use the native OS system calls to
    //           effectuate resolution changes.
    //
    //           This only ever worked for mac, but when moving to
    //           QOpenGLWidget (Qt6) we had to temporarily remove this
    //           support when we made the Mac use the Qt path.
    //           Workaround: change resolution in OS settings first.
    //
    //  This class implements the GL guts shared by the above and
    //  holds on to the static data structs for formats, etc.
    //

    class DesktopVideoDevice : public TwkGLF::GLBindableVideoDevice
    {
    public:
        //
        //  QOpenGLWindow, not a top-level QOpenGLWidget: only a ctor-supplied
        //  share context guarantees the renderer's share group, which
        //  transfer() needs to see the renderer's textures. PartialUpdateBlit
        //  keeps a backing FBO and does not clear before paintGL().
        //
        class ScreenWindow : public QOpenGLWindow
        {
        public:
            ScreenWindow(const QSurfaceFormat& fmt, QOpenGLContext* glShareContext);

            void initializeGL() override;
            void paintGL() override;

        private:
            QOpenGLContext* m_glShareContext = nullptr;

            //  Size of the backing FBO at the last paint; a change means it
            //  was (re)allocated empty.
            QSize m_paintedSize;
        };

        //  QWidget container for the ScreenWindow, as GLView is for GLWindow.
        class ScreenView : public QWidget
        {
        public:
            //
            //  glShareContext is the control view's GL context to share with
            //  (so blits/FBOs are usable across the two surfaces). It comes
            //  from QTGLVideoDevice::glShareContext() and is backing-agnostic:
            //  the control view may be a QOpenGLWidget or a QOpenGLWindow. A
            //  null share context falls back to Qt's global share context.
            //
            ScreenView(const QSurfaceFormat& fmt, QWidget* parent, QOpenGLContext* glShareContext, Qt::WindowFlags flags);

            ScreenWindow* glWindow() const { return m_glWindow; }

        private:
            ScreenWindow* m_glWindow = nullptr;
        };

    public:
        //
        //  Types
        //

        using FBOMap = std::map<const TwkGLF::GLFBO*, std::unique_ptr<TwkGLF::GLFBO>>;
        using Mutex = boost::mutex;
        using ScopedLock = boost::mutex::scoped_lock;

        enum DesktopStereoMode
        {
            Mono,
            QuadBufferStereo,
            TopBottomStereo,
            SideBySideStereo,
            ScanlineStereo,
            CheckerStereo,
            FramePacked
        };

        enum GLSyncMode
        {
            NoSync,
            VSync
        };

        struct DesktopDataFormat : public TwkApp::VideoDevice::DataFormat
        {
            DesktopDataFormat(const std::string& desc, DesktopStereoMode fms)
                : DataFormat(desc)
                , stereoMode(fms)
            {
            }

            DesktopStereoMode stereoMode;
        };

        struct DesktopVideoFormat : public TwkApp::VideoDevice::VideoFormat
        {
            DesktopVideoFormat(size_t w, size_t h, float pa, float ps, float hertz, const std::string& desc, void* xtradata = 0)
                : TwkApp::VideoDevice::VideoFormat(w, h, pa, ps, hertz, desc)
                , data(xtradata)
            {
            }

            DesktopVideoFormat(const TwkApp::VideoDevice::VideoFormat& v, void* xtradata = 0)
                : TwkApp::VideoDevice::VideoFormat(v.width, v.height, v.pixelAspect, v.pixelScale, v.hz, v.description)
                ,

                data(xtradata)
            {
            }

            void* data;
        };

        typedef std::vector<DesktopDataFormat> DesktopDataFormats;
        typedef std::vector<DesktopVideoFormat> DesktopVideoFormats;

        //
        //
        //

        DesktopVideoDevice(TwkApp::VideoModule*, const std::string& name, int qtscreen, const QTGLVideoDevice* glViewShared);

        ~DesktopVideoDevice() override;

        virtual void redraw() const;
        virtual void redrawImmediately() const;

        const QTGLVideoDevice* shareDevice() const { return m_share; }

        void setViewDevice(TwkGLF::GLVideoDevice* d) { m_viewDevice = d; }

        void setShareDevice(QTGLVideoDevice* d) { m_share = d; }

        //
        //  These can differ from the usual output versions in the case of
        //  frame packed.
        //

        Resolution internalResolution() const override;
        VideoFormat internalFormat() const override;
        size_t internalHeight() const override;

        //
        //  This will try and lock the sync mutex
        //

        void blockUntilSyncComplete() const override;

        //
        //  GLBindableVideoDevice API
        //

        bool isStereo() const override;
        bool isDualStereo() const override;

        // virtual bool willBlockOnTransfer() const;
        void transfer(const TwkGLF::GLFBO*) const override;
        void transfer2(const TwkGLF::GLFBO*, const TwkGLF::GLFBO*) const override;

        void unbind() const override;

        void clearCaches() const override { releaseFBOClones(); }

        //
        //  Delete the FBO clones in m_fboMap. Must run while this device's view
        //  context is still alive, so close() calls it first. Idempotent.
        //
        void releaseFBOClones() const;

        //
        //  This context's cached clone of a renderer FBO, wrapping its colour
        //  texture (FBOs are not shared across contexts, textures are). The
        //  cache key is a pointer the renderer can reuse, so hits are
        //  re-verified. Returns null if no complete clone could be built.
        //
        TwkGLF::GLFBO* cloneForSource(const TwkGLF::GLFBO* sourceFbo) const;

        //
        //  Configurations
        //

        size_t numVideoFormats() const override;
        VideoFormat videoFormatAtIndex(size_t) const override;
        virtual VideoFormat videoFormatFromData(void*) const;
        void setVideoFormat(size_t) override;
        size_t currentVideoFormat() const override;

        size_t numDataFormats() const override;
        DataFormat dataFormatAtIndex(size_t) const override;
        void setDataFormat(size_t) override;
        size_t currentDataFormat() const override;

        size_t numSyncModes() const override;
        SyncMode syncModeAtIndex(size_t) const override;
        void setSyncMode(size_t) override;
        size_t currentSyncMode() const override;

        size_t numSyncSources() const override;
        SyncSource syncSourceAtIndex(size_t) const override;
        void setSyncSource(size_t) override;
        size_t currentSyncSource() const override;

        // imported

        //  From QTGLVideoDevice

        void setViewWidget(ScreenView*);

        ScreenView* viewWidget() const { return m_view; }

        virtual void makeCurrent() const;

        const QTTranslator& translator() const { return *m_translator; }

        //
        //  VideoDevice API
        //

        Resolution resolution() const override;
        Offset offset() const override;
        Timing timing() const override;
        VideoFormat format() const override;

        size_t width() const override;
        size_t height() const override;

        void open(const StringVector&) override;
        void close() override;
        bool isOpen() const override;

        void syncBuffers() const override;

        virtual int qtScreen() const { return m_screen; }

        bool useFullScreen() const;
        QRect screenGeometry() const;

        static std::vector<VideoDevice*> createDesktopVideoDevices(TwkApp::VideoModule* module, const QTGLVideoDevice* shareDevice);

        //  As above, with the backend supplied by the caller. Use once the main view is live.
        static std::vector<VideoDevice*> createDesktopVideoDevices(TwkApp::VideoModule* module, const QTGLVideoDevice* shareDevice,
                                                                   bool useVulkan);

        //
        //  Presentation backend for the initial build: true for a 10-bit request
        //  this machine's Vulkan can present. Reads the persisted preference,
        //  which can differ from the live main-view backend, so it must not be
        //  used once a view exists. Always false on macOS.
        //
        static bool shouldUseVulkanPresentation();

        //  True when the persisted display depth is RGB 10 + A 2.
        static bool tenBitDisplayRequested();

        //
        //  An output surface only shows what the control session composites
        //  into it, so a freshly shown or resized one holds nothing. Asks every
        //  session driving a separate output device for a new frame.
        //
        static void requestOutputRecomposite();

    protected:
        void addDefaultDataFormats(size_t bits = 8);
        void sortVideoFormatsByWidth();

        void setupModelviewAndProjection(float w, float h, TwkGLF::GLPipeline*) const;
        void fillWithTexture(const TwkGLF::GLFBO*, size_t, float, float, TwkGLF::GLPipeline*) const;
        void fillWithTexture2(const TwkGLF::GLFBO*, const TwkGLF::GLFBO*, size_t, float, float, TwkGLF::GLPipeline*) const;

        bool maybeFramePacked(const TwkApp::VideoDevice::VideoFormat&) const;

    private:
        void addDataFormatAtDepth(size_t depth, DesktopStereoMode m);

#ifdef PLATFORM_WINDOWS
        ColorProfile colorProfile() const override;
#endif

    protected:
        const QTGLVideoDevice* m_share;
        const TwkGLF::GLVideoDevice* m_viewDevice;
        ScreenView* m_view;
        DesktopStereoMode m_stereoMode;
        mutable FBOMap m_fboMap;

        //  Last unusable source texture reported, so the report fires once per transition.
        mutable GLuint m_reportedBadSourceTex{0};

        //  Latches the "no backing FBO" report.
        mutable bool m_transferStalled{false};
        TwkGLF::GLState* m_glGlobalState;
        DesktopVideoFormats m_videoFormats;
        DesktopDataFormats m_dataFormats;

        int m_screen;
        std::unique_ptr<QTTranslator> m_translator;
        mutable ColorProfile m_colorProfile;

        size_t m_videoFormatIndex;
        size_t m_dataFormatIndex;
        bool m_vsync;
        mutable bool m_syncing;
        mutable Mutex m_mutex;
        mutable Mutex m_syncTestMutex;
    };

} // namespace Rv

#endif // __RvCommon__DesktopVideoDevice__h__
