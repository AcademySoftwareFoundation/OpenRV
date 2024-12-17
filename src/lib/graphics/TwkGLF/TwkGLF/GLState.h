//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#pragma once

#ifndef __TwkGLF__GLSTATE__h__
#define __TwkGLF__GLSTATE__h__

#include <TwkGLF/GL.h>
#include <TwkGLF/GLProgram.h>
#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GLVBO.h>
#include <TwkGLF/GLPipeline.h>
#include <stack>
#include <map>

//----------------------------------------------------------------------
//
// the class manages a list of GLPipeline objects and the currently bound FBO.
// each GLPipeline object corresponds to a glProgram, provides access to
// (sets/gets) uniforms of this glProgram GLState is in charge of maintaining
// all the active GLPipeline objects, switch between GLPipeline objects, etc.
//
//
// all generation and deletion of textures are registered here
// so we can keep track of card memory
// NOTE we currently don't keep track of vbo memory because we only
// use them for rendering quads, which are neligible in terms of memory.
//

namespace TwkGLF
{

    class GLFBO;

    class GLState
    {
    public:
        //
        //  Types
        //
        typedef TwkGLF::GLFBO GLFBO;
        typedef TwkGLF::GLVBO GLVBO;
        typedef std::map<unsigned int, GLPipeline> PipelineMap;

        //
        //  FixedFunctionPipeline is intended to be allocated on the
        //  stack. The constructor will store the current program and
        //  switch to the GL fixed function pipeline. When destroyed, the
        //  previous program will be restored:
        //
        //  {
        //      FixedFunctionPipeline FFP(globalState);
        //      // render some stuff
        //  }
        //

        struct FixedFunctionPipeline
        {
            FixedFunctionPipeline(GLState* s)
                : state(s)
                , program(s->activeGLProgram())
            {
                glUseProgram(GLuint(0));
            }

            void setViewport(int x, int y, int w, int h)
            {
                glViewport(x, y, w, h);
            }

            ~FixedFunctionPipeline() { state->useGLProgram(program); }

            GLState* state;
            const GLProgram* program;
        };

        //----------------------------------------------------------------------
        // constructor
        GLState()
            : m_activeProgram(NULL)
            , m_totalTextureMemory(0)
        {
        } // fixed function pipeline by default

        //----------------------------------------------------------------------
        // GL prgoram
        GLPipeline* useGLProgram(const GLProgram* p);
        GLPipeline* activeGLPipeline();
        const GLProgram* activeGLProgram() const;
        GLPipeline* pipeline(const GLuint p);

        //----------------------------------------------------------------------
        // create/delete textures
        GLuint createGLTexture(size_t);
        void deleteGLTexture(const GLuint&);

        //----------------------------------------------------------------------
        GLVBO::GLVBOVector& vboList() { return m_vboList; }

        //----------------------------------------------------------------------
        void clearState();

    private:
        GLVBO::GLVBOVector m_vboList;
        const GLProgram* m_activeProgram;
        PipelineMap m_state; // maintains a list of all existing GLPipelines
        std::map<size_t, size_t> m_activeTextures;
        size_t m_totalTextureMemory; // do not include vbo
    };

} // namespace TwkGLF
#endif // __TwkGLF__GLSTATE__h__
