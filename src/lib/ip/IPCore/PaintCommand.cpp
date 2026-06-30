//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/BrushTextureManager.h>
#include <IPCore/PaintCommand.h>
#include <IPBaseNodes/PaintIPNode.h>
#include <TwkPaint/StampPath.h>
#include <TwkMath/Function.h>
#include <cmath>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLPipeline.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/BasicGLProgram.h>
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Frustum.h>
#include <TwkGLText/TwkGLText.h>
#include <stl_ext/string_algo.h>
#include <QString>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <functional>
#include <iostream>

// Shader source strings embedded at compile time via quote_file() in
// IPCore/CMakeLists.txt.  Defined at global scope by the generated .cpp files.
extern const char* shape_compat_gl21;
extern const char* shape_common;
extern const char* shape_quad_vert;
extern const char* shape_arrow_frag;
extern const char* shape_ellipse_frag;
extern const char* shape_rectangle_frag;
extern const char* shape_line_frag;

namespace IPCore
{
    namespace Paint
    {
        using namespace std;
        using namespace TwkMath;
        using namespace TwkPaint;
        using namespace TwkFB;
        using namespace TwkGLText;
        using namespace TwkGLF;

        void Font::init() const
        {
            if (m_name == "")
                GLtext::init();
            else
                GLtext::init(m_name.c_str());
            m_initialized = true;
        }

        void Font::setSize(unsigned int s) const
        {
            assert(m_initialized);
            GLtext::size(s);
        }

        void Font::setColor(float r, float g, float b, float a) const { GLtext::color(r, g, b, a); }

        pair<float, float> Font::computeText(const string& text, string& t, float space, string origin) const
        {
            assert(m_initialized);

            vector<string> buffer;
            stl_ext::tokenize(buffer, text, "\n\r");
            string pipes;
            for (int i = buffer.size() - 1; i >= 0; i--)
            {
                if (i != buffer.size() - 1)
                {
                    t += "\n";
                    pipes += "\n";
                }
                t += buffer[i];
                pipes += "|,~`'\"yqj";
            }

            Box2f pb = GLtext::boundsNL(pipes.c_str(), space);
            float h = pb.min.y - pb.max.y - globalDescenderHeight();
            Box2f tb = GLtext::boundsNL(t.c_str(), space);
            float w = -tb.max.x;

            float xoffset, yoffset;
            if (origin == "top-left")
            {
                xoffset = 0;
                yoffset = h;
            }
            else if (origin == "top-center")
            {
                xoffset = w / 2.0;
                yoffset = h;
            }
            else if (origin == "top-right")
            {
                xoffset = w;
                yoffset = h;
            }
            else if (origin == "center-left")
            {
                xoffset = 0;
                yoffset = h / 2.0;
            }
            else if (origin == "center-center")
            {
                xoffset = w / 2.0;
                yoffset = h / 2.0;
            }
            else if (origin == "center-right")
            {
                xoffset = w;
                yoffset = h / 2.0;
            }
            else if (origin == "bottom-left")
            {
                xoffset = 0;
                yoffset = 0;
            }
            else if (origin == "bottom-center")
            {
                xoffset = w / 2.0;
                yoffset = 0;
            }
            else if (origin == "bottom-right")
            {
                xoffset = w;
                yoffset = 0;
            }
            else // old behavior default
            {
                xoffset = 0;
                yoffset = globalAscenderHeight() - (pb.max.y - pb.min.y);
            }
            return make_pair(xoffset, yoffset);
        }

        float Font::globalAscenderHeight() const { return GLtext::globalAscenderHeight(); }

        float Font::globalDescenderHeight() const { return GLtext::globalDescenderHeight(); }

        size_t PolyLine::getType() const { return Command::PolyLine; }

        //
        // this is for rv3 backwards compatability
        // in rv4 we updated the mechanism for over mode (the basic paint
        // stroke), and the look different from rv3.
        //
        void PolyLine::executeOldOverMode(CommandContext& context) const
        {
            const GLFBO* textureFBO = context.currentTexture;
            const GLFBO* currentFBO = context.currentRender;
            const float w = textureFBO->width();
            const float h = textureFBO->height();

            GLState* glState = context.glState;

            currentFBO->bind();
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            TWK_GLDEBUG;
            glClear(GL_COLOR_BUFFER_BIT);
            TWK_GLDEBUG;

            // Draw a quad with the existing renders as the background
            GLPipeline* glPipeline = glState->useGLProgram(textureRectGLProgram());

            // transforms
            Mat44f identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

            Frustumf f;
            f.window(0, w - 1, 0, h - 1, -1, 1, true);
            Mat44f projMat = f.matrix();

            glPipeline->setModelview(identity);
            glPipeline->setProjection(projMat);
            glPipeline->setViewport(0, 0, w, h);

            GLint id = 0;
            glPipeline->setUniformInt("texture0", 1, &id);
            glActiveTexture(GL_TEXTURE0);
            textureFBO->bindColorTexture(0);

            //
            //  NOTE: rectangle coords are [0,w] x [0,h]
            //  *not* [0,w-1] x [0,h-1]
            //
            float data[] = {0, 0, 0, 0, w, 0, w - 1, 0, w, h, w - 1, h - 1, 0, h, 0, h - 1};
            PrimitiveData buffer(data, NULL, GL_QUADS, 4, 1, 16 * sizeof(float));
            std::vector<VertexAttribute> attributeInfo;

            attributeInfo.push_back(VertexAttribute(std::string("in_Position"), GL_FLOAT, 2, 2 * sizeof(float), 4 * sizeof(float)));

            attributeInfo.push_back(VertexAttribute(std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 4 * sizeof(float)));

            RenderPrimitives renderprimitives(glState->activeGLProgram(), buffer, attributeInfo, glState->vboList());

            renderprimitives.setupAndRender();

            textureFBO->unbindColorTexture();

            //  do paint
            if (brush == "gauss")
            {
                glPipeline = glState->useGLProgram(softPaintOldReplaceGLProgram());
            }
            else
            {
                glPipeline = glState->useGLProgram(paintOldReplaceGLProgram());
            }

            glPipeline->setProjection(context.projMatrix);
            glPipeline->setModelview(context.modelviewMatrix);
            glPipeline->setViewport(0, 0, w, h);

            Color pcolor = color;
            glPipeline->setUniformFloat("uniformColor", 4, &(pcolor[0]));

            // draw
            glEnable(GL_BLEND);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            RenderPrimitives renderprimitives2(glState->activeGLProgram(), primitives, primitiveAttributes, glState->vboList());
            renderprimitives2.setupAndRender();

            // cleanup
            currentFBO->unbind();

            glDisable(GL_BLEND);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }

        // verts owns the vertex data; primData holds a raw pointer into verts.data().
        // std::vector move keeps the allocation intact, so the pointer survives RVO.
        struct StampQuads
        {
            std::vector<float> verts;
            PrimitiveData primData;
            std::vector<VertexAttribute> attrs;
        };

        // CPU-batches stamp instances into GL_QUADS vertex data.
        // VBO layout matches PolyLine::build(): texcoord block [all (u,v)] then
        // position block [all (x,y)], tightly packed, no index buffer.
        // Each stamp is 4 vertices with UV in [0,1], rotated by angle, scaled by radius/squish.
        // NOTE: can be replaced by VAO + glDrawArraysInstanced (GL 3.3) if OpenGL is upgraded.
        // The CPU vertex expansion loop below would be eliminated entirely.
        static StampQuads buildStampQuads(const std::vector<StampInstance>& stamps)
        {
            const size_t N = stamps.size();
            const size_t Nv = 4 * N; // 4 vertices per quad

            StampQuads result;

            // [all UVs: u0 v0 u1 v1 ... | all positions: x0 y0 x1 y1 ...]
            result.verts.resize(4 * Nv);

            // UV block: BL=(0,0)  BR=(1,0)  TR=(1,1)  TL=(0,1)
            static const float kUV[4][2] = {{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}};
            for (size_t i = 0; i < N; ++i)
                for (int v = 0; v < 4; ++v)
                {
                    result.verts[2 * (4 * i + v) + 0] = kUV[v][0];
                    result.verts[2 * (4 * i + v) + 1] = kUV[v][1];
                }

            // Position block: local corners (half-extents: radius x, radius*squish y),
            // rotated by stamp.angle degrees and translated to stamp.pos.
            static const float kCX[4] = {-1.f, 1.f, 1.f, -1.f};
            static const float kCY[4] = {-1.f, -1.f, 1.f, 1.f};
            static constexpr float kPi = 3.14159265f;
            const size_t posOff = 2 * Nv;
            for (size_t i = 0; i < N; ++i)
            {
                const auto& s = stamps[i];
                const float ca = std::cos(s.angle * (kPi / 180.f));
                const float sa = std::sin(s.angle * (kPi / 180.f));
                for (int v = 0; v < 4; ++v)
                {
                    const float lx = kCX[v] * s.radius;
                    const float ly = kCY[v] * s.radius * s.squish;
                    result.verts[posOff + 2 * (4 * i + v) + 0] = s.pos.x + lx * ca - ly * sa;
                    result.verts[posOff + 2 * (4 * i + v) + 1] = s.pos.y + lx * sa + ly * ca;
                }
            }

            result.primData = PrimitiveData(result.verts.data(), nullptr, GL_QUADS, Nv, N, result.verts.size() * sizeof(float));
            result.attrs.push_back(VertexAttribute("in_TexCoord0", GL_FLOAT, 2, 0, 0));
            result.attrs.push_back(VertexAttribute("in_Position", GL_FLOAT, 2, static_cast<int>(2 * Nv * sizeof(float)), 0));
            return result;
        }

        void PolyLine::execute(CommandContext& context) const
        {
            const auto* localPoly = dynamic_cast<const PaintIPNode::LocalPolyLine*>(this);
            const bool isStamp = localPoly && !localPoly->stampInstances.empty();

            if (!npoints && !isStamp)
                return;

            if (!isStamp)
            {
                HashValue newid = hashValue();
                if (!built || idhash != newid)
                {
                    idhash = newid;
                    build();
                }

                if (version < 3 && mode == OverMode)
                {
                    executeOldOverMode(context);
                    return;
                }
            }

            const GLFBO* originalFBO = context.initialRender;
            const GLFBO* textureFBO = context.currentTexture;
            const GLFBO* currentFBO = context.currentRender;
            const float w = textureFBO->width();
            const float h = textureFBO->height();

            //////////////////////////////////////////////////////////////////////////
            // first render the background
            //////////////////////////////////////////////////////////////////////////
            GLState* glState = context.glState;
            currentFBO->bind();

            GLPipeline* glPipeline = glState->useGLProgram(textureRectGLProgram());
            Mat44f identity(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
            Frustumf f;
            f.window(0, w - 1, 0, h - 1, -1, 1, true);
            Mat44f projMat = f.matrix();

            glPipeline->setModelview(identity);
            glPipeline->setProjection(projMat);
            glPipeline->setViewport(0, 0, w, h);

            int id = 0;
            glPipeline->setUniformInt("texture0", 1, &id);
            glActiveTexture(GL_TEXTURE0);
            textureFBO->bindColorTexture(0);

            glDisable(GL_BLEND);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            TWK_GLDEBUG;
            glClear(GL_COLOR_BUFFER_BIT);
            TWK_GLDEBUG;
            //  NOTE: rectangle coords are [0,w] x [0,h]
            //  *not* [0,w-1] x [0,h-1]
            float data[] = {0, 0, 0, 0, w, 0, w - 1, 0, w, h, w - 1, h - 1, 0, h, 0, h - 1};
            PrimitiveData buffer(data, NULL, GL_QUADS, 4, 1, 16 * sizeof(float));
            std::vector<VertexAttribute> attributeInfo2;
            attributeInfo2.push_back(VertexAttribute(std::string("in_Position"), GL_FLOAT, 2, 2 * sizeof(float), 4 * sizeof(float)));

            attributeInfo2.push_back(VertexAttribute(std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 4 * sizeof(float)));
            RenderPrimitives renderprimitives2(glState->activeGLProgram(), buffer, attributeInfo2, glState->vboList());
            renderprimitives2.setupAndRender();

            textureFBO->unbindColorTexture();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);

            //////////////////////////////////////////////////////////////////////////
            // render paint onto the background
            //////////////////////////////////////////////////////////////////////////

            // determine effective stroke color (respects ghost mode)
            Color pcolor;
            const auto* localCommand = dynamic_cast<const PaintIPNode::LocalCommand*>(this);
            bool isGhostOn = (localCommand != nullptr) ? localCommand->ghostOn : false;
            pcolor = isGhostOn ? localCommand->ghostColor : color;

            if (isStamp)
            {
                // Resolve brush properties at render time — the manager is
                // guaranteed loaded by renderPaintCommands() before execute().
                const BrushInfo info = BrushTextureManager::instance().get(brush);

                const GLProgram* stampProg = info.textureId ? texturePaintReplaceGLProgram()
                                                            : (info.softShader ? softPaintReplaceGLProgram() : paintReplaceGLProgram());
                GLPipeline* stampPipeline = glState->useGLProgram(stampProg);
                stampPipeline->setProjection(context.projMatrix);
                stampPipeline->setModelview(context.modelviewMatrix);
                stampPipeline->setViewport(0, 0, w, h);
                stampPipeline->setUniformFloat("uniformColor", 4, &(pcolor[0]));

                if (info.textureId)
                {
                    int tipUnit = 1;
                    stampPipeline->setUniformInt("brushTip", 1, &tipUnit);
                    glActiveTexture(GL_TEXTURE0 + 1);
                    glBindTexture(GL_TEXTURE_2D, info.textureId);
                }

                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(context.stencilBox[0], context.stencilBox[1], context.stencilBox[2] - context.stencilBox[0],
                              context.stencilBox[3] - context.stencilBox[1]);
                }

                auto sq = buildStampQuads(localPoly->stampInstances);

                // BlendMarker and ghost mode both use two-pass isolation: GL_MAX into
                // a cleared currentFBO (prevents intra-stroke opacity accumulation),
                // then composite the background under the result via ONE_MINUS_DST_ALPHA.
                if (info.blendMode == BlendMarker || isGhostOn)
                {

                    // Pass 1: stamps into cleared currentFBO
                    glDisable(GL_BLEND);
                    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    glEnable(GL_BLEND);
                    glBlendEquationSeparate(GL_MAX, GL_MAX);
                    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
                    {
                        RenderPrimitives rpStamp(glState->activeGLProgram(), sq.primData, sq.attrs, glState->vboList());
                        rpStamp.setupAndRender();
                    }
                    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

                    // Pass 2: composite background under stamps
                    GLPipeline* bgPipeline = glState->useGLProgram(textureRectGLProgram());
                    bgPipeline->setModelview(identity);
                    bgPipeline->setProjection(projMat);
                    bgPipeline->setViewport(0, 0, w, h);
                    int bgUnit = 0;
                    bgPipeline->setUniformInt("texture0", 1, &bgUnit);
                    glActiveTexture(GL_TEXTURE0);
                    textureFBO->bindColorTexture(0);
                    glEnable(GL_BLEND);
                    glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_ONE, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
                    {
                        RenderPrimitives rpBg(glState->activeGLProgram(), buffer, attributeInfo2, glState->vboList());
                        rpBg.setupAndRender();
                    }
                    textureFBO->unbindColorTexture();
                }
                else
                {
                    glEnable(GL_BLEND);
                    if (info.blendMode == BlendAdditive)
                        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
                    else
                        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    RenderPrimitives rpStamp(glState->activeGLProgram(), sq.primData, sq.attrs, glState->vboList());
                    rpStamp.setupAndRender();
                }

                if (context.hasStencil)
                    glDisable(GL_SCISSOR_TEST);
            }
            else
            {
                switch (mode)
                {
                case EraseMode:
                    if (brush != "gauss")
                        glPipeline = glState->useGLProgram(paintEraseGLProgram());
                    else
                        glPipeline = glState->useGLProgram(softPaintEraseGLProgram());
                    break;
                case ScaleMode:
                case GradientScaleMode:
                    if (brush != "gauss")
                        glPipeline = glState->useGLProgram(paintScaleGLProgram());
                    else
                        glPipeline = glState->useGLProgram(softPaintScaleGLProgram());
                    break;
                case CloneMode:
                    if (brush != "gauss")
                        glPipeline = glState->useGLProgram(paintCloneGLProgram());
                    else
                        glPipeline = glState->useGLProgram(softPaintCloneGLProgram());
                    break;
                case TessellateMode:
                    glPipeline = glState->useGLProgram(paintTessellateGLProgram());
                    break;
                case OverMode:
                default:
                    if (brush != "gauss")
                        glPipeline = glState->useGLProgram(paintReplaceGLProgram());
                    else
                        glPipeline = glState->useGLProgram(softPaintReplaceGLProgram());
                    break;
                }

                // set transforms
                glPipeline->setProjection(context.projMatrix);
                glPipeline->setModelview(context.modelviewMatrix);
                glPipeline->setViewport(0, 0, w, h);

                // bind textures
                if (mode == EraseMode)
                {
                    id = 1;
                    glPipeline->setUniformInt("texture0", 1, &id);
                    glActiveTexture(GL_TEXTURE0 + 1);
                    originalFBO->bindColorTexture(0);
                }
                else if (mode == ScaleMode || mode == CloneMode)
                {
                    id = 1;
                    glPipeline->setUniformInt("texture0", 1, &id);
                    glActiveTexture(GL_TEXTURE0 + 1);
                    textureFBO->bindColorTexture(0);
                }

                // set uniforms
                glPipeline->setUniformFloat("uniformColor", 4, &(pcolor[0]));
                float coffset[2] = {50, 50};
                if (mode == CloneMode)
                    glPipeline->setUniformFloat("cloneOffset", 2, coffset);
                if (mode != OverMode)
                {
                    glPipeline->setUniformFloat("texWidth", 1, (float*)&w);
                    glPipeline->setUniformFloat("texHeight", 1, (float*)&h);
                }

                // render
                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(context.stencilBox[0], context.stencilBox[1], context.stencilBox[2] - context.stencilBox[0],
                              context.stencilBox[3] - context.stencilBox[1]);
                }
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                RenderPrimitives renderprimitives3(glState->activeGLProgram(), primitives, primitiveAttributes, glState->vboList());
                renderprimitives3.setupAndRender();
                if (context.hasStencil)
                    glDisable(GL_SCISSOR_TEST);

                // clean up textures (ribbon modes only)
                if (mode == EraseMode)
                    originalFBO->unbindColorTexture();
                else if (mode == ScaleMode || mode == CloneMode)
                    textureFBO->unbindColorTexture();
            }

            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_BLEND);

            currentFBO->unbind();
            glState->useGLProgram(defaultGLProgram());
        }

        void PolyLine::hash(ostream& ostream) const
        {
            for (const auto& point : points)
            {
                ostream << point;
            }

            ostream << npoints << width << smoothingWidth << splat;

            for (const auto& width : widths)
            {
                ostream << width;
            }

            ostream << color << brush << join << cap << mode << debug;
        }

        PolyLine::HashValue PolyLine::hashValue() const
        {
            ostringstream ostream;

            for (const auto& point : points)
            {
                ostream << point;
            }

            ostream << npoints << width << smoothingWidth << splat;

            for (const auto& width : widths)
            {
                ostream << width;
            }

            ostream << color << brush << join << cap << mode << debug;

            std::string id = ostream.str();

            boost::hash<string> string_hash;
            size_t v = string_hash(id);

            if (sizeof(size_t) == 8)
            {
                return (v & 0xffffffff) ^ ((v >> 32) & 0xffffffff);
            }
            else
            {
                return v;
            }
        }

        void PolyLine::build() const
        {
            path.clear();

            if (!widths.empty())
            {
                for (size_t i = 0; i < npoints; i++)
                    path.add(points[i], widths[i]);
            }
            else
            {
                for (size_t i = 0; i < npoints; i++)
                    path.add(points[i]);
                path.setContantWidth(width);
            }

            path.computeGeometry(join, cap, Path::RadiallySymmetric, Path::QualityAlgorithm, true, smoothingWidth, splat);

            // setup data to be prepared for late rendering
            const PointArray& opoints = path.outputPoints();
            const PointArray& tpoints = path.outputTexCoords();
            const IndexTriangleArray& triangles = path.outputTriangles();
            const ScalarArray& direction = path.outputDirectionalities();

            size_t vertexno = opoints.size();
            size_t trino = triangles.size();

            size_t vbodatasize = 0;
            if (mode == TessellateMode)
            {
                vertexno = 3 * trino;
                vbodatasize += 4 * vertexno;
                vbodatasize += 4 * vertexno; // all vertices of a triangle have
                                             // the same color
            }
            else
            {
                vbodatasize += 4 * vertexno;

                if (mode == GradientScaleMode)
                {
                    vbodatasize += vertexno;
                }
                else if (mode == TessellateMode)
                {
                    vbodatasize += 4 * vertexno;
                }
            }

            primitiveData.clear();
            primitiveData.resize(vbodatasize);
            primitiveIndices.clear();

            if (mode != TessellateMode)
            {
                for (size_t i = 0; i < vertexno; i++)
                {
                    primitiveData[2 * i + 0] = (float)tpoints[i].x;
                    primitiveData[2 * i + 1] = (float)tpoints[i].y;
                }
                for (size_t i = 0; i < vertexno; i++)
                {
                    primitiveData[2 * vertexno + 2 * i + 0] = (float)opoints[i].x;
                    primitiveData[2 * vertexno + 2 * i + 1] = (float)opoints[i].y;
                }
                if (mode == GradientScaleMode)
                {
                    for (size_t i = 0; i < vertexno; i++)
                    {
                        primitiveData[4 * vertexno + i] = (float)direction[i];
                    }
                }

                // indices
                primitiveIndices.resize(3 * triangles.size());
                for (size_t i = 0; i < trino; i++)
                {
                    primitiveIndices[3 * i + 0] = triangles[i][0];
                    primitiveIndices[3 * i + 1] = triangles[i][1];
                    primitiveIndices[3 * i + 2] = triangles[i][2];
                }
            }
            else
            {
                // fill in vertices
                for (size_t i = 0; i < trino; i++)
                {
                    primitiveData[6 * i + 0] = (float)tpoints[triangles[i][0]].x;
                    primitiveData[6 * i + 1] = (float)tpoints[triangles[i][0]].y;
                    primitiveData[6 * i + 2] = (float)tpoints[triangles[i][1]].x;
                    primitiveData[6 * i + 3] = (float)tpoints[triangles[i][1]].y;
                    primitiveData[6 * i + 4] = (float)tpoints[triangles[i][2]].x;
                    primitiveData[6 * i + 5] = (float)tpoints[triangles[i][2]].y;
                }
                size_t offset = 6 * trino;
                for (size_t i = 0; i < trino; i++)
                {
                    primitiveData[offset + 6 * i + 0] = (float)opoints[triangles[i][0]].x;
                    primitiveData[offset + 6 * i + 1] = (float)opoints[triangles[i][0]].y;
                    primitiveData[offset + 6 * i + 2] = (float)opoints[triangles[i][1]].x;
                    primitiveData[offset + 6 * i + 3] = (float)opoints[triangles[i][1]].y;
                    primitiveData[offset + 6 * i + 4] = (float)opoints[triangles[i][2]].x;
                    primitiveData[offset + 6 * i + 5] = (float)opoints[triangles[i][2]].y;
                }

                // fill in color per triangle
                offset = 12 * trino;
                srand(std::time(0));
                for (size_t i = 0; i < trino; i++)
                {
                    float r = (float)rand() / RAND_MAX;
                    float g = (float)rand() / RAND_MAX;
                    float b = (float)rand() / RAND_MAX;
                    primitiveData[offset + 12 * i] = r;
                    primitiveData[offset + 12 * i + 1] = g;
                    primitiveData[offset + 12 * i + 2] = b;
                    primitiveData[offset + 12 * i + 3] = 1.0;

                    primitiveData[offset + 12 * i + 4] = r;
                    primitiveData[offset + 12 * i + 5] = g;
                    primitiveData[offset + 12 * i + 6] = b;
                    primitiveData[offset + 12 * i + 7] = 1.0;

                    primitiveData[offset + 12 * i + 8] = r;
                    primitiveData[offset + 12 * i + 9] = g;
                    primitiveData[offset + 12 * i + 10] = b;
                    primitiveData[offset + 12 * i + 11] = 1.0;
                }
            }

            primitives.m_dataBuffer = &(primitiveData[0]);
            primitives.m_indexBuffer = mode == TessellateMode ? 0 : &(primitiveIndices[0]);
            primitives.m_primitiveType = GL_TRIANGLES;
            primitives.m_vertexNo = vertexno;
            primitives.m_primitiveNo = trino;
            primitives.m_size = sizeof(float) * vbodatasize;

            primitiveAttributes.clear();
            primitiveAttributes.push_back(VertexAttribute(std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 0)); // texture
            primitiveAttributes.push_back(
                VertexAttribute(std::string("in_Position"), GL_FLOAT, 2, 2 * vertexno * sizeof(float), 0)); // vertex
            if (mode == GradientScaleMode)
            {
                primitiveAttributes.push_back(
                    VertexAttribute(std::string("in_DirectionCoord"), GL_FLOAT, 1, 4 * vertexno * sizeof(float), 0)); // direction coord
            }
            else if (mode == TessellateMode)
            {
                primitiveAttributes.push_back(VertexAttribute(std::string("in_UniformColor"), GL_FLOAT, 4, 4 * vertexno * sizeof(float),
                                                              0)); // color for each triangle
            }

            built = true;
        }

        PushFrameBuffer::PushFrameBuffer() {}

        void PushFrameBuffer::execute(CommandContext&) const {}

        void PushFrameBuffer::hash(ostream& o) const { o << "pu"; }

        size_t PushFrameBuffer::getType() const { return Command::PushFrameBuffer; }

        PopFrameBuffer::PopFrameBuffer() {}

        void PopFrameBuffer::execute(CommandContext&) const {}

        void PopFrameBuffer::hash(ostream& o) const { o << "po"; }

        size_t PopFrameBuffer::getType() const { return Command::PopFrameBuffer; }

        void Text::setup(CommandContext& context)
        {
            const GLFBO* fbo = context.currentRender;
            context.currentTexture->copyTo(fbo);
            fbo->bind();

            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            // Premultiplied-alpha blend: texture is ARGB32_Premultiplied.
            glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            glActiveTexture(GL_TEXTURE0);
        }

        void Text::cleanup(CommandContext& context)
        {
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);

            const GLFBO* fbo = context.currentRender;
            fbo->unbind();
        }

        void Text::execute(CommandContext& context) const
        {
            // Determine effective text colour (ghost mode support)
            const auto* localCommand = dynamic_cast<const PaintIPNode::LocalCommand*>(this);
            const bool isGhostOn = (localCommand != nullptr) ? localCommand->ghostOn : false;
            Color textColor = isGhostOn ? localCommand->ghostColor : color;

            // All text renders via QPainter → QImage → GL texture.
            // Pre-Qt sessions without fontFamily set use the system default font
            // at fontSize 24px (the readProp default in compileTextComponent).
            // Visual tuning for legacy sessions can be done by adjusting those defaults.
            const auto* localText = dynamic_cast<const PaintIPNode::LocalText*>(this);
            const std::string& effectiveFontFamily = localText ? localText->fontFamily : "";
            const float effectiveFontSize = localText ? localText->fontSize : 24.0f;
            const std::string& effectiveFontWeight = localText ? localText->fontWeight : PaintIPNode::FontWeight::Normal;
            const std::string& effectiveFontStyle = localText ? localText->fontStyle : PaintIPNode::FontStyle::Normal;
            const std::string& effectiveTextDecor = localText ? localText->textDecoration : PaintIPNode::TextDecoration::None;

            {
                const GLFBO* currentFBO = context.currentRender;
                const float fbW = currentFBO->width();
                const float fbH = currentFBO->height();

                // The FBO is screen-resolution (viewport pixels). pos.x/pos.y are
                // in image/world space, so after the projection×modelview transform
                // the text quad scales proportionally with how large the image
                // appears on screen. The y-scale of that combined matrix tells us
                // how many NDC units one image-space unit covers; multiplying by
                // fbH/2 gives screen pixels per image-space unit.
                //
                // We must bake the QImage texture at exactly that pixel density so
                // the texture fills the quad pixel-for-pixel. Without this, the
                // quad will grows with the image display scale while the texture stays
                // fixed and will change size according to the viewport size.
                const TwkMath::Mat44f transform = context.projMatrix * context.modelviewMatrix;
                const float proj_scale_x = std::max(std::abs(transform.m00), 0.001f);
                const float proj_scale_y = std::max(std::abs(transform.m11), 0.001f);

                // Render the font at the screen pixel size that corresponds to
                // effectiveFontSize natural image pixels at the current display
                // resolution. Dividing by imageHeight normalises the font size to
                // a fraction of the image so text always occupies the same
                // proportion of the image regardless of screen size or DPI.
                const float imageH = (context.imageHeight > 0) ? static_cast<float>(context.imageHeight) : fbH;
                const float renderFontSize = std::max(1.0f, effectiveFontSize * proj_scale_y * fbH / (2.0f * imageH));

                // ── Build QFont ───────────────────────────────────────────────
                QFont qfont;
                const QStringList families = QFontDatabase::families();
                const QString qfamily = QString::fromStdString(effectiveFontFamily);
                if (!qfamily.isEmpty() && families.contains(qfamily))
                {
                    qfont.setFamily(qfamily);
                }
                qfont.setPixelSize(static_cast<int>(renderFontSize));
                qfont.setWeight(effectiveFontWeight == PaintIPNode::FontWeight::Bold ? QFont::Bold : QFont::Normal);
                qfont.setStyle(effectiveFontStyle == PaintIPNode::FontStyle::Italic ? QFont::StyleItalic : QFont::StyleNormal);
                qfont.setUnderline(effectiveTextDecor == PaintIPNode::TextDecoration::Underline);

                // ── Measure text ──────────────────────────────────────────────
                const QString qtext = QString::fromStdString(text);
                if (qtext.isEmpty())
                    return;
                QImage refImg(1, 1, QImage::Format_ARGB32_Premultiplied);
                QFontMetricsF fm(qfont, &refImg);
                // For multi-line text, measure each line separately: take the
                // max advance width and accumulate lineSpacing() per line so
                // the image is tall enough for all lines.
                const auto lines = qtext.split('\n');
                float maxLineWidth = 0.0f;
                for (const auto& line : lines)
                {
                    const float lw = line.isEmpty() ? 0.0f : static_cast<float>(fm.horizontalAdvance(line));
                    if (lw > maxLineWidth)
                        maxLineWidth = lw;
                }
                const int lineCount = std::max(1, static_cast<int>(lines.size()));
                const int imgW = static_cast<int>(std::ceil(maxLineWidth)) + 4;
                const int imgH = static_cast<int>(std::ceil(fm.lineSpacing() * lineCount)) + 4;

                if (imgW <= 0 || imgH <= 0)
                    return;

                // ── Render to QImage ──────────────────────────────────────────
                QImage img(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
                img.fill(Qt::transparent);
                {
                    QPainter painter(&img);
                    painter.setFont(qfont);
                    auto toQColorComponent = [](float v) { return static_cast<int>(std::max(0.0f, std::min(1.0f, v)) * 255); };
                    painter.setPen(QColor(toQColorComponent(textColor.x), toQColorComponent(textColor.y), toQColorComponent(textColor.z),
                                          toQColorComponent(textColor.w)));
                    painter.setRenderHint(QPainter::TextAntialiasing, true);
                    // Draw at baseline. AlignLeft|AlignVCenter within the padded rect.
                    painter.drawText(QRectF(0, 0, imgW, imgH), Qt::AlignLeft | Qt::AlignTop, qtext);
                }

                // ── Upload as GL texture (OpenGL 2.1 compatible) ───────────────
                // Convert ARGB32_Premultiplied (Qt) → RGBA byte order for GL.
                QImage glImg = img.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

                GLuint texId = 0;
                glGenTextures(1, &texId);
                glBindTexture(GL_TEXTURE_2D, texId);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImg.width(), glImg.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImg.constBits());

                // ── Draw as textured quad ─────────────────────────────────────
                // Convert world-space pos to screen coords for quad placement.
                GLState* glState = context.glState;
                currentFBO->bind();

                GLPipeline* glPipeline = glState->useGLProgram(textureGLProgram());
                glPipeline->setProjection(context.projMatrix);
                glPipeline->setModelview(context.modelviewMatrix);
                glPipeline->setViewport(0, 0, fbW, fbH);

                int texUnit = 0;
                glPipeline->setUniformInt("texture0", 1, &texUnit);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texId);

                // The quad spans from pos to pos + (tw, th) in image/world space.
                // Texture origin is bottom-left in GL, image origin top-left in Qt
                // so we flip the T coordinate.
                //
                // tw/th convert texture pixels → image-space units. Dividing by
                // (proj_scale × fbDim) cancels the projection scale so the
                // on-screen quad is exactly imgW×imgH screen pixels — matching
                // the texture resolution and ensuring crisp text at any display size.
                const float qx = pos.x;
                const float qy = pos.y;
                const float tw = (fbW > 0) ? imgW * 2.0f / (proj_scale_x * fbW) : 0.001f;
                const float th = (fbH > 0) ? imgH * 2.0f / (proj_scale_y * fbH) : 0.001f;

                // Vertices: (x,y, u,v) quads — position + texcoord
                float data[] = {
                    qx,      qy,      0.0f, 1.0f, // BL
                    qx + tw, qy,      1.0f, 1.0f, // BR
                    qx + tw, qy + th, 1.0f, 0.0f, // TR
                    qx,      qy + th, 0.0f, 0.0f, // TL
                };

                // Texture is Format_ARGB32_Premultiplied so use premul blend:
                // GL_ONE (not GL_SRC_ALPHA) avoids squaring the alpha.
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                PrimitiveData databuffer(data, nullptr, GL_QUADS, 4, 1, sizeof(float) * 16);
                std::vector<VertexAttribute> attrs;
                attrs.push_back(VertexAttribute("in_Position", GL_FLOAT, 2, 0, 4 * sizeof(float)));
                attrs.push_back(VertexAttribute("in_TexCoord0", GL_FLOAT, 2, 2 * sizeof(float), 4 * sizeof(float)));

                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(static_cast<GLint>(context.stencilBox[0]), static_cast<GLint>(context.stencilBox[1]),
                              static_cast<GLsizei>(context.stencilBox[2] - context.stencilBox[0]),
                              static_cast<GLsizei>(context.stencilBox[3] - context.stencilBox[1]));
                }
                RenderPrimitives rp(glState->activeGLProgram(), databuffer, attrs, glState->vboList());
                rp.setupAndRender();
                if (context.hasStencil)
                    glDisable(GL_SCISSOR_TEST);

                glDisable(GL_BLEND);
                glBindTexture(GL_TEXTURE_2D, 0);
                currentFBO->unbind();

                // Free the temporary texture immediately — the texture cache in
                // PaintIPNode (keyed on text+font+color) is responsible for reuse
                // across frames.  This execute() path is only reached when the cache
                // misses or the node opts out of caching.
                glDeleteTextures(1, &texId);
            }
        }

        void Text::hash(ostream& o) const { o << pos << color << text << font << origin << ptsize << scale << rotation << spacing; }

        size_t Text::getType() const { return Command::Text; }

        namespace
        {

            void executeQuad(CommandContext& context, float* points, Color color, GLint mode)
            {
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                Color pcolor = color;
                GLState* glState = context.glState;
                GLPipeline* glPipeline = glState->useGLProgram(defaultGLProgram());
                const GLFBO* fbo = context.currentRender;
                int programId = glPipeline->programId();
                const float w = fbo->width();
                const float h = fbo->height();

                glPipeline->setUniformFloat("uniformColor", 4, &(pcolor[0]));
                glPipeline->setProjection(context.projMatrix);
                glPipeline->setModelview(context.modelviewMatrix);
                glPipeline->setViewport(0, 0, w, h);

                context.currentTexture->copyTo(fbo);
                fbo->bind();

                PrimitiveData databuffer((void*)points, NULL, mode, 4, 1, sizeof(float) * 8);
                std::vector<VertexAttribute> attributeInfo;
                attributeInfo.push_back(VertexAttribute(std::string("in_Position"), GL_FLOAT, 2, 0, 0));
                RenderPrimitives renderprimitives(glState->activeGLProgram(), databuffer, attributeInfo, glState->vboList());
                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(context.stencilBox[0], context.stencilBox[1], context.stencilBox[2] - context.stencilBox[0],
                              context.stencilBox[3] - context.stencilBox[1]);
                }
                renderprimitives.setupAndRender();
                if (context.hasStencil)
                    glDisable(GL_SCISSOR_TEST);
                glDisable(GL_BLEND);

                fbo->unbind();
            }

        }; //  namespace

        void Rectangle::execute(CommandContext& context) const
        {
            float data[] = {pos.x, pos.y, pos.x + width, pos.y, pos.x + width, pos.y + height, pos.x, pos.y + height};

            executeQuad(context, data, color, GL_QUADS);
        }

        void Rectangle::hash(ostream& o) const { o << pos << height << width << color; }

        size_t Rectangle::getType() const { return Command::Rectangle; }

        void Quad::execute(CommandContext& context) const
        {
            GLint dm = (drawMode == SolidMode) ? GL_QUADS : GL_LINE_LOOP;

            executeQuad(context, (float*)points, color, dm);
        }

        void Quad::hash(ostream& o) const { o << points[0] << points[1] << points[2] << points[3] << drawMode << color; }

        size_t Quad::getType() const { return Command::Quad; }

        void ExecuteAllBefore::execute(CommandContext& context) const {}

        void ExecuteAllBefore::hash(ostream& o) const {}

        size_t ExecuteAllBefore::getType() const { return Command::ExecuteAllBefore; }

        // Helpers shared by all four shape execute() methods.
        //
        // Shader source strings are embedded at compile time via quote_file() in
        // IPCore/CMakeLists.txt — no runtime file I/O needed.

        namespace
        {
            /// Build and cache the compiled GLSL program for a shape shader.
            /// Returns nullptr if compilation fails.
            static const TwkGLF::GLProgram* getShapeProgram(const std::string& vertSrc, const std::string& fragSrc)
            {
                if (vertSrc.empty() || fragSrc.empty())
                    return nullptr;
                return TwkGLF::BasicGLProgram::select(vertSrc, fragSrc);
            }

            /// Assemble vert + frag source from embedded string literals.
            /// All shape shaders share the same vertex shader (shape_quad.vert).
            /// compat_gl21.glsl is prepended to both; shape_common.glsl is prepended
            /// to the fragment shader only.
            struct ShapeShaderSrc
            {
                std::string vert;
                std::string frag;
            };

            static ShapeShaderSrc buildShapeShaderSrc(const char* fragBody)
            {
                ShapeShaderSrc src;
                src.vert = std::string(shape_compat_gl21) + shape_quad_vert;
                src.frag = std::string(shape_compat_gl21) + shape_common + fragBody;
                return src;
            }

            /// Cached program accessors — each is compiled once per GL context.
            static const TwkGLF::GLProgram* rectShapeGLProgram()
            {
                static const TwkGLF::GLProgram* prog = nullptr;
                if (!prog)
                {
                    auto src = buildShapeShaderSrc(shape_rectangle_frag);
                    prog = getShapeProgram(src.vert, src.frag);
                    if (!prog)
                        std::cerr << "[ShapeSDF] rectangle shader failed to compile\n";
                }
                return prog;
            }

            static const TwkGLF::GLProgram* ellipseShapeGLProgram()
            {
                static const TwkGLF::GLProgram* prog = nullptr;
                if (!prog)
                {
                    auto src = buildShapeShaderSrc(shape_ellipse_frag);
                    prog = getShapeProgram(src.vert, src.frag);
                    if (!prog)
                        std::cerr << "[ShapeSDF] ellipse shader failed to compile\n";
                }
                return prog;
            }

            static const TwkGLF::GLProgram* arrowShapeGLProgram()
            {
                static const TwkGLF::GLProgram* prog = nullptr;
                if (!prog)
                {
                    auto src = buildShapeShaderSrc(shape_arrow_frag);
                    prog = getShapeProgram(src.vert, src.frag);
                    if (!prog)
                        std::cerr << "[ShapeSDF] arrow shader failed to compile\n";
                }
                return prog;
            }

            static const TwkGLF::GLProgram* lineShapeGLProgram()
            {
                static const TwkGLF::GLProgram* prog = nullptr;
                if (!prog)
                {
                    auto src = buildShapeShaderSrc(shape_line_frag);
                    prog = getShapeProgram(src.vert, src.frag);
                    if (!prog)
                        std::cerr << "[ShapeSDF] line shader failed to compile\n";
                }
                return prog;
            }

            /// Render a bounding-box quad for an SDF shape, copying the current
            /// composite texture into the render FBO first (same ping-pong pattern
            /// as executeQuad for Rectangle/Quad).
            ///
            /// The vertex shader expects positions in world space (same space as the
            /// shape's min/max / startPos / endPos coordinates).  uTransform maps
            /// from world space to clip space using the existing projMatrix *
            /// modelviewMatrix (a 4×4 that we flatten to a 3×3 for the 2-D shader).
            ///
            /// @param prog         Compiled SDF shader program
            /// @param context      CommandContext from the render loop
            /// @param quadVerts    8 floats: (x0,y0) (x1,y0) (x1,y1) (x0,y1) quad corners
            /// @param setUniforms  Callback to set shape-specific uniforms on the pipeline
            static void executeShapeQuad(const TwkGLF::GLProgram* prog, CommandContext& context, const float* quadVerts,
                                         std::function<void(TwkGLF::GLPipeline*, GLuint)> setUniforms)
            {
                if (!prog)
                    return;

                const GLFBO* textureFBO = context.currentTexture;
                const GLFBO* currentFBO = context.currentRender;
                const float w = currentFBO->width();
                const float h = currentFBO->height();

                GLState* glState = context.glState;

                // Copy the current composited image into the render FBO
                textureFBO->copyTo(currentFBO);
                currentFBO->bind();

                GLPipeline* glPipeline = glState->useGLProgram(prog);
                const GLuint programId = static_cast<GLuint>(prog->programId());

                // Build the 3×3 transform that maps from world space to clip space.
                // projMatrix * modelviewMatrix is a 4×4; for 2-D shapes we only need
                // the x,y,w rows/columns (rows 0,1,3 × cols 0,1,3).
                const Mat44f& P = context.projMatrix;
                const Mat44f& M = context.modelviewMatrix;
                Mat44f PM = P * M;
                // GLSL mat3 is stored column-major: col0 = [PM(0,0), PM(1,0), 0],
                //                                   col1 = [PM(0,1), PM(1,1), 0],
                //                                   col2 = [PM(0,3), PM(1,3), 1]
                float uTransform[9] = {
                    PM(0, 0), PM(1, 0), 0.0f, // column 0
                    PM(0, 1), PM(1, 1), 0.0f, // column 1
                    PM(0, 3), PM(1, 3), 1.0f  // column 2 (translation)
                };
                float uClipIdentity[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};

                // mat3 uniforms must be set via glUniformMatrix3fv — there is no
                // size-9 variant in setUniformGLFloat.
                const GLint locTransform = prog->uniformLocation("uTransform");
                const GLint locClip = prog->uniformLocation("uClip");
                if (locTransform >= 0)
                    glUniformMatrix3fv(locTransform, 1, GL_FALSE, uTransform);
                if (locClip >= 0)
                    glUniformMatrix3fv(locClip, 1, GL_FALSE, uClipIdentity);

                glPipeline->setViewport(0, 0, w, h);

                setUniforms(glPipeline, programId);

                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                // Submit the bounding-box quad.  Layout: x,y per vertex.
                PrimitiveData databuffer((void*)quadVerts, nullptr, GL_QUADS, 4, 1, sizeof(float) * 8);
                std::vector<VertexAttribute> attributeInfo;
                attributeInfo.push_back(VertexAttribute(std::string("aPosition"), GL_FLOAT, 2, 0, 0));

                RenderPrimitives renderprimitives(glState->activeGLProgram(), databuffer, attributeInfo, glState->vboList());

                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(static_cast<GLint>(context.stencilBox[0]), static_cast<GLint>(context.stencilBox[1]),
                              static_cast<GLsizei>(context.stencilBox[2] - context.stencilBox[0]),
                              static_cast<GLsizei>(context.stencilBox[3] - context.stencilBox[1]));
                }
                renderprimitives.setupAndRender();
                if (context.hasStencil)
                    glDisable(GL_SCISSOR_TEST);

                glDisable(GL_BLEND);
                currentFBO->unbind();
            }

        } // anonymous namespace

        // ── ShapeRect ─────────────────────────────────────────────────────────

        void ShapeRect::execute(CommandContext& context) const
        {
            const TwkGLF::GLProgram* prog = rectShapeGLProgram();

            // Determine effective colours (ghost mode support)
            const auto* localCmd = dynamic_cast<const PaintIPNode::LocalCommand*>(this);
            const bool isGhost = localCmd && localCmd->ghostOn;
            const Color effectiveBorder = isGhost ? localCmd->ghostColor : borderColor;
            const Color effectiveInner = isGhost ? Color(localCmd->ghostColor.x, localCmd->ghostColor.y, localCmd->ghostColor.z,
                                                         innerColor.w > 0.0f ? localCmd->ghostColor.w : 0.0f)
                                                 : innerColor;

            // Bounding-box quad corners: BL BR TR TL
            float verts[8] = {min.x, min.y, max.x, min.y, max.x, max.y, min.x, max.y};

            const Vec2f center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
            const float width = max.x - min.x;
            const float height = max.y - min.y;
            const float bw = borderWidth;
            const float inner[4] = {effectiveInner.x, effectiveInner.y, effectiveInner.z, effectiveInner.w};
            const float border[4] = {effectiveBorder.x, effectiveBorder.y, effectiveBorder.z, effectiveBorder.w};
            const float cornerRadii[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            const float cx = center.x;
            const float cy = center.y;

            // RV renders directly to screen-resolution pixels, so DPR is 1.
            float dpr = 1.0f;

            executeShapeQuad(prog, context, verts,
                             [&](TwkGLF::GLPipeline* p, GLuint /*pid*/)
                             {
                                 float cArr[2] = {cx, cy};
                                 p->setUniformFloat("uCenter", 2, cArr);
                                 p->setUniformFloat("uWidth", 1, const_cast<float*>(&width));
                                 p->setUniformFloat("uHeight", 1, const_cast<float*>(&height));
                                 p->setUniformFloat("uCornerRadii", 4, const_cast<float*>(cornerRadii));
                                 p->setUniformFloat("uInnerColor", 4, const_cast<float*>(inner));
                                 p->setUniformFloat("uBorderColor", 4, const_cast<float*>(border));
                                 p->setUniformFloat("uBorderWidth", 1, const_cast<float*>(&bw));
                                 p->setUniformFloat("uDpr", 1, &dpr);
                             });
        }

        void ShapeRect::hash(ostream& o) const { o << min << max << innerColor << borderColor << borderWidth; }

        size_t ShapeRect::getType() const { return Command::ShapeRectType; }

        // ── ShapeEllipse ──────────────────────────────────────────────────────

        void ShapeEllipse::execute(CommandContext& context) const
        {
            const TwkGLF::GLProgram* prog = ellipseShapeGLProgram();

            const auto* localCmd = dynamic_cast<const PaintIPNode::LocalCommand*>(this);
            const bool isGhost = localCmd && localCmd->ghostOn;
            const Color effectiveBorder = isGhost ? localCmd->ghostColor : borderColor;
            const Color effectiveInner = isGhost ? Color(localCmd->ghostColor.x, localCmd->ghostColor.y, localCmd->ghostColor.z,
                                                         innerColor.w > 0.0f ? localCmd->ghostColor.w : 0.0f)
                                                 : innerColor;

            float verts[8] = {min.x, min.y, max.x, min.y, max.x, max.y, min.x, max.y};

            const Vec2f center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
            const float width = max.x - min.x;
            const float height = max.y - min.y;
            const float bw = borderWidth;
            const float inner[4] = {effectiveInner.x, effectiveInner.y, effectiveInner.z, effectiveInner.w};
            const float border[4] = {effectiveBorder.x, effectiveBorder.y, effectiveBorder.z, effectiveBorder.w};
            const float cx = center.x;
            const float cy = center.y;

            // RV renders directly to screen-resolution pixels, so DPR is 1.
            float dpr = 1.0f;

            executeShapeQuad(prog, context, verts,
                             [&](TwkGLF::GLPipeline* p, GLuint /*pid*/)
                             {
                                 float cArr[2] = {cx, cy};
                                 p->setUniformFloat("uCenter", 2, cArr);
                                 p->setUniformFloat("uWidth", 1, const_cast<float*>(&width));
                                 p->setUniformFloat("uHeight", 1, const_cast<float*>(&height));
                                 p->setUniformFloat("uInnerColor", 4, const_cast<float*>(inner));
                                 p->setUniformFloat("uBorderColor", 4, const_cast<float*>(border));
                                 p->setUniformFloat("uBorderWidth", 1, const_cast<float*>(&bw));
                                 p->setUniformFloat("uDpr", 1, &dpr);
                             });
        }

        void ShapeEllipse::hash(ostream& o) const { o << min << max << innerColor << borderColor << borderWidth; }

        size_t ShapeEllipse::getType() const { return Command::ShapeEllipseType; }

        // ── ShapeArrow ────────────────────────────────────────────────────────

        void ShapeArrow::execute(CommandContext& context) const
        {
            const TwkGLF::GLProgram* prog = arrowShapeGLProgram();

            const auto* localCmd = dynamic_cast<const PaintIPNode::LocalCommand*>(this);
            const bool isGhost = localCmd && localCmd->ghostOn;
            const Color effectiveBorder = isGhost ? localCmd->ghostColor : borderColor;
            const Color effectiveInner = isGhost ? Color(localCmd->ghostColor.x, localCmd->ghostColor.y, localCmd->ghostColor.z,
                                                         innerColor.w > 0.0f ? localCmd->ghostColor.w : 0.0f)
                                                 : innerColor;

            // Bounding box: expand by thickness + borderWidth on all sides
            const float margin = thickness * 3.0f + borderWidth; // 3× = head half-width (2.5×) + AA
            const float x0 = std::min(startPos.x, endPos.x) - margin;
            const float y0 = std::min(startPos.y, endPos.y) - margin;
            const float x1 = std::max(startPos.x, endPos.x) + margin;
            const float y1 = std::max(startPos.y, endPos.y) + margin;
            float verts[8] = {x0, y0, x1, y0, x1, y1, x0, y1};

            const float bw = borderWidth;
            const float thick = thickness;
            const float inner[4] = {effectiveInner.x, effectiveInner.y, effectiveInner.z, effectiveInner.w};
            const float border[4] = {effectiveBorder.x, effectiveBorder.y, effectiveBorder.z, effectiveBorder.w};
            const float start[2] = {startPos.x, startPos.y};
            const float end[2] = {endPos.x, endPos.y};

            // RV renders directly to screen-resolution pixels, so DPR is 1.
            float dpr = 1.0f;

            executeShapeQuad(prog, context, verts,
                             [&](TwkGLF::GLPipeline* p, GLuint /*pid*/)
                             {
                                 p->setUniformFloat("uStart", 2, const_cast<float*>(start));
                                 p->setUniformFloat("uEnd", 2, const_cast<float*>(end));
                                 p->setUniformFloat("uThickness", 1, const_cast<float*>(&thick));
                                 p->setUniformFloat("uInnerColor", 4, const_cast<float*>(inner));
                                 p->setUniformFloat("uBorderColor", 4, const_cast<float*>(border));
                                 p->setUniformFloat("uBorderWidth", 1, const_cast<float*>(&bw));
                                 p->setUniformFloat("uDpr", 1, &dpr);
                             });
        }

        void ShapeArrow::hash(ostream& o) const { o << startPos << endPos << innerColor << borderColor << thickness << borderWidth; }

        size_t ShapeArrow::getType() const { return Command::ShapeArrowType; }

        // ── ShapeLine ─────────────────────────────────────────────────────────

        void ShapeLine::execute(CommandContext& context) const
        {
            const TwkGLF::GLProgram* prog = lineShapeGLProgram();

            const auto* localCmd = dynamic_cast<const PaintIPNode::LocalCommand*>(this);
            const bool isGhost = localCmd && localCmd->ghostOn;
            const Color effectiveBorder = isGhost ? localCmd->ghostColor : borderColor;

            // Margin must be at least as large as the SDF AA band (0.001) plus
            // the border half-width so that the capsule silhouette and its AA
            // fringe are fully covered by the bounding quad.
            const float margin = std::max(borderWidth + 0.002f, 0.004f);
            const float x0 = std::min(startPos.x, endPos.x) - margin;
            const float y0 = std::min(startPos.y, endPos.y) - margin;
            const float x1 = std::max(startPos.x, endPos.x) + margin;
            const float y1 = std::max(startPos.y, endPos.y) + margin;
            float verts[8] = {x0, y0, x1, y0, x1, y1, x0, y1};

            const float bw = borderWidth;
            const float border[4] = {effectiveBorder.x, effectiveBorder.y, effectiveBorder.z, effectiveBorder.w};
            const float start[2] = {startPos.x, startPos.y};
            const float end[2] = {endPos.x, endPos.y};

            // RV renders directly to screen-resolution pixels, so DPR is 1.
            float dpr = 1.0f;

            executeShapeQuad(prog, context, verts,
                             [&](TwkGLF::GLPipeline* p, GLuint /*pid*/)
                             {
                                 p->setUniformFloat("uStart", 2, const_cast<float*>(start));
                                 p->setUniformFloat("uEnd", 2, const_cast<float*>(end));
                                 p->setUniformFloat("uBorderColor", 4, const_cast<float*>(border));
                                 p->setUniformFloat("uBorderWidth", 1, const_cast<float*>(&bw));
                                 p->setUniformFloat("uDpr", 1, &dpr);
                             });
        }

        void ShapeLine::hash(ostream& o) const { o << startPos << endPos << borderColor << borderWidth; }

        size_t ShapeLine::getType() const { return Command::ShapeLineType; }

        void renderPaintCommands(PaintContext& context)
        {
            //
            // TODO: respect the incoming pixel aspect ratio
            //

            if (context.commands.empty())
                return;

            if (!BrushTextureManager::instance().isLoaded())
                BrushTextureManager::instance().load();

            // fbo contains the render of the current image, only used by erase
            // strokes
            const GLFBO* fbo = context.initialRender;
            const IPImage* root = context.image;
            const size_t curCmdNum = context.commands.size();

            // get ready to pingpong
            const GLFBO* textureFBO = NULL;
            const GLFBO* currentFBO = NULL;
            const GLFBO* tempfbo1 = context.tempRender1; // holds current render
            const GLFBO* tempfbo2 = context.tempRender2;

            size_t count = 0;
            for (size_t i = 0; i < context.commands.size(); count++)
            {
                const Paint::Command* cmd = context.commands[i];
                if (cmd->getType() == Paint::Command::ExecuteAllBefore)
                {
                    i++;
                    continue;
                }

                if (count % 2 == 0)
                {
                    textureFBO = tempfbo1;
                    currentFBO = tempfbo2;
                }
                else
                {
                    textureFBO = tempfbo2;
                    currentFBO = tempfbo1;
                }

                glDisable(GL_STENCIL_TEST);
                TWK_GLDEBUG;
                glDisable(GL_CULL_FACE);
                TWK_GLDEBUG;
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                TWK_GLDEBUG;

                // draw
                const Mat44f& O = root->orientationMatrix;
                const Mat44f& MP = root->placementMatrix;
                const Mat44f I = (O * MP).inverted();
                const Mat44f model = root->imageMatrix;
                const Mat44f proj = root->projectionMatrix;

                CommandContext commandContext(proj, model, fbo, textureFBO, currentFBO, context.glState, context.hasStencil,
                                              context.stencilBox, root->width, root->height);

                //
                // to speed up the rendering, we execute all consecutive text
                // commands together
                //

                if (cmd->getType() == Paint::Command::Text)
                {
                    size_t notText = 0;
                    // find all consecutive text commands
                    for (notText = i + 1; notText < context.commands.size(); ++notText)
                    {
                        if (context.commands[notText]->getType() != Paint::Command::Text)
                            break;
                    }
                    const Paint::Text* tcmd = static_cast<const Paint::Text*>(cmd);
                    tcmd->setup(commandContext);
                    if (i < curCmdNum - 1)
                        notText = min(notText, curCmdNum - 1);
                    for (size_t j = i; j < notText; ++j)
                    {
                        context.commands[j]->execute(commandContext);
                    }
                    tcmd->cleanup(commandContext);
                    i = notText;
                }
                else
                {
                    cmd->execute(commandContext);
                    ++i;
                }
                // Update the cache AFTER command execution
                // We cache up to the second-to-last command in the batch (not the very last, which might still be dragging)
                // count is the number of commands rendered so far (1-indexed after increment)
                // Cache after rendering the second-to-last command: count == curCmdNum - 1
                bool shouldCache = context.updateCache && context.cachedfbo && (curCmdNum > 1) && (count == curCmdNum - 1);

                if (shouldCache)
                {
                    // Always save to cache from tempfbo1 to maintain consistent ping-pong state
                    // If currentFBO is tempfbo2, copy it to tempfbo1 first
                    if (currentFBO != tempfbo1)
                    {
                        currentFBO->copyTo(tempfbo1);
                    }
                    tempfbo1->copyTo(context.cachedfbo);
                    context.cacheUpdated = true;
                }
            }

            context.commandExecuted = count;

            // currentFBO contains the resulting content after rendering these
            // commands
            if (fbo && currentFBO)
                currentFBO->copyTo(fbo);
        }

    } // namespace Paint
} // namespace IPCore
