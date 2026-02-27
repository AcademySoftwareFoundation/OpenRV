//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#import <TwkMTLF/MetalProgram.h>

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <cassert>
#include <sstream>
#include <stdexcept>

namespace TwkMTLF
{

// ---------------------------------------------------------------------------
//  Static cache
// ---------------------------------------------------------------------------

MetalProgram::ProgramCache MetalProgram::s_programCache;

// ---------------------------------------------------------------------------
//  Construction / destruction
// ---------------------------------------------------------------------------

MetalProgram::MetalProgram(const std::string& vertexMSL,
                           const std::string& fragmentMSL)
    : m_vertexMSL(vertexMSL)
    , m_fragmentMSL(fragmentMSL)
    , m_lastError()
    , m_compiled(false)
    , m_vertexLibrary(nil)
    , m_fragmentLibrary(nil)
    , m_vertexFunction(nil)
    , m_fragmentFunction(nil)
    , m_pipelineState(nil)
{
    // Build a stable identifier from a hash of the source strings.
    std::ostringstream oss;
    oss << "MetalProgram("
        << std::hash<std::string>{}(vertexMSL)
        << ":"
        << std::hash<std::string>{}(fragmentMSL)
        << ")";
    m_identifier = oss.str();
}

MetalProgram::~MetalProgram()
{
    m_pipelineState   = nil;
    m_vertexFunction  = nil;
    m_fragmentFunction = nil;
    m_vertexLibrary   = nil;
    m_fragmentLibrary = nil;
}

// ---------------------------------------------------------------------------
//  Compilation
// ---------------------------------------------------------------------------

bool MetalProgram::compile(id<MTLDevice> device,
                           MTLPixelFormat colorFormat,
                           MTLPixelFormat depthFormat,
                           const std::string& vertexEntry,
                           const std::string& fragmentEntry)
{
    assert(device != nil);

    @autoreleasepool
    {
        m_compiled    = false;
        m_lastError.clear();
        m_pipelineState    = nil;
        m_vertexFunction   = nil;
        m_fragmentFunction = nil;
        m_vertexLibrary    = nil;
        m_fragmentLibrary  = nil;

        // ---- Compile vertex library ----------------------------------------

        NSError* err = nil;

        NSString* vertSrc =
            [NSString stringWithUTF8String:m_vertexMSL.c_str()];

        MTLCompileOptions* opts = [MTLCompileOptions new];
        opts.languageVersion = MTLLanguageVersion3_0;

        m_vertexLibrary =
            [device newLibraryWithSource:vertSrc options:opts error:&err];

        if (!m_vertexLibrary)
        {
            m_lastError = "Vertex shader compilation failed: ";
            if (err)
                m_lastError += err.localizedDescription.UTF8String;
            return false;
        }

        // ---- Compile fragment library ----------------------------------------

        NSString* fragSrc =
            [NSString stringWithUTF8String:m_fragmentMSL.c_str()];

        err = nil;
        m_fragmentLibrary =
            [device newLibraryWithSource:fragSrc options:opts error:&err];

        if (!m_fragmentLibrary)
        {
            m_lastError = "Fragment shader compilation failed: ";
            if (err)
                m_lastError += err.localizedDescription.UTF8String;
            m_vertexLibrary = nil;
            return false;
        }

        // ---- Retrieve entry-point functions ----------------------------------

        NSString* vertEntry =
            [NSString stringWithUTF8String:vertexEntry.c_str()];
        m_vertexFunction = [m_vertexLibrary newFunctionWithName:vertEntry];
        if (!m_vertexFunction)
        {
            m_lastError  = "Vertex entry point '";
            m_lastError += vertexEntry;
            m_lastError += "' not found in MSL source.";
            return false;
        }

        NSString* fragEntry =
            [NSString stringWithUTF8String:fragmentEntry.c_str()];
        m_fragmentFunction = [m_fragmentLibrary newFunctionWithName:fragEntry];
        if (!m_fragmentFunction)
        {
            m_lastError  = "Fragment entry point '";
            m_lastError += fragmentEntry;
            m_lastError += "' not found in MSL source.";
            return false;
        }

        // ---- Build render pipeline state -------------------------------------

        MTLRenderPipelineDescriptor* pipelineDesc =
            [MTLRenderPipelineDescriptor new];
        pipelineDesc.label               = [NSString stringWithUTF8String:m_identifier.c_str()];
        pipelineDesc.vertexFunction      = m_vertexFunction;
        pipelineDesc.fragmentFunction    = m_fragmentFunction;

        pipelineDesc.colorAttachments[0].pixelFormat = colorFormat;

        // Standard pre-multiplied-alpha blending; callers can rebuild the PSO
        // with different settings by subclassing / creating a new MetalProgram.
        pipelineDesc.colorAttachments[0].blendingEnabled             = YES;
        pipelineDesc.colorAttachments[0].rgbBlendOperation           = MTLBlendOperationAdd;
        pipelineDesc.colorAttachments[0].alphaBlendOperation         = MTLBlendOperationAdd;
        pipelineDesc.colorAttachments[0].sourceRGBBlendFactor        = MTLBlendFactorOne;
        pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorOne;
        pipelineDesc.colorAttachments[0].destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
        pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

        if (depthFormat != MTLPixelFormatInvalid)
        {
            pipelineDesc.depthAttachmentPixelFormat   = depthFormat;
            // Depth32Float_Stencil8 has a stencil component.
            if (depthFormat == MTLPixelFormatDepth32Float_Stencil8)
            {
                pipelineDesc.stencilAttachmentPixelFormat = depthFormat;
            }
        }

        err = nil;
        m_pipelineState =
            [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&err];

        if (!m_pipelineState)
        {
            m_lastError = "Pipeline state creation failed: ";
            if (err)
                m_lastError += err.localizedDescription.UTF8String;
            m_vertexFunction   = nil;
            m_fragmentFunction = nil;
            m_vertexLibrary    = nil;
            m_fragmentLibrary  = nil;
            return false;
        }

        m_compiled = true;
        return true;
    }
}

// ---------------------------------------------------------------------------
//  Use / binding
// ---------------------------------------------------------------------------

void MetalProgram::use(id<MTLRenderCommandEncoder> encoder) const
{
    assert(m_compiled && "MetalProgram::use called on uncompiled program");
    assert(encoder != nil);

    [encoder setRenderPipelineState:m_pipelineState];
}

// ---------------------------------------------------------------------------
//  Static program cache
// ---------------------------------------------------------------------------

MetalProgram*
MetalProgram::select(const std::string& vertexMSL,
                     const std::string& fragmentMSL,
                     id<MTLDevice> device,
                     MTLPixelFormat colorFormat,
                     MTLPixelFormat depthFormat,
                     const std::string& vertexEntry,
                     const std::string& fragmentEntry)
{
    CacheKey key;
    key.combined = vertexMSL + "\n---\n" + fragmentMSL;

    auto it = s_programCache.find(key);
    if (it != s_programCache.end())
    {
        return it->second;
    }

    // Not cached — compile a new program.
    MetalProgram* prog = new MetalProgram(vertexMSL, fragmentMSL);

    bool ok = prog->compile(device, colorFormat, depthFormat,
                            vertexEntry, fragmentEntry);
    if (!ok)
    {
        // Log the error but don't throw; caller checks isCompiled() / lastError().
        delete prog;
        return nullptr;
    }

    s_programCache[key] = prog;
    return prog;
}

void MetalProgram::clearCache()
{
    for (auto& kv : s_programCache)
    {
        delete kv.second;
    }
    s_programCache.clear();
}

} // namespace TwkMTLF
