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
        //  The fallback teardown context.
        //
        //  Created once and never destroyed. The paths that need it run while
        //  the application object is itself being torn down, so anything that
        //  freed this at static-destruction time would free it either too
        //  early to be useful or after QGuiApplication has already gone. One
        //  leaked context at process exit costs nothing; getting that ordering
        //  wrong costs a crash on the way out, which is the class of bug this
        //  exists to remove.
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
        //  Build the fallback context, once. Returns false -- quietly after
        //  the first time -- if it cannot be had.
        //
        bool createFallbackContext()
        {
            s_fallbackAttempted = true;

            //
            //  Insist on the global share group. GL names live in a share
            //  group, so deleting an FBO under a context outside the group
            //  that created it is not an error -- it simply does nothing,
            //  which is the exact silent leak this class is meant to stop. A
            //  non-sharing fallback would look like a fix and behave like the
            //  bug.
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
            //  QOpenGLContext is thread-affine and QOffscreenSurface::create()
            //  is GUI-thread only, so this fallback serves the GUI thread. That
            //  is where teardown runs. Anywhere else, decline rather than
            //  silently misbehave.
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
        //  Already current -- including a context bound natively rather than
        //  through Qt -- so leave it alone. Displacing a live context from
        //  inside a destructor would be far worse than the problem this scope
        //  solves.
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
        //  Nothing was current on entry -- that is the only state in which
        //  this scope acquires -- so restoring means making nothing current.
        //
        //  A context bound natively by the device cannot be released this way;
        //  Qt does not know about it. That is acceptable: it belongs to the
        //  device, which will bind or release it on its own terms.
        //
        if (QOpenGLContext* current = QOpenGLContext::currentContext())
        {
            current->doneCurrent();
        }
    }

} // namespace TwkGLF
