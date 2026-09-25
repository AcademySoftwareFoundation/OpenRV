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
    //  so that GL deletes in teardown paths are not silent no-ops.
    //
    //  Construction resolves a context in this order:
    //
    //    1. A context is already current: do nothing (a pointer compare).
    //    2. A GLVideoDevice was supplied: makeCurrent() on it.
    //    3. Otherwise: a process-lifetime fallback context in RV's global
    //       share group.
    //
    //  Destruction makes nothing current again if this scope acquired. If no
    //  context can be resolved, the scope reports once and does nothing; it
    //  never throws. Check hasContext() if needed.
    //
    class GLContextScope
    {
    public:
        explicit GLContextScope(const GLVideoDevice* device = nullptr);
        ~GLContextScope();

        GLContextScope(const GLContextScope&) = delete;
        GLContextScope& operator=(const GLContextScope&) = delete;

        bool hasContext() const { return m_hasContext; }

    private:
        bool m_acquired; // did this scope make something current?
        bool m_hasContext;
    };

} // namespace TwkGLF

#endif // __TwkGLF__GLContextScope__h__
