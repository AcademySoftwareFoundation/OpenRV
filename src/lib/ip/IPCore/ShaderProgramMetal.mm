//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//  ShaderProgramMetal.mm
//  Implements Program::compileMetal() for the Metal rendering path.
//
//  This file MUST be compiled as Objective-C++ (.mm) because it includes
//  <Metal/Metal.h>.  Do NOT #import Metal headers from plain .cpp files.
//

#if defined(PLATFORM_DARWIN) && defined(USE_METAL)

#import <Metal/Metal.h>
#include <IPCore/ShaderProgram.h>
#include <IPCore/MetalContext.h>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <iostream>

namespace IPCore
{
    namespace Shader
    {

        namespace
        {
            //
            // glslToMSL — simple syntactic transformation of IPCore-generated GLSL
            // into a valid MSL fragment shader.  This covers the patterns produced
            // by Program::compile() / outputLocalFunction(); it is NOT a general-
            // purpose GLSL→MSL transpiler.
            //
            // Key substitutions:
            //   uniform sampler2DRect N  →  texture2d<float> N [[texture(i)]]
            //                              + sampler N_samp [[sampler(i)]]
            //   texture2DRect(N, coord)  →  N.sample(N_samp, coord)
            //   gl_FragColor / FragColor →  [[color(0)]] output struct member
            //   varying in vec2          →  struct member passed from vertex stage
            //   uniform vec2/float/…     →  constant-buffer members
            //
            std::string glslToMSL(const std::string& glsl,
                                  const std::string& fragmentMainBody)
            {
                //
                // Build the MSL source in three sections:
                //   1. Standard pass-through vertex shader (constant across all programs)
                //   2. Fragment-shader uniform struct  (one entry per GLSL uniform)
                //   3. Fragment entry point that calls the body
                //
                // We do a line-by-line scan of the GLSL to collect information and
                // emit MSL equivalents.
                //

                std::ostringstream msl;
                msl << "#include <metal_stdlib>\n"
                    << "using namespace metal;\n\n";

                // ---- Vertex shader ----
                msl << "struct VertexOut {\n"
                    << "    float4 position [[position]];\n"
                    << "    float2 texCoord;\n"
                    << "};\n\n"
                    << "vertex VertexOut vertexMain(uint vertexID [[vertex_id]]) {\n"
                    << "    float2 positions[3] = { float2(-1,-1), float2(3,-1), float2(-1,3) };\n"
                    << "    VertexOut out;\n"
                    << "    out.position = float4(positions[vertexID], 0, 1);\n"
                    << "    out.texCoord = (positions[vertexID] + 1.0) * 0.5;\n"
                    << "    return out;\n"
                    << "}\n\n";

                // ---- Collect uniforms from the GLSL preamble ----
                std::istringstream stream(glsl);
                std::string line;

                // Uniform buffer struct members
                std::ostringstream uniformStructMembers;
                // Texture/sampler bindings
                std::ostringstream textureBindings;
                // Replacement body (GLSL → MSL syntax)
                std::string body = fragmentMainBody;

                int textureIndex = 0;

                while (std::getline(stream, line))
                {
                    // Skip blank lines and comments
                    std::string trimmed = line;
                    boost::algorithm::trim(trimmed);
                    if (trimmed.empty() || trimmed.substr(0, 2) == "//")
                        continue;

                    // uniform sampler2DRect <name>;
                    if (trimmed.find("uniform sampler2DRect") != std::string::npos)
                    {
                        std::string texName = trimmed;
                        boost::algorithm::replace_all(texName, "uniform sampler2DRect", "");
                        boost::algorithm::replace_all(texName, ";", "");
                        boost::algorithm::trim(texName);

                        // Emit texture+sampler binding
                        textureBindings << "    texture2d<float> " << texName
                                        << " [[texture(" << textureIndex << ")]],\n"
                                        << "    sampler " << texName << "_samp"
                                        << " [[sampler(" << textureIndex << ")]],\n";

                        // Replace all texture2DRect(<texName>, …) calls in the body
                        // texture2DRect(N, coord) → N.sample(N_samp, coord)
                        std::string pattern     = "texture2DRect(" + texName + ",";
                        std::string replacement = texName + ".sample(" + texName + "_samp,";
                        boost::algorithm::replace_all(body, pattern, replacement);

                        textureIndex++;
                        continue;
                    }

                    // uniform sampler2D <name>;
                    if (trimmed.find("uniform sampler2D ") != std::string::npos
                        && trimmed.find("sampler2DRect") == std::string::npos)
                    {
                        std::string texName = trimmed;
                        boost::algorithm::replace_all(texName, "uniform sampler2D", "");
                        boost::algorithm::replace_all(texName, ";", "");
                        boost::algorithm::trim(texName);

                        textureBindings << "    texture2d<float> " << texName
                                        << " [[texture(" << textureIndex << ")]],\n"
                                        << "    sampler " << texName << "_samp"
                                        << " [[sampler(" << textureIndex << ")]],\n";

                        std::string pattern     = "texture2D(" + texName + ",";
                        std::string replacement = texName + ".sample(" + texName + "_samp,";
                        boost::algorithm::replace_all(body, pattern, replacement);

                        textureIndex++;
                        continue;
                    }

                    // uniform <type> <name>;  → struct member
                    if (trimmed.substr(0, 8) == "uniform ")
                    {
                        std::string decl = trimmed.substr(8); // strip "uniform "
                        // Convert GLSL type names to MSL equivalents
                        boost::algorithm::replace_all(decl, "vec2", "float2");
                        boost::algorithm::replace_all(decl, "vec3", "float3");
                        boost::algorithm::replace_all(decl, "vec4", "float4");
                        boost::algorithm::replace_all(decl, "mat4", "float4x4");
                        boost::algorithm::replace_all(decl, "mat3", "float3x3");
                        boost::algorithm::replace_all(decl, "mat2", "float2x2");
                        uniformStructMembers << "    " << decl << "\n";
                        continue;
                    }

                    // varying in vec2 <name>; or  in vec2 <name>;
                    if (trimmed.find("varying") != std::string::npos
                        || (trimmed.substr(0, 3) == "in " && trimmed.find("vec2") != std::string::npos))
                    {
                        // These become VertexOut members — no separate declaration needed
                        continue;
                    }

                    // out vec4 FragColor;  → handled via struct output
                    if (trimmed.find("out vec4") != std::string::npos)
                        continue;
                }

                // ---- Patch body: gl_FragColor / FragColor → _out.color ----
                boost::algorithm::replace_all(body, "gl_FragColor", "_out");
                boost::algorithm::replace_all(body, "FragColor",    "_out");

                // ---- Patch body: GLSL built-in type names → MSL ----
                boost::algorithm::replace_all(body, "vec2", "float2");
                boost::algorithm::replace_all(body, "vec3", "float3");
                boost::algorithm::replace_all(body, "vec4", "float4");
                boost::algorithm::replace_all(body, "mat4", "float4x4");
                boost::algorithm::replace_all(body, "mat3", "float3x3");
                boost::algorithm::replace_all(body, "mat2", "float2x2");
                boost::algorithm::replace_all(body, "ivec2", "int2");
                boost::algorithm::replace_all(body, "ivec3", "int3");
                boost::algorithm::replace_all(body, "ivec4", "int4");

                // ---- Emit uniform buffer struct (may be empty) ----
                std::string uniformMembersStr = uniformStructMembers.str();
                if (!uniformMembersStr.empty())
                {
                    msl << "struct Uniforms {\n"
                        << uniformMembersStr
                        << "};\n\n";
                }

                // ---- Fragment entry point ----
                msl << "fragment float4 fragmentMain(\n"
                    << "    VertexOut in [[stage_in]]";

                // Append texture/sampler parameters (already formatted with leading comma)
                std::string texBindStr = textureBindings.str();
                if (!texBindStr.empty())
                {
                    msl << ",\n" << texBindStr;
                }

                if (!uniformMembersStr.empty())
                {
                    msl << "    constant Uniforms& uniforms [[buffer(0)]]";
                }

                msl << ")\n{\n"
                    << "    float4 _out;\n"
                    << body
                    << "    return _out;\n"
                    << "}\n";

                return msl.str();
            }

        } // anonymous namespace

        bool Program::compileMetal() const
        {
            void* devicePtr = IPCore::MetalContext::device();
            if (!devicePtr)
            {
                NSLog(@"[IPCore] compileMetal: no MTLDevice available");
                return false;
            }

            id<MTLDevice> device = (__bridge id<MTLDevice>)devicePtr;

            //
            // Generate the "main" GLSL body the same way compile() does for GL,
            // then translate it to MSL.
            //
            // We re-use the existing outputLocalFunction logic by temporarily
            // redirecting output to a string stream.  Because outputLocalFunction
            // is non-const and calls GL functions for the version string we
            // replicate only the body-generation part here.
            //

            // Collect the GLSL preamble (uniforms, varyings, etc.) from the
            // existing m_bindingMap — same logic as in compile().
            std::ostringstream glslPreamble;

            for (BindingMap::const_iterator i = m_bindingMap.begin();
                 i != m_bindingMap.end(); ++i)
            {
                const NameBinding& b = (*i).second;
                const Symbol*      S = b.bsymbol->symbol();

                if (b.uniform && !S->isSpecial() && !S->isCoordinate()
                    && !S->isOutputImage() && !S->isFragmentPositon())
                {
                    glslPreamble << "uniform " << S->glslTypeName() << " " << b.name << ";\n";
                }
            }

            if (m_needOutputSize)
                glslPreamble << "uniform vec2 _windowSize;\n";

            for (const std::string& id : m_outputSTSet)
                glslPreamble << "varying vec2 TexCoord" << id << ";\n";

            for (const std::string& id : m_outputSizeSet)
                glslPreamble << "uniform vec2 Size" << id << ";\n";

            // Build the fragment body: "return <expr>;" wrapped in braces
            std::ostringstream bodyStream;
            bodyStream << "    _out = "
                       << const_cast<Program*>(this)->recursiveOutputExpr(m_expr, false)
                       << ";\n";

            std::string mslSource = glslToMSL(glslPreamble.str(), bodyStream.str());

            if (Shader::debuggingType() != Shader::NoDebugInfo)
            {
                std::cout << "INFO: ---- Metal MSL source follows ----\n";
                outputAnnotatedCode(std::cout, mslSource);
            }

            NSError*  error  = nil;
            NSString* mslStr = [NSString stringWithUTF8String:mslSource.c_str()];

            id<MTLLibrary> library = [device newLibraryWithSource:mslStr
                                                          options:nil
                                                            error:&error];
            if (!library)
            {
                NSLog(@"[IPCore] MSL compile error in Program::compileMetal: %@",
                      error.localizedDescription);
                return false;
            }

            id<MTLFunction> vertFn = [library newFunctionWithName:@"vertexMain"];
            id<MTLFunction> fragFn = [library newFunctionWithName:@"fragmentMain"];

            if (!vertFn || !fragFn)
            {
                NSLog(@"[IPCore] compileMetal: missing entry points in compiled library");
                return false;
            }

            MTLRenderPipelineDescriptor* desc = [[MTLRenderPipelineDescriptor alloc] init];
            desc.vertexFunction              = vertFn;
            desc.fragmentFunction            = fragFn;
            // Use a standard BGRA_8Unorm pixel format as the default; callers that
            // need a different format (e.g. 10-bit BGR10A2) should override after
            // compilation.
            desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

            id<MTLRenderPipelineState> pipeline =
                [device newRenderPipelineStateWithDescriptor:desc error:&error];
            if (!pipeline)
            {
                NSLog(@"[IPCore] MTLRenderPipelineState creation failed: %@",
                      error.localizedDescription);
                return false;
            }

            // __bridge_retained transfers ownership to the void* — balanced by
            // CFRelease() in Program::releaseCompiledState().
            m_metalPipeline = (__bridge_retained void*)pipeline;
            return true;
        }

    } // namespace Shader
} // namespace IPCore

#endif // PLATFORM_DARWIN && USE_METAL
