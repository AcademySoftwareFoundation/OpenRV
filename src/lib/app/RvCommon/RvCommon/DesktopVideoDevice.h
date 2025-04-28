
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
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/GLPipeline.h>
#include <TwkApp/VideoModule.h>
#include <boost/thread/mutex.hpp>

#include <RvCommon/QTGLVideoDevice.h>
#include <QtWidgets/QWidget>
#include <QOpenGLWidget>
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
        class ScreenView : public QOpenGLWidget
        {
        public:
            ScreenView(const QSurfaceFormat& fmt, QWidget* parent,
                       QOpenGLWidget* glViewShare, Qt::WindowFlags flags);

            void initializeGL() override;
            void paintGL() override;

        private:
            QOpenGLWidget* m_glViewShare = nullptr;
        };

    public:
        //
        //  Types
        //

        typedef std::map<const TwkGLF::GLFBO*, TwkGLF::GLFBO*> FBOMap;
        typedef boost::mutex Mutex;
        typedef boost::mutex::scoped_lock ScopedLock;

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
            DesktopVideoFormat(size_t w, size_t h, float pa, float ps,
                               float hertz, const std::string& desc,
                               void* xtradata = 0)
                : TwkApp::VideoDevice::VideoFormat(w, h, pa, ps, hertz, desc)
                , data(xtradata)
            {
            }

            DesktopVideoFormat(const TwkApp::VideoDevice::VideoFormat& v,
                               void* xtradata = 0)
                : TwkApp::VideoDevice::VideoFormat(v.width, v.height,
                                                   v.pixelAspect, v.pixelScale,
                                                   v.hz, v.description)
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

        DesktopVideoDevice(TwkApp::VideoModule*, const std::string& name,
                           int qtscreen, const QTGLVideoDevice* glViewShared);

        virtual ~DesktopVideoDevice();

        virtual void redraw() const;
        virtual void redrawImmediately() const;

        const QTGLVideoDevice* shareDevice() const { return m_share; }

        void setViewDevice(TwkGLF::GLVideoDevice* d) { m_viewDevice = d; }

        void setShareDevice(QTGLVideoDevice* d) { m_share = d; }

        //
        //  These can differ from the usual output versions in the case of
        //  frame packed.
        //

        virtual Resolution internalResolution() const;
        virtual VideoFormat internalFormat() const;
        virtual size_t internalHeight() const;

        //
        //  This will try and lock the sync mutex
        //

        virtual void blockUntilSyncComplete() const;

        //
        //  GLBindableVideoDevice API
        //

        virtual bool isStereo() const;
        virtual bool isDualStereo() const;

        // virtual bool willBlockOnTransfer() const;
        virtual void transfer(const TwkGLF::GLFBO*) const;
        virtual void transfer2(const TwkGLF::GLFBO*,
                               const TwkGLF::GLFBO*) const;

        virtual void unbind() const;

        virtual void clearCaches() const {}

        //
        //  Configurations
        //

        virtual size_t numVideoFormats() const;
        virtual VideoFormat videoFormatAtIndex(size_t) const;
        virtual VideoFormat videoFormatFromData(void*) const;
        virtual void setVideoFormat(size_t);
        virtual size_t currentVideoFormat() const;

        virtual size_t numDataFormats() const;
        virtual DataFormat dataFormatAtIndex(size_t) const;
        virtual void setDataFormat(size_t);
        virtual size_t currentDataFormat() const;

        virtual size_t numSyncModes() const;
        virtual SyncMode syncModeAtIndex(size_t) const;
        virtual void setSyncMode(size_t);
        virtual size_t currentSyncMode() const;

        virtual size_t numSyncSources() const;
        virtual SyncSource syncSourceAtIndex(size_t) const;
        virtual void setSyncSource(size_t);
        virtual size_t currentSyncSource() const;

        // imported

        //  From QTGLVideoDevice

        void setViewWidget(QOpenGLWidget*);

        QOpenGLWidget* viewWidget() const { return m_view; }

        virtual void makeCurrent() const;

        const QTTranslator& translator() const { return *m_translator; }

        //
        //  VideoDevice API
        //

        virtual Resolution resolution() const;
        virtual Offset offset() const;
        virtual Timing timing() const;
        virtual VideoFormat format() const;

        virtual size_t width() const;
        virtual size_t height() const;

        virtual void open(const StringVector&);
        virtual void close();
        virtual bool isOpen() const;

        virtual void syncBuffers() const;

        virtual int qtScreen() const { return m_screen; }

        bool useFullScreen() const;
        QRect screenGeometry() const;

        static std::vector<VideoDevice*>
        createDesktopVideoDevices(TwkApp::VideoModule* module,
                                  const QTGLVideoDevice* shareDevice);

    protected:
        void addDefaultDataFormats(size_t bits = 8);
        void sortVideoFormatsByWidth();

        void setupModelviewAndProjection(float w, float h,
                                         TwkGLF::GLPipeline*) const;
        void fillWithTexture(const TwkGLF::GLFBO*, size_t, float, float,
                             TwkGLF::GLPipeline*) const;
        void fillWithTexture2(const TwkGLF::GLFBO*, const TwkGLF::GLFBO*,
                              size_t, float, float, TwkGLF::GLPipeline*) const;

        bool maybeFramePacked(const TwkApp::VideoDevice::VideoFormat&) const;

    private:
        void addDataFormatAtDepth(size_t depth, DesktopStereoMode m);

#ifdef PLATFORM_WINDOWS
        virtual ColorProfile colorProfile() const;
#endif

    protected:
        const QTGLVideoDevice* m_share;
        const TwkGLF::GLVideoDevice* m_viewDevice;
        QOpenGLWidget* m_view;
        DesktopStereoMode m_stereoMode;
        mutable FBOMap m_fboMap;
        TwkGLF::GLState* m_glGlobalState;
        DesktopVideoFormats m_videoFormats;
        DesktopDataFormats m_dataFormats;

        int m_screen;
        QTTranslator* m_translator;
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
