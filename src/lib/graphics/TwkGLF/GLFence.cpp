//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkGLF/GL.h>
#include <TwkUtil/Timer.h>
#include <TwkGLF/GLFence.h>

namespace TwkGLF
{
    using namespace std;
    using namespace TwkUtil;

    static bool init = false;
    static bool hasARB = false;
    static bool hasAPPLE = false;
    static bool hasNV = false;

#ifdef VM_NO_GL
#undef GL_VERSION_3_2
#undef GL_ARB_sync
#undef GL_NV_fence
#endif

#if defined(GL_ARB_sync) || defined(GL_VERSION_3_2) || defined(glFenceSync)
#define HAVE_ARB_SYNC_API
#else
    typedef GLuint GLsync;
#endif

#if defined(GL_APPLE_fence) && defined(PLATFORM_DARWIN)
#define HAVE_APPLE_FENCE_API
#endif

#if defined(GL_NV_fence)
#define HAVE_NV_FENCE_API
#endif

    struct GLFenceInternal
    {
        GLsync arbSync;
        GLuint otherSync;
    };

    GLFence::GLFence()
    {
        if (!init)
        {
            init = true;
            hasARB = TWK_GL_SUPPORTS("GL_ARB_sync");
            hasAPPLE = TWK_GL_SUPPORTS("GL_APPLE_fence");
            hasNV = TWK_GL_SUPPORTS("GL_NV_fence");

#if !defined(HAVE_ARB_SYNC_API)
            hasARB = 0;
#endif

#if !defined(HAVE_APPLE_FENCE_API)
            hasAPPLE = 0;
#endif

#if !defined(HAVE_NV_FENCE_API)
            hasNV = 0;
#endif
        }

        m_imp = new GLFenceInternal;

        if (hasARB)
        {
#if defined(HAVE_ARB_SYNC_API)
            m_imp->arbSync = 0;
#endif
        }
        else if (hasAPPLE)
        {
#if defined(HAVE_APPLE_FENCE_API)
            glGenFencesAPPLE(1, &m_imp->otherSync);
#endif
        }
        else if (hasNV)
        {
#if defined(HAVE_NV_FENCE_API)
            glGenFencesNV(1, &m_imp->otherSync);
#endif
        }
    }

    GLFence::~GLFence()
    {
        if (hasARB)
        {
#if defined(HAVE_ARB_SYNC_API)
            if (m_imp->arbSync && glIsSync(m_imp->arbSync))
                glDeleteSync(m_imp->arbSync);
#endif
        }
        else if (hasAPPLE)
        {
#if defined(HAVE_APPLE_FENCE_API)
            glDeleteFencesAPPLE(1, &m_imp->otherSync);
#endif
        }
        else if (hasNV)
        {
#if defined(HAVE_NV_FENCE_API)
            glDeleteFencesNV(1, &m_imp->otherSync);
#endif
        }

        delete m_imp;
        m_imp = 0;
    }

    void GLFence::set() const
    {
        if (hasARB)
        {
#if defined(HAVE_ARB_SYNC_API)
            m_imp->arbSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
#endif
        }
        else if (hasAPPLE)
        {
#if defined(HAVE_APPLE_FENCE_API)
            glSetFenceAPPLE(m_imp->otherSync);
#endif
        }
        else if (hasNV)
        {
#if defined(HAVE_NV_FENCE_API)
            glSetFenceNV(m_imp->otherSync, GL_ALL_COMPLETED_NV);
#endif
        }
    }

    void GLFence::wait(bool client) const
    {
        if (hasARB)
        {
#if defined(HAVE_ARB_SYNC_API)

            if (!glIsSync(m_imp->arbSync))
            {
                //
                //  Some weird behavior with shutdown timing. Just exit if
                //  the fence is no longer a sync object for some reason.
                //

                // cout << "ERROR: GL fence no longer valid: " << m_imp->arbSync
                // << endl;
                return;
            }

            if (client)
            {
                GLenum error = glClientWaitSync(
                    m_imp->arbSync, GL_SYNC_FLUSH_COMMANDS_BIT, 10000000000);
#if 0
            if (error == GL_ALREADY_SIGNALED) fprintf(stderr, "fence already signaled\n");
            else if (error == GL_TIMEOUT_EXPIRED) fprintf(stderr, "fence time expired\n");
            else if (error == GL_CONDITION_SATISFIED) fprintf(stderr, "fence condition satisfied\n");
            else fprintf(stderr, "fence wait failed\n");
#endif
            }
            else
            {
                glWaitSync(m_imp->arbSync, 0, GL_TIMEOUT_IGNORED);
            }
#endif
        }
        else if (hasAPPLE)
        {
#if defined(HAVE_APPLE_FENCE_API)
            glFinishFenceAPPLE(m_imp->otherSync);
#endif
        }
        else if (hasNV)
        {
#if defined(HAVE_NV_FENCE_API)
            glFinishFenceNV(m_imp->otherSync);
#endif
        }
    }

} // namespace TwkGLF
