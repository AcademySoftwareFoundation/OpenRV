//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifdef PLATFORM_WINDOWS
#include <GL/glew.h>
#include <GL/wglew.h>
#include <Wingdi.h>
#include <Shlwapi.h>
#include <lcms2.h>
#endif

#include <RvCommon/DesktopVideoDevice.h>
#include <TwkGLF/GLPipeline.h>
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkGLF/BasicGLProgram.h>
#include <TwkGLF/GLFBO.h>
#include <TwkMath/Frustum.h>
#include <stdio.h>

#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>

#include <QOpenGLContext>
#include <QScreen>

// #define DEBUG_NO_FULLSCREEN

namespace Rv
{

    using namespace std;
    using namespace TwkGLF;
    using namespace TwkApp;

    DesktopVideoDevice::DesktopVideoDevice(VideoModule* m,
                                           const std::string& name, int screen,
                                           const QTGLVideoDevice* glViewShared)
        : GLBindableVideoDevice(m, name, ImageOutput | NormalizedCoordinates)
        , m_viewDevice(0)
        , m_share(glViewShared)
        , m_stereoMode(Mono)
        , m_vsync(true)
        , m_videoFormatIndex(0)
        , m_dataFormatIndex(0)
        , m_syncing(false)
        , m_screen(screen)
        , m_translator(0)
        , m_view(0)
    {
        m_glGlobalState = new GLState();
        m_glGlobalState->useGLProgram(textureRectGLProgram());

        const QList<QScreen*> screens = QGuiApplication::screens();
        if (screen < 0 || screen >= screens.size())
        {
            m_screen = 0;
        }

        const QRect rect = screenGeometry();

        ostringstream str;
        str << rect.width() << " x " << rect.height();
        m_videoFormats.push_back(
            VideoFormat(rect.width(), rect.height(), 1.0, 1.0, 0.0, str.str()));
        addDefaultDataFormats();
    }

    DesktopVideoDevice::~DesktopVideoDevice()
    {
        close();
        delete m_glGlobalState;
    }

    void DesktopVideoDevice::redraw() const
    {
        if (m_view)
        {
            ScopedLock lock(m_mutex);
            QSize s = m_view->size();
            m_view->update();
        }
    }

    void DesktopVideoDevice::redrawImmediately() const
    {
        if (m_view && m_view->isVisible())
        {
            ScopedLock lock(m_mutex);
            TWK_GLDEBUG;
            m_view->update();
            TWK_GLDEBUG;
        }
        else
        {
            TWK_GLDEBUG;
            redraw();
            TWK_GLDEBUG;
        }
    }

    void DesktopVideoDevice::blockUntilSyncComplete() const
    {
        ScopedLock lock(m_mutex);
    }

    void DesktopVideoDevice::transfer(const GLFBO* sourceFbo) const
    {
        ScopedLock lock(m_mutex);

        //        sourceFbo->debugSaveFramebuffer();

        // viewDevice->fboID will eventuall retrieve the ScreenView's FBO id.
        // But if was never shown/painted yet, then it will be 0.
        // If that's the case then nothing to do for now. Let's defer this to
        // later.
        GLint svFboId = m_viewDevice->fboID();
        if (svFboId == 0)
            return;

        // Switch to the ScreenView's OpenGL context.
        m_viewDevice->makeCurrent(); // calls screenview's makeCurrent, sets the
                                     // font current, etc etc.

        // Next, because we can't blit from FBOs belonging to different
        // contexts, check to see if we already have an clone of the source FBO
        // associated to the view's context.
        GLFBO* svSourceFbo = m_fboMap[sourceFbo];
        if (!svSourceFbo)
        {
            // We don't yet have a clone of the source FBO living in the context
            // of the ScreenView. Therefore, we now create this new clone FBO
            // with the dimensions/format as the source.
            // Afterwards, associate the source FBO's color attachment to the
            // clone's color attachment, because color attachments *can* be
            // shared for the blit operation
            svSourceFbo = new GLFBO(sourceFbo->width(), sourceFbo->height(),
                                    sourceFbo->primaryColorFormat());

            svSourceFbo->attachColorTexture(
                sourceFbo->colorTarget(0),
                sourceFbo->colorID(0)); // PB: What's colorId ?
            m_fboMap[sourceFbo] = svSourceFbo;
        }

        // Finally, create a temporary GLFBO with the ID of the ScreenView's FBO
        // so that we can blit to it.  BTW, the copyTo() call queries width()
        // and height() on the videodevice, which in turn queries the view.
        const GLFBO* svDestFbo = m_viewDevice->defaultFBO();

        // Copy the FBO (first time = black) into the FBO of the QOpenGLWidget.
        svSourceFbo->copyTo(svDestFbo);

        // Note: do not delete svDestFBO because it's an internal data member of
        // m_viewDevice

        // Note: Upon existing from here, the caller will restore the correct
        // current context.
    }

    void DesktopVideoDevice::open(const StringVector& args)
    {
        TWK_GLDEBUG;

        QSurfaceFormat fmt = shareDevice()->widget()->format();
        fmt.setSwapInterval(m_vsync ? 1 : 0);

        ScreenView* vw =
            new ScreenView(fmt, 0, shareDevice()->widget(), Qt::Window);
        setViewWidget(vw);

        QTGLVideoDevice* vd = new QTGLVideoDevice(0, "local view", vw);
        setViewDevice(vd);

        QRect g = screenGeometry();
        viewWidget()->move(g.x(), g.y());
        viewWidget()->setGeometry(g);

        if (useFullScreen())
            viewWidget()->setWindowState(Qt::WindowFullScreen);
        else
            viewWidget()->setWindowState(Qt::WindowNoState);

        TWK_GLDEBUG;

        viewWidget()->setGeometry(g);

        viewWidget()->show();
        //        QCoreApplication::processEvents(); // force the window to
        //        show. m_share->makeCurrent();

        TWK_GLDEBUG;
    }

    void DesktopVideoDevice::close()
    {
        delete m_view;
        delete m_viewDevice;
        delete m_translator;
        m_view = 0;
        m_viewDevice = 0;
        m_translator = 0;
    }

    void DesktopVideoDevice::setViewWidget(QOpenGLWidget* widget)
    {
        m_view = widget;
        m_translator = new QTTranslator(this, m_view);
    }

    void DesktopVideoDevice::makeCurrent() const
    {
        if (m_view)
            m_view->makeCurrent();
    }

    void DesktopVideoDevice::setupModelviewAndProjection(
        float w, float h, GLPipeline* glPipeline) const
    {
        // viewport, modelview, proj
        glPipeline->setViewport(0, 0, w, h);

        TwkMath::Mat44f identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
                                 1);
        glPipeline->setModelview(identity);

        TwkMath::Frustumf f;
        f.window(0, w - 1, 0, h - 1, -1, 1, true);
        TwkMath::Mat44f projMat = f.matrix();

        glPipeline->setProjection(projMat);
    }

    void DesktopVideoDevice::fillWithTexture2(const GLFBO* fbo1,
                                              const GLFBO* fbo2, size_t n,
                                              float w, float h,
                                              GLPipeline* glPipeline) const
    {
        // tex
        GLint id = 0;
        glPipeline->setUniformInt("texture0", 1, &id);
        glActiveTexture(GL_TEXTURE0);
        fbo1->bindColorTexture(n);
        id = 1;
        glPipeline->setUniformInt("texture1", 1, &id);
        glActiveTexture(GL_TEXTURE1);
        fbo2->bindColorTexture(n);

        // draw
        float data[] = {0, 0, 0,     0,     w, 0, w - 1, 0,
                        w, h, w - 1, h - 1, 0, h, 0,     h - 1};
        PrimitiveData buffer(data, NULL, GL_QUADS, 4, 1, 16 * sizeof(float));
        std::vector<VertexAttribute> attributeInfo;

        attributeInfo.push_back(VertexAttribute(std::string("in_Position"),
                                                GL_FLOAT, 2, 2 * sizeof(float),
                                                4 * sizeof(float)));

        attributeInfo.push_back(VertexAttribute(
            std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 4 * sizeof(float)));

        RenderPrimitives renderprimitives(m_glGlobalState->activeGLProgram(),
                                          buffer, attributeInfo,
                                          m_glGlobalState->vboList());
        renderprimitives.setupAndRender();

        fbo1->unbindColorTexture();
        fbo2->unbindColorTexture();
    }

    void DesktopVideoDevice::fillWithTexture(const GLFBO* fbo, size_t n,
                                             float w, float h,
                                             GLPipeline* glPipeline) const
    {
        // tex
        GLint id = 0;
        glPipeline->setUniformInt("texture0", 1, &id);
        glActiveTexture(GL_TEXTURE0);
        fbo->bindColorTexture(n);

        // draw
        float data[] = {0, 0, 0,     0,     w, 0, w - 1, 0,
                        w, h, w - 1, h - 1, 0, h, 0,     h - 1};
        PrimitiveData buffer(data, NULL, GL_QUADS, 4, 1, 16 * sizeof(float));
        std::vector<VertexAttribute> attributeInfo;

        attributeInfo.push_back(VertexAttribute(std::string("in_Position"),
                                                GL_FLOAT, 2, 2 * sizeof(float),
                                                4 * sizeof(float)));

        attributeInfo.push_back(VertexAttribute(
            std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 4 * sizeof(float)));

        RenderPrimitives renderprimitives(m_glGlobalState->activeGLProgram(),
                                          buffer, attributeInfo,
                                          m_glGlobalState->vboList());
        renderprimitives.setupAndRender();

        fbo->unbindColorTexture();
    }

    void DesktopVideoDevice::transfer2(const GLFBO* fbo1,
                                       const GLFBO* fbo2) const
    {
        //
        //  NOTE: fbo1 and fbo2 are in the controllers context. So we
        //  can't use them directly. FBOs are not shared between contexts
        //  (textures are) so we need to use our own local fbos which use
        //  the shared textures.
        //

        ScopedLock lock(m_mutex);
        m_viewDevice->makeCurrent();
        TWK_GLDEBUG;
        const float w = m_viewDevice->width();
        const float h = m_viewDevice->height();

        GLFBO* local_fbo1 = m_fboMap[fbo1];
        GLFBO* local_fbo2 = m_fboMap[fbo2];

        if (!local_fbo1)
        {
            local_fbo1 = new GLFBO(fbo1->width(), fbo1->height(),
                                   fbo1->primaryColorFormat());
            local_fbo1->attachColorTexture(fbo1->colorTarget(0),
                                           fbo1->colorID(0));
            m_fboMap[fbo1] = local_fbo1;
        }

        if (!local_fbo2)
        {
            local_fbo2 = new GLFBO(fbo2->width(), fbo2->height(),
                                   fbo2->primaryColorFormat());
            local_fbo2->attachColorTexture(fbo2->colorTarget(0),
                                           fbo2->colorID(0));
            m_fboMap[fbo2] = local_fbo2;
        }

        const GLFBO *leftFBO = local_fbo1, *rightFBO = local_fbo2;

#if 0
    {
        const GLFBO* fbo = leftFBO;
        fbo->bind(GL_READ_FRAMEBUFFER);

        TwkFB::FrameBuffer fb(fbo->width(), fbo->height(), 4, TwkFB::FrameBuffer::UCHAR);
        std::vector<const TwkFB::FrameBuffer*> fbs(1);
        fbs.front() = &fb;
        
        glReadPixels(0,
                     0,
                     fbo->width(),
                     fbo->height(),
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     fb.pixels<GLvoid>());
        
        ostringstream file;
        file << "desktopFBO_left.tif";
        TwkFB::GenericIO::writeImages(fbs, file.str(), TwkFB::FrameBufferIO::WriteRequest());
    }

    {
        const GLFBO* fbo = rightFBO;
        fbo->bind(GL_READ_FRAMEBUFFER);

        TwkFB::FrameBuffer fb(fbo->width(), fbo->height(), 4, TwkFB::FrameBuffer::UCHAR);
        std::vector<const TwkFB::FrameBuffer*> fbs(1);
        fbs.front() = &fb;
        
        glReadPixels(0,
                     0,
                     fbo->width(),
                     fbo->height(),
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     fb.pixels<GLvoid>());
        
        ostringstream file;
        file << "desktopFBO_right.tif";
        TwkFB::GenericIO::writeImages(fbs, file.str(), TwkFB::FrameBufferIO::WriteRequest());
        fbo->unbind();
    }

#endif

        if (m_stereoMode == SideBySideStereo)
        {
            const GLFBO* outputFBO = m_viewDevice->defaultFBO();
            local_fbo1->copyRegionTo(outputFBO, 0, 0, 1, 1, 0, 0, .5, 1,
                                     GL_COLOR_BUFFER_BIT, GL_LINEAR);
            local_fbo2->copyRegionTo(outputFBO, 0, 0, 1, 1, .5, 0, .5, 1,
                                     GL_COLOR_BUFFER_BIT, GL_LINEAR);
            outputFBO->unbind();
        }
        else if (m_stereoMode == TopBottomStereo)
        {
            const GLFBO* outputFBO = m_viewDevice->defaultFBO();
            local_fbo1->copyRegionTo(outputFBO, 0, 0, 1, 1, 0, .5, 1, .5,
                                     GL_COLOR_BUFFER_BIT, GL_LINEAR);
            local_fbo2->copyRegionTo(outputFBO, 0, 0, 1, 1, 0, 0, 1, .5,
                                     GL_COLOR_BUFFER_BIT, GL_LINEAR);
            outputFBO->unbind();
        }
        else if (m_stereoMode == QuadBufferStereo)
        {
            local_fbo1
                ->unbind(); // this is necessary, because a fbo could be bound
                            // (from the render) at this point, and it will not
                            // have stereo, or double buffering causing the
                            // following glDrawBuffer lines to crash.
            GLPipeline* glPipeline =
                m_glGlobalState->useGLProgram(textureRectGLProgram());
            setupModelviewAndProjection(w, h, glPipeline);
            glDrawBuffer(GL_BACK_LEFT); // CORE DUMPS!? on Mac
            fillWithTexture(fbo1, 0, w, h, glPipeline);
            glDrawBuffer(GL_BACK_RIGHT); // CORE DUMPS!? on Mac
            fillWithTexture(fbo2, 0, w, h, glPipeline);
            glDrawBuffer(GL_BACK); // CORE DUMPS!? on Mac (yes, this one too!)
        }
        else if (m_stereoMode == ScanlineStereo)
        {
            const GLFBO* outputFBO = m_viewDevice->defaultFBO();
            outputFBO->bind();
            GLPipeline* glPipeline =
                m_glGlobalState->useGLProgram(stereoScanlineGLProgram());
            setupModelviewAndProjection(w, h, glPipeline);
            // send in uniforms
            VideoDevice::Offset o = this->offset();
            int viewYOrigin = this->height() - o.y - 1 - this->margins().bottom;
            float parity = viewYOrigin % 2 == 0 ? 0.0 : 1.0;
            glPipeline->setUniformFloat(
                "parityOffset", 1, &parity); // send GL the uniform values to
                                             // the current active glProgram
            fillWithTexture2(leftFBO, rightFBO, 0, w, h, glPipeline);
            outputFBO->unbind();
        }
        else if (m_stereoMode == CheckerStereo)
        {
            const GLFBO* outputFBO = m_viewDevice->defaultFBO();
            outputFBO->bind();
            GLPipeline* glPipeline =
                m_glGlobalState->useGLProgram(stereoCheckerGLProgram());
            setupModelviewAndProjection(w, h, glPipeline);
            // send in uniforms
            VideoDevice::Offset o = this->offset();
            int viewXOrigin = o.x;
            int viewYOrigin = this->height() - o.y - 1 - this->margins().bottom;
            float parity[2];
            parity[0] = viewXOrigin % 2 == 0 ? 0.0 : 1.0;
            parity[1] = viewYOrigin % 2 == 0 ? 0.0 : 1.0;
            glPipeline->setUniformFloat(
                "parityOffset", 2, parity); // send GL the uniform values to the
                                            // current active glProgram
            fillWithTexture2(leftFBO, rightFBO, 0, w, h, glPipeline);
            outputFBO->unbind();
        }
        else if (m_stereoMode == FramePacked)
        {
            const GLFBO* outputFBO = m_viewDevice->defaultFBO();
            const float iw = internalWidth();
            const float ih = internalHeight();
            const float padding = h - ih * 2.0;

            const float p0 = ih / h;
            const float p1 = (ih + padding) / h;

            outputFBO->bind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            //
            //  XXX This block for testing -- BEGIN
            //

            static bool userOverrideFirst = true;
            static bool userOverride = false;

            static float userOverrideLSrcX, userOverrideLSrcY,
                userOverrideLSrcW, userOverrideLSrcH;
            static float userOverrideLDstX, userOverrideLDstY,
                userOverrideLDstW, userOverrideLDstH;
            static float userOverrideRSrcX, userOverrideRSrcY,
                userOverrideRSrcW, userOverrideRSrcH;
            static float userOverrideRDstX, userOverrideRDstY,
                userOverrideRDstW, userOverrideRDstH;

            if (userOverrideFirst)
            {
                userOverrideFirst = false;

                const char* leftCopyString =
                    getenv("RV_FRAME_PACKED_LEFT_COPY");
                const char* rightCopyString =
                    getenv("RV_FRAME_PACKED_RIGHT_COPY");

                if (leftCopyString && rightCopyString)
                {
                    sscanf(leftCopyString, "%f %f %f %f %f %f %f %f",
                           &userOverrideLSrcX, &userOverrideLSrcY,
                           &userOverrideLSrcW, &userOverrideLSrcH,
                           &userOverrideLDstX, &userOverrideLDstY,
                           &userOverrideLDstW, &userOverrideLDstH);

                    sscanf(rightCopyString, "%f %f %f %f %f %f %f %f",
                           &userOverrideRSrcX, &userOverrideRSrcY,
                           &userOverrideRSrcW, &userOverrideRSrcH,
                           &userOverrideRDstX, &userOverrideRDstY,
                           &userOverrideRDstW, &userOverrideRDstH);

                    userOverride = true;

                    float lw = local_fbo1->width();
                    float rw = local_fbo2->width();
                    float lh = local_fbo1->height();
                    float rh = local_fbo2->height();

                    float dw = outputFBO->width();
                    float dh = outputFBO->height();

                    cerr << "ndc left copy  " << " x " << userOverrideLSrcX
                         << " y " << userOverrideLSrcY << " w "
                         << userOverrideLSrcW << " h " << userOverrideLSrcH
                         << " -> " << " x " << userOverrideLDstX << " y "
                         << userOverrideLDstY << " w " << userOverrideLDstW
                         << " h " << userOverrideLDstH << endl;

                    cerr << "ndc right copy " << " x " << userOverrideRSrcX
                         << " y " << userOverrideRSrcY << " w "
                         << userOverrideRSrcW << " h " << userOverrideRSrcH
                         << " -> " << " x " << userOverrideRDstX << " y "
                         << userOverrideRDstY << " w " << userOverrideRDstW
                         << " h " << userOverrideRDstH << endl;

                    cerr << "pixel left copy  " << " x0 "
                         << int(userOverrideLSrcX * lw) << " y0 "
                         << int(userOverrideLSrcY * lh) << " x1 "
                         << int((userOverrideLSrcX + userOverrideLSrcW) * lw)
                         << " y1 "
                         << int((userOverrideLSrcY + userOverrideLSrcH) * lh)
                         << " -> " << " x0 " << int(userOverrideLDstX * dw)
                         << " y0 " << int(userOverrideLDstY * dh) << " x1 "
                         << int((userOverrideLDstX + userOverrideLDstW) * dw)
                         << " y1 "
                         << int((userOverrideLDstY + userOverrideLDstH) * dh)
                         << endl;

                    cerr << "pixel right copy " << " x0 "
                         << int(userOverrideRSrcX * rw) << " y0 "
                         << int(userOverrideRSrcY * rh) << " x1 "
                         << int((userOverrideRSrcX + userOverrideRSrcW) * rw)
                         << " y1 "
                         << int((userOverrideRSrcY + userOverrideRSrcH) * rh)
                         << " -> " << " x0 " << int(userOverrideRDstX * dw)
                         << " y0 " << int(userOverrideRDstY * dh) << " x1 "
                         << int((userOverrideRDstX + userOverrideRDstW) * dw)
                         << " y1 "
                         << int((userOverrideRDstY + userOverrideRDstH) * dh)
                         << endl;
                }
            }

            if (userOverride)
            {
                leftFBO->copyRegionTo(
                    outputFBO, userOverrideLSrcX, userOverrideLSrcY,
                    userOverrideLSrcW, userOverrideLSrcH, userOverrideLDstX,
                    userOverrideLDstY, userOverrideLDstW, userOverrideLDstH,
                    GL_COLOR_BUFFER_BIT, GL_LINEAR);

                rightFBO->copyRegionTo(
                    outputFBO, userOverrideRSrcX, userOverrideRSrcY,
                    userOverrideRSrcW, userOverrideRSrcH, userOverrideRDstX,
                    userOverrideRDstY, userOverrideRDstW, userOverrideRDstH,
                    GL_COLOR_BUFFER_BIT, GL_LINEAR);
            }
            //
            //  XXX This block for testing -- END
            //
            else
            {
                local_fbo1->copyRegionTo(outputFBO, 0, 0, 1,
                                         1,            // NDC src x, y, w, h
                                         0, p1, 1, p0, // NDC dst x, y, w, h
                                         GL_COLOR_BUFFER_BIT, GL_LINEAR);

                local_fbo2->copyRegionTo(outputFBO, 0, 0, 1, 1, 0, 0, 1, p0,
                                         GL_COLOR_BUFFER_BIT, GL_LINEAR);
            }
            outputFBO->unbind();
        }
    }

    bool DesktopVideoDevice::isStereo() const { return m_stereoMode != Mono; }

    bool DesktopVideoDevice::isDualStereo() const { return isStereo(); }

    void DesktopVideoDevice::unbind() const
    {
        ScopedLock lock(m_mutex);

        for (FBOMap::iterator i = m_fboMap.begin(); i != m_fboMap.end(); ++i)
        {
            delete i->second;
        }

        m_fboMap.clear();
    }

    size_t DesktopVideoDevice::numVideoFormats() const
    {
        return m_videoFormats.size();
    }

    DesktopVideoDevice::VideoFormat
    DesktopVideoDevice::videoFormatAtIndex(size_t i) const
    {
        if (i >= m_videoFormats.size())
            i = m_videoFormats.size() - 1;
        return m_videoFormats[i];
    }

    DesktopVideoDevice::VideoFormat
    DesktopVideoDevice::videoFormatFromData(void* data) const
    {
        for (int i = 0; i < m_videoFormats.size(); ++i)
            if (m_videoFormats[i].data == data)
                return m_videoFormats[i];

        return m_videoFormats[m_videoFormatIndex];
    }

    void DesktopVideoDevice::setVideoFormat(size_t i)
    {
        if (i >= m_videoFormats.size())
            i = m_videoFormats.size() - 1;
        m_videoFormatIndex = i;
        GLBindableVideoDevice::setVideoFormat(i);
    }

    size_t DesktopVideoDevice::currentVideoFormat() const
    {
        return m_videoFormatIndex;
    }

    bool DesktopVideoDevice::maybeFramePacked(
        const TwkApp::VideoDevice::VideoFormat& d) const
    {
        return
            //(d.width == 1920 && d.height >= 1080 * 2 && d.hz == 24) ||
            //(d.width == 1280 && d.height >= 720 * 2 && (d.hz == 50 || d.hz ==
            // 60));
            (d.width == 1920 && d.height >= 1080 * 2)
            || (d.width == 1280 && d.height >= 720 * 2);
    }

    TwkApp::VideoDevice::Resolution
    DesktopVideoDevice::internalResolution() const
    {
        const DesktopVideoDevice::DesktopVideoFormat& d =
            m_videoFormats[m_videoFormatIndex];

        if (maybeFramePacked(d))
        {
            if (d.width == 1920)
                return Resolution(1920, 1080, d.pixelAspect, d.pixelScale);
            else if (d.width == 1280)
                return Resolution(1280, 720, d.pixelAspect, d.pixelScale);
        }

        return resolution();
    }

    TwkApp::VideoDevice::VideoFormat DesktopVideoDevice::internalFormat() const
    {
        const DesktopVideoDevice::DesktopVideoFormat& d =
            m_videoFormats[m_videoFormatIndex];

        if (maybeFramePacked(d))
        {
            if (d.width == 1920)
                return VideoFormat(1920, 1080, d.pixelAspect, d.pixelScale,
                                   d.hz, d.description);
            else if (d.width == 1280)
                return VideoFormat(1280, 720, d.pixelAspect, d.pixelScale, d.hz,
                                   d.description);
        }

        return format();
    }

    size_t DesktopVideoDevice::internalHeight() const
    {
        const DesktopVideoDevice::DesktopVideoFormat& d =
            m_videoFormats[m_videoFormatIndex];

        if (maybeFramePacked(d))
        {
            if (d.width == 1920)
                return 1080;
            else if (d.width == 1280)
                return 720;
        }

        return height();
    }

    size_t DesktopVideoDevice::numDataFormats() const
    {
        const DesktopVideoDevice::DesktopVideoFormat& d =
            m_videoFormats[m_videoFormatIndex];
        return m_dataFormats.size() + (maybeFramePacked(d) ? 1 : 0);
    }

    DesktopVideoDevice::DataFormat
    DesktopVideoDevice::dataFormatAtIndex(size_t i) const
    {
        const DesktopVideoDevice::DesktopVideoFormat& d =
            m_videoFormats[m_videoFormatIndex];

        if (maybeFramePacked(d) && i == m_dataFormats.size())
        {
            ostringstream str;
            str << "RGB8";
            str << " HDMI 1.4a Frame Packed Stereo";

            return DesktopDataFormat(str.str(), FramePacked);
        }
        else
        {
            if (i >= m_dataFormats.size())
                i = m_dataFormats.size() - 1;
            return m_dataFormats[i];
        }
    }

    void DesktopVideoDevice::setDataFormat(size_t i)
    {
        const DesktopVideoDevice::DesktopVideoFormat& d =
            m_videoFormats[m_videoFormatIndex];

        if (maybeFramePacked(d) && i == m_dataFormats.size())
        {
            // nothing
            m_dataFormatIndex = i;
            m_stereoMode = FramePacked;
        }
        else
        {
            if (i >= m_dataFormats.size())
                i = m_dataFormats.size() - 1;
            m_dataFormatIndex = i;
            m_stereoMode = m_dataFormats[m_dataFormatIndex].stereoMode;
        }
    }

    size_t DesktopVideoDevice::currentDataFormat() const
    {
        return m_dataFormatIndex;
    }

    size_t DesktopVideoDevice::numSyncModes() const { return 2; }

    DesktopVideoDevice::SyncMode
    DesktopVideoDevice::syncModeAtIndex(size_t i) const
    {
        if (i == 0)
            return SyncMode("No Sync");
        else
            return SyncMode("Vertical Sync");
    }

    void DesktopVideoDevice::setSyncMode(size_t i) { m_vsync = i != 0; }

    size_t DesktopVideoDevice::currentSyncMode() const
    {
        return m_vsync ? 1 : 0;
    }

    size_t DesktopVideoDevice::numSyncSources() const { return 0; }

    DesktopVideoDevice::SyncSource
    DesktopVideoDevice::syncSourceAtIndex(size_t) const
    {
        return SyncSource();
    }

    void DesktopVideoDevice::setSyncSource(size_t) {}

    size_t DesktopVideoDevice::currentSyncSource() const { return 0; }

    void DesktopVideoDevice::addDataFormatAtDepth(size_t depth,
                                                  DesktopStereoMode m)
    {
        ostringstream str;
        str << "RGB";
        if (depth)
            str << depth;
        const char* d = "";

        switch (m)
        {
        case Mono:
            d = "";
            break;
        case QuadBufferStereo:
            str << " Quad Buffer Stereo";
            break;
        case SideBySideStereo:
            str << " HDMI 1.4a Side-by-Side Stereo";
            break;
        case TopBottomStereo:
            str << " HDMI 1.4a Top-and-Bottom Stereo";
            break;
        case ScanlineStereo:
            str << " Scanline Stereo";
            break;
        case CheckerStereo:
            str << " Checker Board Stereo";
            break;
        default:
            str << " DEFAULT";
            break;
        }

        m_dataFormats.push_back(DesktopDataFormat(str.str(), m));
    }

    void DesktopVideoDevice::addDefaultDataFormats(size_t depth)
    {
        addDataFormatAtDepth(depth, Mono);
        addDataFormatAtDepth(depth, QuadBufferStereo);
        addDataFormatAtDepth(depth, SideBySideStereo);
        addDataFormatAtDepth(depth, TopBottomStereo);
        addDataFormatAtDepth(depth, ScanlineStereo);
        addDataFormatAtDepth(depth, CheckerStereo);
    }

    namespace
    {

        bool widthSort(const TwkApp::VideoDevice::VideoFormat& a,
                       const TwkApp::VideoDevice::VideoFormat& b)
        {
            if (a.width != b.width)
                return a.width < b.width;
            else if (a.height != b.height)
                return a.height < b.height;
            else if (a.hz != b.hz)
                return a.hz < b.hz;
            else
                return a.description.compare(b.description) < 0;
        }

    } // namespace

    void DesktopVideoDevice::sortVideoFormatsByWidth()
    {
        sort(m_videoFormats.begin(), m_videoFormats.end(), widthSort);
    }

    DesktopVideoDevice::ScreenView::ScreenView(const QSurfaceFormat& fmt,
                                               QWidget* parent,
                                               QOpenGLWidget* glViewShare,
                                               Qt::WindowFlags flags)
        : QOpenGLWidget(parent, flags)
    {
        m_glViewShare = glViewShare;
        setFormat(fmt);

        // Important: set PartialUpdate, because otherwise
        // before every call to paintGL Qt will call glClear(),
        // thereby erasing the FBO we just transferred pixels to.
        setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    }

    void DesktopVideoDevice::ScreenView::initializeGL()
    {
        QOpenGLWidget::initializeGL();

        if (m_glViewShare && context() && context()->isValid())
        {
            context()->setShareContext(m_glViewShare->context());
        }
    }

    void DesktopVideoDevice::ScreenView::paintGL()
    {
        // This method is explicitely empty because this widget's FBO is
        // written to by the transfer/transfer2() method
    }

    //----------------------------------------------------------------------

    bool DesktopVideoDevice::isOpen() const { return m_view != 0; }

    bool DesktopVideoDevice::useFullScreen() const
    {
#ifdef DEBUG_NO_FULLSCREEN
        return false;
#else
        return true;
#endif
    }

    QRect DesktopVideoDevice::screenGeometry() const
    {
        const QList<QScreen*> screens = QGuiApplication::screens();
        QRect g = screens[m_screen]->geometry();

#ifdef DEBUG_NO_FULLSCREEN
        // in the no-fullscreen debug mode, use 800x600 resolution in a simple
        // window.
        int offset = 30;
        g = QRect(g.x() + offset, g.y() + offset, 1024, 768);
#endif
        return g;
    }

    VideoDevice::Resolution DesktopVideoDevice::resolution() const
    {
        QRect g = screenGeometry();

        return Resolution(g.width(), g.height(), 1.0, 1.0);
    }

    size_t DesktopVideoDevice::width() const
    {
        QRect g = screenGeometry();
        return g.width();
    }

    size_t DesktopVideoDevice::height() const
    {
        QRect g = screenGeometry();
        return g.height();
    }

    VideoDevice::VideoFormat DesktopVideoDevice::format() const
    {
        QRect g = screenGeometry();
        ostringstream str;
        str << g.width() << " x " << g.height();
        const float pa = 1.0;
        const float ps = 1.0;
        const float rate = 60.0;
        return VideoFormat(g.width(), g.height(), pa, ps, rate, str.str());
    }

    VideoDevice::Offset DesktopVideoDevice::offset() const
    {
        return Offset(0, 0);
    }

    VideoDevice::Timing DesktopVideoDevice::timing() const { return format(); }

#ifdef PLATFORM_WINDOWS
    TwkApp::VideoDevice::ColorProfile DesktopVideoDevice::colorProfile() const
    {
        //
        // XXX The following steps are not Unicode safe
        //

        // Get the context for this screen
        const QList<QScreen*> screens = QGuiApplication::screens();

        // Ensure the screen index is valid.
        if (m_screen < 0 || m_screen >= screens.size())
        {
            m_colorProfile = ColorProfile();
            return m_colorProfile;
        }

        QScreen* targetScreen = screens[m_screen];
        QWindow* windowOnTargetScreen = nullptr;
        const QList<QWindow*> windows = QGuiApplication::topLevelWindows();

        // Check all windows to find one on the target screen.
        for (QWindow* window : windows)
        {
            if (window->screen() == targetScreen)
            {
                windowOnTargetScreen = window;
            }
        }

        if (!windowOnTargetScreen)
        {
            // Return empty profile if no window is found on the screen.
            m_colorProfile = ColorProfile();
            return m_colorProfile;
        }

        HWND hwnd = reinterpret_cast<HWND>(windowOnTargetScreen->winId());
        HDC hdc = GetDC(hwnd);

        if (hdc)
        {
            // Look for the profile path
            unsigned long pathLen;
            GetICMProfile(hdc, &pathLen, NULL);
            char* path = new char[pathLen];

            if (GetICMProfile(hdc, &pathLen, path))
            {
                // If we found a profile lets set the type,
                // url, and description

                m_colorProfile.type = ICCProfile;

                unsigned long maxLen = 2084;
                char* url = new char[maxLen];
                UrlCreateFromPath(path, url, &maxLen, NULL);
                m_colorProfile.url = url;

                char desc[256];
                cmsHPROFILE profile = cmsOpenProfileFromFile(path, "r");
                cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", "US",
                                       desc, 256);
                m_colorProfile.description = desc;

                delete url;
            }

            delete path;
            ReleaseDC(hwnd, hdc);
        }
        else
        {
            m_colorProfile = ColorProfile();
        }

        return m_colorProfile;
    }
#endif

    std::vector<VideoDevice*> DesktopVideoDevice::createDesktopVideoDevices(
        TwkApp::VideoModule* module, const QTGLVideoDevice* shareDevice)
    {
        std::vector<VideoDevice*> devices;

        const auto screens = QGuiApplication::screens();
        for (int screen = 0; screen < screens.size(); screen++)
        {
            const QScreen* w = screens[screen];
            QString name = QString("%1 %2 %3")
                               .arg(w->manufacturer())
                               .arg(w->model())
                               .arg(w->name());

            if (name.trimmed().isEmpty()) {
                name = QString("Screen %1").arg(screen);
            }

            DesktopVideoDevice* sd = new DesktopVideoDevice(
                module, name.toUtf8().constData(), screen, shareDevice);

            devices.push_back(sd);
        }

        return devices;
    }

    void DesktopVideoDevice::syncBuffers() const
    {
        redrawImmediately(); //?
    }

} // namespace Rv
