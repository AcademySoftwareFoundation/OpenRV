//
//  Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkGLF/GLContextScope.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLVideoDevice.h>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QThread>

#include <iostream>

namespace TwkGLF
{

    namespace
    {

        //
        //  The fallback teardown context. Intentionally leaked: it is needed
        //  while the application is being torn down, so there is no safe
        //  point to free it.
        //
        QOpenGLContext* s_fallbackContext = nullptr;
        QOffscreenSurface* s_fallbackSurface = nullptr;
        bool s_fallbackAttempted = false;

        void reportNoContext(const char* why)
        {
            static bool reported = false;

            if (!reported)
            {
                reported = true;
                std::cerr << "WARNING: no GL context available for teardown (" << why
                          << "); GL objects destroyed without one will leak in the driver" << std::endl;
            }
        }

        //
        //  Build the fallback context, once.
        //
        bool createFallbackContext()
        {
            s_fallbackAttempted = true;

            //
            //  Must share with the global group: deleting a name from outside
            //  its share group silently does nothing.
            //
            QOpenGLContext* share = QOpenGLContext::globalShareContext();

            if (!share)
            {
                reportNoContext("no global share context; Qt::AA_ShareOpenGLContexts is not set");
                return false;
            }

            QOpenGLContext* context = new QOpenGLContext;
            context->setShareContext(share);
            context->setFormat(share->format());

            if (!context->create() || !context->shareContext())
            {
                delete context;
                reportNoContext("shared context creation failed");
                return false;
            }

            QOffscreenSurface* surface = new QOffscreenSurface;
            surface->setFormat(context->format());
            surface->create();

            if (!surface->isValid())
            {
                delete surface;
                delete context;
                reportNoContext("offscreen surface creation failed");
                return false;
            }

            s_fallbackContext = context;
            s_fallbackSurface = surface;

            return true;
        }

        bool makeFallbackCurrent()
        {
            //
            //  QOffscreenSurface::create() is GUI-thread only.
            //
            QCoreApplication* app = QCoreApplication::instance();

            if (!qobject_cast<QGuiApplication*>(app))
            {
                reportNoContext("no QGuiApplication");
                return false;
            }

            if (QThread::currentThread() != app->thread())
            {
                reportNoContext("not on the GUI thread");
                return false;
            }

            if (!s_fallbackAttempted && !createFallbackContext())
            {
                return false;
            }

            if (!s_fallbackContext)
            {
                reportNoContext("fallback context unavailable");
                return false;
            }

            if (!s_fallbackContext->makeCurrent(s_fallbackSurface))
            {
                reportNoContext("fallback context could not be made current");
                return false;
            }

            return true;
        }

    } // namespace

    GLContextScope::GLContextScope(const GLVideoDevice* device)
        : m_acquired(false)
        , m_hasContext(false)
    {
        //
        //  Never displace a live context, including a natively bound one.
        //
        if (twkGlAnyContextIsCurrent())
        {
            m_hasContext = true;
            return;
        }

        if (device)
        {
            device->makeCurrent();

            if (twkGlAnyContextIsCurrent())
            {
                m_acquired = true;
                m_hasContext = true;
                return;
            }
        }

        if (makeFallbackCurrent())
        {
            m_acquired = true;
            m_hasContext = true;
        }
    }

    GLContextScope::~GLContextScope()
    {
        if (!m_acquired)
        {
            return;
        }

        //
        //  Nothing was current on entry. A context bound natively by the
        //  device is invisible to Qt and is left to the device.
        //
        if (QOpenGLContext* current = QOpenGLContext::currentContext())
        {
            current->doneCurrent();
        }
    }

} // namespace TwkGLF
