//
//  Copyright (c) 2025 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkGLF__GLContextScope__h__
#define __TwkGLF__GLContextScope__h__

class QOpenGLContext;

namespace TwkGLF
{
    class GLVideoDevice;

    //
    //  GLContextScope
    //
    //  Guarantees that a GL context is current for the lifetime of the scope,
    //  so that code which destroys GL objects actually destroys them.
    //
    //  glDeleteFramebuffers() and friends are silent no-ops with no context
    //  current: the C++ object goes away, the driver's object does not, and
    //  nothing says so. Teardown paths are where this bites, because they are
    //  reached from destructors and from event callbacks rather than from
    //  inside a render, so nothing has arranged a context for them. Open one of
    //  these at the top of any scope that deletes GL objects and the question
    //  stops being the caller's problem.
    //
    //  Construction resolves a context in this order:
    //
    //    1. A context is already current -- do nothing. This is the common
    //       case and costs a pointer compare, so the scope is safe to put on
    //       paths that also run mid-render.
    //    2. A GLVideoDevice was supplied -- makeCurrent() on it. Prefer this
    //       where the caller knows its device; it is cheaper than 3 and it is
    //       the context the objects were most likely created under.
    //    3. Otherwise -- a process-lifetime fallback context in RV's global
    //       share group. This is what makes the scope work in destructors that
    //       have already had their device pointers cleared out from under them.
    //
    //  Destruction puts back what was current before, which -- because the
    //  scope only acquires when nothing was current -- means making nothing
    //  current again.
    //
    //  If no context can be resolved, the scope reports once and does nothing.
    //  It never throws and never aborts: a teardown helper must not be the
    //  reason the process fails to exit. Callers that need to know can ask
    //  hasContext().
    //
    class GLContextScope
    {
    public:
        explicit GLContextScope(const GLVideoDevice* device = nullptr);
        ~GLContextScope();

        GLContextScope(const GLContextScope&) = delete;
        GLContextScope& operator=(const GLContextScope&) = delete;

        //
        //  Is a context current for the body of this scope? False only when
        //  none could be resolved, in which case GL work here will not land.
        //
        bool hasContext() const { return m_hasContext; }

    private:
        bool m_acquired;  // did this scope make something current?
        bool m_hasContext;
    };

} // namespace TwkGLF

#endif // __TwkGLF__GLContextScope__h__
