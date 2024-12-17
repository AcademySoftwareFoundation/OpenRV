//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkGLF__GLFence__h__
#define __TwkGLF__GLFence__h__
#include <iostream>

namespace TwkGLF
{

    struct GLFenceInternal;

    //
    //  A wrapper around GL_APPLE_fence, GL_NV_fence, and GL_ARB_sync
    //  until everybody uses GL 3.2
    //
    //  To use:
    //
    //  GLFence* fence = new GLFence(); // initializes fence
    //  fence->set();                   // inject fence into GL command stream
    //  fence->wait();                  // block until fence completes (finish)
    //  delete fence;                   // delete fence object
    //
    //  This is a once shot deal: you need to make a new GLFence, you can't
    //  reuse it.
    //

    class GLFence
    {
    public:
        GLFence();
        ~GLFence();

        void set() const;
        void wait(bool client = true) const;

    private:
        GLFenceInternal* m_imp;
    };

} // namespace TwkGLF

#endif // __TwkGLF__GLFence__h__
