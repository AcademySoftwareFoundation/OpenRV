//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ShaderState__h__
#define __IPCore__ShaderState__h__
#include <iostream>
#include <TwkGLF/GL.h>

namespace IPCore
{
    namespace Shader
    {

        struct FunctionGLState
        {
            FunctionGLState()
                : shader(0)
            {
            }

            GLuint shader;
        };

    } // namespace Shader
} // namespace IPCore

#endif // __IPCore__ShaderState__h__
