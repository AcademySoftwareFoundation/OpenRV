//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkGLFFBO/FBOVideoDevice.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLFBO.h>
#include <TwkExc/Exception.h>
#include <iostream>

#ifdef PLATFORM_LINUX
#include <GL/glx.h>
#endif

#ifdef PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#include <gl/glew.h>
#endif

#ifdef PLATFORM_DARWIN
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLRenderers.h>
#endif

namespace TwkGLF
{
    using namespace std;
    using namespace TwkApp;

    bool FBOVideoDevice::m_swRendererOnMac = false;

#if defined(PLATFORM_DARWIN)
    struct FBOImp
    {
        FBOImp()
            : pfo(0)
            , npfo(0)
            , ctx(0)
        {
        }

        CGLPixelFormatObj pfo;
        GLint npfo;
        CGLContextObj ctx;
    };
#endif

#if defined(PLATFORM_LINUX)
    struct FBOImp
    {
        FBOImp()
            : display(0)
        {
        }

        Display* display;
        Window root;
        XVisualInfo* vis;
        Window tiny;
        GLXContext ctx;
    };
#endif

#if defined(PLATFORM_WINDOWS)
    struct FBOImp
    {
        FBOImp()
            : deviceContext(0)
        {
        }

        // HPBUFFERARB pbuffer;
        HWND window;
        HDC deviceContext;
        HGLRC glContext;
        PIXELFORMATDESCRIPTOR format;
    };
#endif

    FBOVideoDevice::FBOVideoDevice(VideoModule* m, int w, int h, bool alpha,
                                   int numFBOs)
        : GLVideoDevice(m, "fbo-rb", GLVideoDevice::ImageOutput)
        , m_width(w)
        , m_height(h)
        , m_alpha(alpha)
    {
        m_imp = new FBOImp();

        //--------------------------------------------------
        // platform specific stuff

#if defined(PLATFORM_DARWIN)

        unsigned long hwAttrs[] = {
            kCGLPFAAccelerated,
            kCGLPFAColorFloat, /* color buffers store floating point pixels */
            kCGLPFAColorSize,
            3 * 16,
            kCGLPFAAlphaSize,
            1 * 16,
            0};

        unsigned long swAttrs[] = {
            kCGLPFAAllRenderers,
            kCGLPFARendererID,
            kCGLRendererAppleSWID,
            kCGLPFAColorFloat, /* color buffers store floating point pixels */
            kCGLPFAColorSize,
            3 * 32,
            kCGLPFAAlphaSize,
            1 * 32,
            0};

        unsigned long* attrs = (m_swRendererOnMac) ? swAttrs : hwAttrs;

        if (CGLError err = CGLChoosePixelFormat((CGLPixelFormatAttribute*)attrs,
                                                &m_imp->pfo, &m_imp->npfo))
        {
            cout << "ERROR: choosing pixel format: " << CGLErrorString(err)
                 << endl;
            exit(-1);
        }

        if (CGLError err = CGLCreateContext(m_imp->pfo, 0, &m_imp->ctx))
        {
            cout << "ERROR: create context: " << CGLErrorString(err) << endl;
            exit(-1);
        }

        // cout << "DEBUG: made the context" << endl;

        if (CGLError err = CGLSetCurrentContext(m_imp->ctx))
        {
            cout << "ERROR: CGLSetCurrentContext " << CGLErrorString(err)
                 << endl;
            exit(-1);
        }

#endif

#if defined(PLATFORM_LINUX)
        XSetWindowAttributes swa;
        int attrs[] = {GLX_BUFFER_SIZE, 32, GLX_RGBA, 0, GLX_STENCIL_SIZE, 1};

        m_imp->display = XOpenDisplay(0);
        m_imp->root = DefaultRootWindow(m_imp->display);
        m_imp->vis = glXChooseVisual(m_imp->display,
                                     DefaultScreen(m_imp->display), attrs);

        swa.colormap = XCreateColormap(m_imp->display, m_imp->root,
                                       m_imp->vis->visual, AllocNone);

        swa.event_mask = ExposureMask | KeyPressMask;

        m_imp->tiny = XCreateWindow(
            m_imp->display, m_imp->root, 0, 0, 64, 64, 0, m_imp->vis->depth,
            InputOutput, m_imp->vis->visual, CWColormap | CWEventMask, &swa);

        m_imp->ctx = glXCreateContext(m_imp->display, m_imp->vis, 0, True);

        glXMakeCurrent(m_imp->display, m_imp->tiny, m_imp->ctx);
#endif

#if defined(PLATFORM_WINDOWS)

        HINSTANCE hInstance = GetModuleHandle(0);
        assert(hInstance != NULL);
        static const char className[] = "fbopbuffer";
        WNDCLASS wc;

        if (!GetClassInfo(hInstance, className, &wc))
        {
            wc.style = CS_OWNDC;
            wc.lpfnWndProc = DefWindowProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = 0;
            wc.hInstance = hInstance;
            wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = NULL;
            wc.lpszMenuName = NULL;
            wc.lpszClassName = className;

            if (!RegisterClass(&wc))
            {
                int registerClassFailed = 0;
                assert(registerClassFailed == 1);
            }
        }

        m_imp->window =
            CreateWindow(className, 0, 0, 0, 0, 0, 0, 0, 0, hInstance, 0);
        assert(m_imp->window != NULL);

        m_imp->deviceContext = GetDC(m_imp->window);
        assert(m_imp->deviceContext != NULL);

        // m_imp->pbuffer = wglCreatePbufferARB()

        m_imp->format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        m_imp->format.nVersion = 1;
        m_imp->format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL
                                | PFD_DOUBLEBUFFER | PFD_GENERIC_ACCELERATED;
        m_imp->format.iPixelType = PFD_TYPE_RGBA;
        m_imp->format.cColorBits = 32;
        m_imp->format.cRedBits = 8;
        m_imp->format.cRedShift = 0;
        m_imp->format.cGreenBits = 8;
        m_imp->format.cGreenShift = 0;
        m_imp->format.cBlueBits = 8;
        m_imp->format.cBlueShift = 0;
        m_imp->format.cAlphaBits = 8;
        m_imp->format.cAlphaShift = 0;
        m_imp->format.cAccumBits = 0;
        m_imp->format.cAccumRedBits = 0;
        m_imp->format.cAccumGreenBits = 0;
        m_imp->format.cAccumBlueBits = 0;
        m_imp->format.cAccumAlphaBits = 0;
        m_imp->format.cDepthBits = 0;
        m_imp->format.cStencilBits = 8;
        m_imp->format.cAuxBuffers = 0;
        m_imp->format.iLayerType = PFD_MAIN_PLANE;
        m_imp->format.bReserved = 0;
        m_imp->format.dwLayerMask = 0;
        m_imp->format.dwVisibleMask = 0;
        m_imp->format.dwDamageMask = 0;

        int result = ChoosePixelFormat(m_imp->deviceContext, &m_imp->format);
        SetPixelFormat(m_imp->deviceContext, result, &m_imp->format);

        m_imp->glContext = wglCreateContext(m_imp->deviceContext);
        assert(m_imp->glContext != NULL);

        wglMakeCurrent(m_imp->deviceContext, m_imp->glContext);
        assert(wglGetCurrentContext() != NULL);
        glewInit(NULL);
#endif

        for (int i = 0; i < numFBOs; ++i)
        {
            m_fbos.push_back(
                new GLFBO(w, h, m_alpha ? GL_RGBA16F_ARB : GL_RGB16F_ARB));
            m_fbos.back()->newColorRenderBuffer();
        }
        setDefaultFBOIndex(0);
    }

    FBOVideoDevice::~FBOVideoDevice()
    {
        makeCurrent();

        if (m_imp)
        {
#if defined(PLATFORM_DARWIN)
            CGLDestroyContext(m_imp->ctx);
#endif

#if defined(PLATFORM_LINUX)
            glXMakeCurrent(m_imp->display, None, 0);
            glXDestroyContext(m_imp->display, m_imp->ctx);
            XDestroyWindow(m_imp->display, m_imp->tiny);
            XCloseDisplay(m_imp->display);
#endif
        }

        delete m_imp;
    }

    size_t FBOVideoDevice::width() const { return m_width; }

    size_t FBOVideoDevice::height() const { return m_height; }

    void FBOVideoDevice::redraw() const {}

    void FBOVideoDevice::bind() const { defaultFBO()->bind(); }

    void FBOVideoDevice::unbind() const { defaultFBO()->unbind(); }

    GLFBO* FBOVideoDevice::defaultFBO() { return m_fbo; }

    const GLFBO* FBOVideoDevice::defaultFBO() const { return m_fbo; }

    void FBOVideoDevice::makeCurrent() const
    {
#ifdef PLATFORM_DARWIN
        CGLSetCurrentContext(m_imp->ctx);
#endif

#ifdef PLATFORM_WINDOWS
        wglMakeCurrent(m_imp->deviceContext, m_imp->glContext);
#endif

#ifdef PLATFORM_LINUX
        glXMakeCurrent(m_imp->display, m_imp->tiny, m_imp->ctx);
#endif

        bind();

        GLVideoDevice::makeCurrent();
    }

    std::string FBOVideoDevice::hardwareIdentification() const
    {
        return "fbo-rb";
    }

    //----------------------------------------------------------------------

} // namespace TwkGLF
