//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/PaintCommand.h>
#include <TwkMath/Function.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLPipeline.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/BasicGLProgram.h>
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Frustum.h>
#include <TwkGLText/TwkGLText.h>
#include <stl_ext/string_algo.h>

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

        void Font::setColor(float r, float g, float b, float a) const
        {
            GLtext::color(r, g, b, a);
        }

        pair<float, float> Font::computeText(const string& text, string& t,
                                             float space, string origin) const
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

        float Font::globalAscenderHeight() const
        {
            return GLtext::globalAscenderHeight();
        }

        float Font::globalDescenderHeight() const
        {
            return GLtext::globalDescenderHeight();
        }

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
            GLPipeline* glPipeline =
                glState->useGLProgram(textureRectGLProgram());

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
            float data[] = {0, 0, 0,     0,     w, 0, w - 1, 0,
                            w, h, w - 1, h - 1, 0, h, 0,     h - 1};
            PrimitiveData buffer(data, NULL, GL_QUADS, 4, 1,
                                 16 * sizeof(float));
            std::vector<VertexAttribute> attributeInfo;

            attributeInfo.push_back(
                VertexAttribute(std::string("in_Position"), GL_FLOAT, 2,
                                2 * sizeof(float), 4 * sizeof(float)));

            attributeInfo.push_back(VertexAttribute(std::string("in_TexCoord0"),
                                                    GL_FLOAT, 2, 0,
                                                    4 * sizeof(float)));

            RenderPrimitives renderprimitives(glState->activeGLProgram(),
                                              buffer, attributeInfo,
                                              glState->vboList());

            renderprimitives.setupAndRender();

            textureFBO->unbindColorTexture();

            //  do paint
            if (brush == "gauss")
            {
                glPipeline =
                    glState->useGLProgram(softPaintOldReplaceGLProgram());
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
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                                GL_ONE_MINUS_SRC_ALPHA);

            RenderPrimitives renderprimitives2(glState->activeGLProgram(),
                                               primitives, primitiveAttributes,
                                               glState->vboList());
            renderprimitives2.setupAndRender();

            // cleanup
            currentFBO->unbind();

            glDisable(GL_BLEND);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
        }

        void PolyLine::execute(CommandContext& context) const
        {
            if (!npoints)
                return;

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

            GLPipeline* glPipeline =
                glState->useGLProgram(textureRectGLProgram());
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
            float data[] = {0, 0, 0,     0,     w, 0, w - 1, 0,
                            w, h, w - 1, h - 1, 0, h, 0,     h - 1};
            PrimitiveData buffer(data, NULL, GL_QUADS, 4, 1,
                                 16 * sizeof(float));
            std::vector<VertexAttribute> attributeInfo2;
            attributeInfo2.push_back(
                VertexAttribute(std::string("in_Position"), GL_FLOAT, 2,
                                2 * sizeof(float), 4 * sizeof(float)));

            attributeInfo2.push_back(
                VertexAttribute(std::string("in_TexCoord0"), GL_FLOAT, 2, 0,
                                4 * sizeof(float)));
            RenderPrimitives renderprimitives2(glState->activeGLProgram(),
                                               buffer, attributeInfo2,
                                               glState->vboList());
            renderprimitives2.setupAndRender();

            textureFBO->unbindColorTexture();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);

            //////////////////////////////////////////////////////////////////////////
            // render paint onto the background
            //////////////////////////////////////////////////////////////////////////
            switch (mode)
            {
            case EraseMode:
                if (brush != "gauss")
                    glPipeline = glState->useGLProgram(paintEraseGLProgram());
                else
                    glPipeline =
                        glState->useGLProgram(softPaintEraseGLProgram());
                break;
            case ScaleMode:
            case GradientScaleMode:
                if (brush != "gauss")
                    glPipeline = glState->useGLProgram(paintScaleGLProgram());
                else
                    glPipeline =
                        glState->useGLProgram(softPaintScaleGLProgram());
                break;
            case CloneMode:
                if (brush != "gauss")
                    glPipeline = glState->useGLProgram(paintCloneGLProgram());
                else
                    glPipeline =
                        glState->useGLProgram(softPaintCloneGLProgram());
                break;
            case TessellateMode:
                glPipeline = glState->useGLProgram(paintTessellateGLProgram());
                break;
            case OverMode:
            default:
                if (brush != "gauss")
                    glPipeline = glState->useGLProgram(paintReplaceGLProgram());
                else
                    glPipeline =
                        glState->useGLProgram(softPaintReplaceGLProgram());
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
            Color pcolor = color;
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
                glScissor(context.stencilBox[0], context.stencilBox[1],
                          context.stencilBox[2] - context.stencilBox[0],
                          context.stencilBox[3] - context.stencilBox[1]);
            }
            glEnable(GL_BLEND);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                                GL_ONE_MINUS_SRC_ALPHA);
            RenderPrimitives renderprimitives3(glState->activeGLProgram(),
                                               primitives, primitiveAttributes,
                                               glState->vboList());
            renderprimitives3.setupAndRender();
            if (context.hasStencil)
                glDisable(GL_SCISSOR_TEST);

            // clean up
            if (mode == EraseMode)
                originalFBO->unbindColorTexture();
            else if (mode == ScaleMode || mode == CloneMode)
                textureFBO->unbindColorTexture();

            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_BLEND);

            currentFBO->unbind();
            glState->useGLProgram(defaultGLProgram());
        }

        void PolyLine::hash(ostream& o) const
        {
            o << (void*)points << npoints << width << smoothingWidth << splat
              << widths << color << brush << join << cap << mode << debug;
        }

        PolyLine::HashValue PolyLine::hashValue() const
        {
            ostringstream o;

            o << (void*)points << npoints << width << smoothingWidth << splat
              << widths << color << brush << join << cap << mode << debug;

            std::string id = o.str();

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

            if (widths)
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

            path.computeGeometry(join, cap, Path::RadiallySymmetric,
                                 Path::QualityAlgorithm, true, smoothingWidth,
                                 splat);

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
                    primitiveData[2 * vertexno + 2 * i + 0] =
                        (float)opoints[i].x;
                    primitiveData[2 * vertexno + 2 * i + 1] =
                        (float)opoints[i].y;
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
                    primitiveData[6 * i + 0] =
                        (float)tpoints[triangles[i][0]].x;
                    primitiveData[6 * i + 1] =
                        (float)tpoints[triangles[i][0]].y;
                    primitiveData[6 * i + 2] =
                        (float)tpoints[triangles[i][1]].x;
                    primitiveData[6 * i + 3] =
                        (float)tpoints[triangles[i][1]].y;
                    primitiveData[6 * i + 4] =
                        (float)tpoints[triangles[i][2]].x;
                    primitiveData[6 * i + 5] =
                        (float)tpoints[triangles[i][2]].y;
                }
                size_t offset = 6 * trino;
                for (size_t i = 0; i < trino; i++)
                {
                    primitiveData[offset + 6 * i + 0] =
                        (float)opoints[triangles[i][0]].x;
                    primitiveData[offset + 6 * i + 1] =
                        (float)opoints[triangles[i][0]].y;
                    primitiveData[offset + 6 * i + 2] =
                        (float)opoints[triangles[i][1]].x;
                    primitiveData[offset + 6 * i + 3] =
                        (float)opoints[triangles[i][1]].y;
                    primitiveData[offset + 6 * i + 4] =
                        (float)opoints[triangles[i][2]].x;
                    primitiveData[offset + 6 * i + 5] =
                        (float)opoints[triangles[i][2]].y;
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
            primitives.m_indexBuffer =
                mode == TessellateMode ? 0 : &(primitiveIndices[0]);
            primitives.m_primitiveType = GL_TRIANGLES;
            primitives.m_vertexNo = vertexno;
            primitives.m_primitiveNo = trino;
            primitives.m_size = sizeof(float) * vbodatasize;

            primitiveAttributes.clear();
            primitiveAttributes.push_back(VertexAttribute(
                std::string("in_TexCoord0"), GL_FLOAT, 2, 0, 0)); // texture
            primitiveAttributes.push_back(
                VertexAttribute(std::string("in_Position"), GL_FLOAT, 2,
                                2 * vertexno * sizeof(float), 0)); // vertex
            if (mode == GradientScaleMode)
            {
                primitiveAttributes.push_back(VertexAttribute(
                    std::string("in_DirectionCoord"), GL_FLOAT, 1,
                    4 * vertexno * sizeof(float), 0)); // direction coord
            }
            else if (mode == TessellateMode)
            {
                primitiveAttributes.push_back(
                    VertexAttribute(std::string("in_UniformColor"), GL_FLOAT, 4,
                                    4 * vertexno * sizeof(float),
                                    0)); // color for each triangle
            }

            built = true;
        }

        PushFrameBuffer::PushFrameBuffer() {}

        void PushFrameBuffer::execute(CommandContext&) const {}

        void PushFrameBuffer::hash(ostream& o) const { o << "pu"; }

        size_t PushFrameBuffer::getType() const
        {
            return Command::PushFrameBuffer;
        }

        PopFrameBuffer::PopFrameBuffer() {}

        void PopFrameBuffer::execute(CommandContext&) const {}

        void PopFrameBuffer::hash(ostream& o) const { o << "po"; }

        size_t PopFrameBuffer::getType() const
        {
            return Command::PopFrameBuffer;
        }

        void Text::setup(CommandContext& context)
        {
            const GLFBO* fbo = context.currentRender;
            context.currentTexture->copyTo(fbo);
            fbo->bind();

            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                                GL_ONE_MINUS_SRC_ALPHA);

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

            // render
            const GLFBO* fbo = context.currentRender;

            Font* f = new Font(font);
            if (!f->initialized())
                f->init();
            f->setSize(ptsize);
            f->setColor(color.x, color.y, color.z, color.w);

            const float w = fbo->width();
            const float h = fbo->height();

            Matrix T, S;
            S.makeScale(Vec3f(scale, scale, scale));

            double x = pos.x / scale;
            double y = pos.y / scale;
            T.makeTranslation(Vec3f(x, y, 0.0f));

            GLState* glState = context.glState;
            GLPipeline* glPipeline = glState->useGLProgram(defaultGLProgram());

            glPipeline->setProjection(context.projMatrix);
            glPipeline->setModelview(context.modelviewMatrix * S * T);

            glPipeline->useCurrentProjectionGL2();
            glPipeline->useCurrentModelviewGL2();

            GLState::FixedFunctionPipeline FFP(glState);
            FFP.setViewport(0, 0, w, h);

            string outtext;
            pair<float, float> loc =
                f->computeText(text, outtext, spacing, origin);

            if (context.hasStencil)
            {
                glEnable(GL_SCISSOR_TEST);
                glScissor(context.stencilBox[0], context.stencilBox[1],
                          context.stencilBox[2] - context.stencilBox[0],
                          context.stencilBox[3] - context.stencilBox[1]);
            }
            GLtext::writeAtNL(loc.first, loc.second, outtext.c_str(), spacing);
            if (context.hasStencil)
                glDisable(GL_SCISSOR_TEST);
            delete f;
        }

        void Text::hash(ostream& o) const
        {
            o << pos << color << text << font << origin << ptsize << scale
              << rotation << spacing;
        }

        size_t Text::getType() const { return Command::Text; }

        namespace
        {

            void executeQuad(CommandContext& context, float* points,
                             Color color, GLint mode)
            {
                glEnable(GL_BLEND);
                glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                    GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                Color pcolor = color;
                GLState* glState = context.glState;
                GLPipeline* glPipeline =
                    glState->useGLProgram(defaultGLProgram());
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

                PrimitiveData databuffer((void*)points, NULL, mode, 4, 1,
                                         sizeof(float) * 8);
                std::vector<VertexAttribute> attributeInfo;
                attributeInfo.push_back(VertexAttribute(
                    std::string("in_Position"), GL_FLOAT, 2, 0, 0));
                RenderPrimitives renderprimitives(glState->activeGLProgram(),
                                                  databuffer, attributeInfo,
                                                  glState->vboList());
                if (context.hasStencil)
                {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(context.stencilBox[0], context.stencilBox[1],
                              context.stencilBox[2] - context.stencilBox[0],
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
            float data[] = {pos.x, pos.y,         pos.x + width,
                            pos.y, pos.x + width, pos.y + height,
                            pos.x, pos.y + height};

            executeQuad(context, data, color, GL_QUADS);
        }

        void Rectangle::hash(ostream& o) const
        {
            o << pos << height << width << color;
        }

        size_t Rectangle::getType() const { return Command::Rectangle; }

        void Quad::execute(CommandContext& context) const
        {
            GLint dm = (drawMode == SolidMode) ? GL_QUADS : GL_LINE_LOOP;

            executeQuad(context, (float*)points, color, dm);
        }

        void Quad::hash(ostream& o) const
        {
            o << points[0] << points[1] << points[2] << points[3] << drawMode
              << color;
        }

        size_t Quad::getType() const { return Command::Quad; }

        void ExecuteAllBefore::execute(CommandContext& context) const {}

        void ExecuteAllBefore::hash(ostream& o) const {}

        size_t ExecuteAllBefore::getType() const
        {
            return Command::ExecuteAllBefore;
        }

        void renderPaintCommands(PaintContext& context)
        {
            //
            // TODO: respect the incoming pixel aspect ratio
            //

            if (context.commands.empty())
                return;

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
                //
                // update the cache, only up to the second to the last cmd we
                // don't cache the render with the very last cmd because doing
                // so will cause the last cmd to be rendered twice, and in
                // case of alpha not 0, or 1, it will not be correct
                //

                if (context.updateCache && context.cachedfbo
                    && (context.commands[i] == context.lastCommand))
                {
                    if (i > 0)
                    {
                        currentFBO->copyTo(context.cachedfbo);
                    }
                    else
                    {
                        tempfbo1->copyTo(context.cachedfbo);
                    }
                    context.cacheUpdated = true;
                }

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

                CommandContext commandContext(
                    proj, model, fbo, textureFBO, currentFBO, context.glState,
                    context.hasStencil, context.stencilBox);

                //
                // to speed up the rendering, we execute all consecutive text
                // commands together
                //

                if (cmd->getType() == Paint::Command::Text)
                {
                    size_t notText = 0;
                    // find all consecutive text commands
                    for (notText = i + 1; notText < context.commands.size();
                         ++notText)
                    {
                        if (context.commands[notText]->getType()
                            != Paint::Command::Text)
                            break;
                    }
                    const Paint::Text* tcmd =
                        static_cast<const Paint::Text*>(cmd);
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
            }

            context.commandExecuted = count;

            // currentFBO contains the resulting content after rendering these
            // commands
            if (fbo && currentFBO)
                currentFBO->copyTo(fbo);
        }

    } // namespace Paint
} // namespace IPCore
