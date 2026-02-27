//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __TwkMTLF__MetalProgram__h__
#define __TwkMTLF__MetalProgram__h__

#include <string>
#include <map>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#endif

namespace TwkMTLF
{

    //
    //  MetalProgram  —  Metal equivalent of TwkGLF::BasicGLProgram
    //
    //  Wraps:
    //    id<MTLLibrary>              — compiled shader library (vertex + fragment)
    //    id<MTLFunction>             — vertex and fragment shader entry points
    //    id<MTLRenderPipelineState>  — compiled pipeline state object (PSO)
    //
    //  Lifecycle:
    //    1. Construct with MSL source code for both vertex and fragment shaders.
    //    2. Call compile(device, colorPixelFormat, [depthPixelFormat]) to build
    //       the MTLRenderPipelineState.  On error, compile() returns false and
    //       lastError() contains the NSError description.
    //    3. Call use(encoder) to bind the PSO on a render command encoder before
    //       issuing draw calls.
    //
    //  A static program cache (keyed on the combined vertex+fragment source) is
    //  provided via MetalProgram::select(), mirroring BasicGLProgram::select().
    //

    class MetalProgram
    {
    public:
        // -------------------------------------------------------------------------
        //  Construction / destruction
        // -------------------------------------------------------------------------

        MetalProgram(const std::string& vertexMSL, const std::string& fragmentMSL);
        ~MetalProgram();

        // Non-copyable
        MetalProgram(const MetalProgram&) = delete;
        MetalProgram& operator=(const MetalProgram&) = delete;

        // -------------------------------------------------------------------------
        //  Compilation
        // -------------------------------------------------------------------------

        //  Compile the MSL source and build a render pipeline state.
        //  colorFormat    — e.g. MTLPixelFormatBGRA8Unorm_sRGB
        //  depthFormat    — e.g. MTLPixelFormatDepth32Float_Stencil8; pass
        //                   MTLPixelFormatInvalid (0) if no depth attachment.
        //  vertexEntry    — MSL function name for the vertex shader.
        //  fragmentEntry  — MSL function name for the fragment shader.
        //
        //  Returns true on success; false on error (see lastError()).

#ifdef __OBJC__
        bool compile(id<MTLDevice> device, MTLPixelFormat colorFormat, MTLPixelFormat depthFormat = MTLPixelFormatInvalid,
                     const std::string& vertexEntry = "vertexMain", const std::string& fragmentEntry = "fragmentMain");
#endif

        //  Returns the error description from the last failed compile(), or
        //  an empty string if the last compile() succeeded.
        const std::string& lastError() const { return m_lastError; }

        bool isCompiled() const { return m_compiled; }

        // -------------------------------------------------------------------------
        //  Use / binding
        // -------------------------------------------------------------------------

        //  Bind the pipeline state to a render command encoder.
        //  Must be called after beginRenderPass and before draw calls.
#ifdef __OBJC__
        void use(id<MTLRenderCommandEncoder> encoder) const;
#endif

        // -------------------------------------------------------------------------
        //  Accessors
        // -------------------------------------------------------------------------

#ifdef __OBJC__
        id<MTLRenderPipelineState> pipelineState() const { return m_pipelineState; }

        id<MTLFunction> vertexFunction() const { return m_vertexFunction; }

        id<MTLFunction> fragmentFunction() const { return m_fragmentFunction; }
#else
        void* pipelineState() const { return m_pipelineState_opaque; }

        void* vertexFunction() const { return m_vertexFunction_opaque; }

        void* fragmentFunction() const { return m_fragmentFunction_opaque; }
#endif

        const std::string& identifier() const { return m_identifier; }

        // -------------------------------------------------------------------------
        //  Static program cache
        // -------------------------------------------------------------------------

        //  Look up an existing compiled MetalProgram by source, or create and
        //  compile a new one.  The device and pixel formats are used only on first
        //  creation.  Returns nullptr on compile failure.

#ifdef __OBJC__
        static MetalProgram* select(const std::string& vertexMSL, const std::string& fragmentMSL, id<MTLDevice> device,
                                    MTLPixelFormat colorFormat, MTLPixelFormat depthFormat = MTLPixelFormatInvalid,
                                    const std::string& vertexEntry = "vertexMain", const std::string& fragmentEntry = "fragmentMain");
#endif

        //  Remove all cached programs (call on teardown / context loss).
        static void clearCache();

    private:
        struct CacheKey
        {
            std::string combined; // vertexMSL + "\n---\n" + fragmentMSL

            bool operator<(const CacheKey& o) const { return combined < o.combined; }
        };

        using ProgramCache = std::map<CacheKey, MetalProgram*>;
        static ProgramCache s_programCache;

        std::string m_vertexMSL;
        std::string m_fragmentMSL;
        std::string m_identifier;
        std::string m_lastError;
        bool m_compiled;

#ifdef __OBJC__
        id<MTLLibrary> m_vertexLibrary;
        id<MTLLibrary> m_fragmentLibrary;
        id<MTLFunction> m_vertexFunction;
        id<MTLFunction> m_fragmentFunction;
        id<MTLRenderPipelineState> m_pipelineState;
#else
        void* m_vertexLibrary_opaque;
        void* m_fragmentLibrary_opaque;
        void* m_vertexFunction_opaque;
        void* m_fragmentFunction_opaque;
        void* m_pipelineState_opaque;
#endif
    };

} // namespace TwkMTLF

#endif // __TwkMTLF__MetalProgram__h__
