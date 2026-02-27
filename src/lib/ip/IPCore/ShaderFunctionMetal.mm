//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//  ShaderFunctionMetal.mm
//  Implements Function::compileMetal() for the Metal rendering path.
//
//  This file MUST be compiled as Objective-C++ (.mm) because it includes
//  <Metal/Metal.h>.  Do NOT #import Metal headers from plain .cpp files.
//
//  NOTE: Metal compilation in IPCore happens primarily at the Program level
//  (Program::compileMetal() in ShaderProgramMetal.mm), which generates a full
//  vertex+fragment MSL program from the composed expression tree.
//
//  Function::compileMetal() is reserved for future ahead-of-time compilation
//  of individual shader snippets (e.g. compute pipelines).  For now it is a
//  no-op placeholder.
//

#if defined(PLATFORM_DARWIN) && defined(USE_METAL)

#import <Metal/Metal.h>
#include <IPCore/ShaderFunction.h>

namespace IPCore
{
    namespace Shader
    {

        void Function::compileMetal(void* /* devicePtr */) const
        {
            // Metal compilation for individual Function objects is deferred to
            // Program::compileMetal(), which generates and compiles the complete
            // MSL vertex+fragment program from the composed expression tree.
            //
            // Individual Functions do not compile to standalone MTLFunction objects
            // under the current architecture.  m_metalFunction remains nullptr and
            // is not used by the rendering pipeline.
            m_metalFunction = nullptr;
        }

    } // namespace Shader
} // namespace IPCore

#endif // PLATFORM_DARWIN && USE_METAL
