//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#pragma once

#ifndef __TwkGLF__GLPIPELINE__h__
#define __TwkGLF__GLPIPELINE__h__

#include <TwkGLF/GL.h>
#include <TwkGLF/GLProgram.h>
#include <stack>
#include <vector>

//----------------------------------------------------------------------
//
// A GLPipeline object corresponds to a glProgram (containing the glProgram as a
// member) its main purpose is to provides access to (sets/gets) a glProgram's
// uniforms NOTE unlike gl2, we do not maintain a stack of matrices and states
// for each glProgram before each rendering occurs, the client needs to decide
// the proper matrices/states to set
//

namespace TwkGLF
{

    class GLPipeline
    {
    public:
        //----------------------------------------------------------------------
        //
        // constructor
        //
        GLPipeline()
            : m_program(NULL)
            , m_projectionIsSet(false)
            , m_modelViewIsSet(false)
            , m_viewportIsSet(false)
        {
        }

        GLPipeline(const GLProgram* p)
            : m_program(p)
            , m_projectionIsSet(false)
            , m_modelViewIsSet(false)
            , m_viewportIsSet(false)
        {
            m_viewport[0] = m_viewport[1] = m_viewport[2] = m_viewport[3] = 0;
            // m_modelView and m_projection defaults to identity, see
            // TwkMath::Mat44f
        }

        //----------------------------------------------------------------------
        //
        // set common GL uniforms, these exist only for convenience
        //

        void setProjection(const TwkMath::Mat44f& p);
        void setModelview(const TwkMath::Mat44f& m);
        void setViewport(const int x, const int y, const int w, const int h);

        //----------------------------------------------------------------------
        //
        // set GL uniforms
        //
        void setUniformFloat(const char* name, size_t no,
                             GLfloat* data); // send GL the uniform values to
                                             // the current active glProgram
        void setUniformInt(const char* name, size_t no, GLint* data);
        void setUniformMatrix(const char* name, TwkMath::Mat44f& data);

        //----------------------------------------------------------------------
        //
        // this only exist for GL2 compatibility (for fixed function pipeline)
        //
        void useCurrentProjectionGL2() const;
        void useCurrentModelviewGL2() const;

        //----------------------------------------------------------------------
        //
        // get common GL uniforms, these exist only for convenience
        //
        TwkMath::Mat44f currentProjection() const;
        TwkMath::Mat44f currentModelview() const;
        void currentViewport(float (&v)[4]) const;

        //----------------------------------------------------------------------
        //
        // get GL uniforms
        //
        void uniformFloat(const char* name, GLfloat* data) const;
        void uniformInt(const char* name, GLint* data) const;
        void uniformMatrix(const char* name, GLfloat* data) const;

        //----------------------------------------------------------------------
        // miscellaneous
        void use() const { glUseProgram(m_program->programId()); }

        unsigned int programId() const { return m_program->programId(); }

        const GLProgram* glProgram() const { return m_program; }

    private:
        // check if a uniform of name 'name' exists in the current gl program
        bool uniformIsValid(const std::string& name) const;

    private:
        const GLProgram* m_program;
        bool m_projectionIsSet; // after the first call to glUniform("projmat")
                                // this will be true
        bool m_modelViewIsSet;
        bool m_viewportIsSet;
        // the following 3 are maintained only for GL 2 compatibility, see
        // UseCurrentProjectionGL2()
        TwkMath::Mat44f m_projection;
        TwkMath::Mat44f m_modelView;
        float m_viewport[4];
    };

} // namespace TwkGLF
#endif // __TwkGLF__GLPIPELINE__h__
