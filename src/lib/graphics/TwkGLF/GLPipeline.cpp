//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkGLF/GLPipeline.h>

namespace TwkGLF
{

    using namespace std;
    using namespace TwkMath;

    //----------------------------------------------------------------------
    // set common gl uniforms
    void GLPipeline::setProjection(const Mat44f& p)
    {
        // only change gl state if necessary
        if (m_projection != p || !m_projectionIsSet)
        {
            m_projection = p;
            setUniformMatrix("projMatrix", m_projection);
            m_projectionIsSet = true;
            TWK_GLDEBUG;
        }
    }

    void GLPipeline::setModelview(const Mat44f& m)
    {
        // only change gl state if necessary
        if (m_modelView != m || !m_modelViewIsSet)
        {
            m_modelView = m;
            setUniformMatrix("modelviewMatrix", m_modelView);
            m_modelViewIsSet = true;
            TWK_GLDEBUG;
        }
    }

    void GLPipeline::setViewport(const int x, const int y, const int w,
                                 const int h)
    {
        // only change gl state if necessary
        // if (m_viewport[0] != x || m_viewport[1] != y || m_viewport[2] != w ||
        // m_viewport[3] != h
        //|| !m_viewportIsSet)
        //{
        m_viewport[0] = x;
        m_viewport[1] = y;
        m_viewport[2] = w;
        m_viewport[3] = h;

        glViewport(x, y, w, h);
        m_viewportIsSet = true;
        TWK_GLDEBUG;
        //}
    }

    //----------------------------------------------------------------------
    // these only exist for compatibility with fixed function pipeline
    void GLPipeline::useCurrentProjectionGL2() const
    {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrix(m_projection);
    }

    void GLPipeline::useCurrentModelviewGL2() const
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrix(m_modelView);
    }

    //----------------------------------------------------------------------
    //
    // set uniforms of type float, int, or matrix
    //
    void GLPipeline::setUniformFloat(const char* name, size_t no, GLfloat* data)
    {
#ifdef NDEBUG
        try
        {
            uniformIsValid(name);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: failed to set float array uniform in glProgram "
                 << m_program->programId() << ": " << exc.what() << endl;
            return;
        }
#endif
        setUniformGLFloat(m_program->programId(),
                          m_program->uniformLocation(name), no, data);
    }

    void GLPipeline::setUniformInt(const char* name, size_t no, GLint* data)
    {
#ifdef NDEBUG
        try
        {
            uniformIsValid(name);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: failed to set int array uniform in glProgram "
                 << m_program->programId() << ": " << exc.what() << endl;
            return;
        }
#endif
        setUniformGLInt(m_program->programId(),
                        m_program->uniformLocation(name), no, data);
    }

    void GLPipeline::setUniformMatrix(const char* name, TwkMath::Mat44f& data)
    {
#ifdef NDEBUG
        try
        {
            uniformIsValid(name);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: failed to set matrix uniform in glProgram "
                 << m_program->programId() << ": " << exc.what() << endl;
            return;
        }
#endif
        setUniformGLMatrix(m_program->programId(),
                           m_program->uniformLocation(name), &(data[0][0]));
    }

    //----------------------------------------------------------------------
    //
    // get GL uniforms, these functions are very expensive. should call only
    // when necessary
    //
    void GLPipeline::uniformFloat(const char* name, GLfloat* data) const
    {
#ifdef NDEBUG
        try
        {
            uniformIsValid(name);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: failed to read matrix uniform in glProgram "
                 << m_program->programId() << ": " << exc.what() << endl;
            return;
        }
#endif
        getUniformGLFloat(m_program->programId(),
                          m_program->uniformLocation(name), data);
    }

    void GLPipeline::uniformInt(const char* name, GLint* data) const
    {
#ifdef NDEBUG
        try
        {
            uniformIsValid(name);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: failed to read int array uniform in glProgram "
                 << m_program->programId() << ": " << exc.what() << endl;
            return;
        }
#endif
        getUniformGLInt(m_program->programId(),
                        m_program->uniformLocation(name), data);
    }

    void GLPipeline::uniformMatrix(const char* name, GLfloat* data) const
    {
#ifdef NDEBUG
        try
        {
            uniformIsValid(name);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: failed to read int array uniform in glProgram "
                 << m_program->programId() << ": " << exc.what() << endl;
            return;
        }
#endif
        getUniformGLMatrix(m_program->programId(),
                           m_program->uniformLocation(name), data);
    }

    //----------------------------------------------------------------------
    //
    // get common gl uniforms
    //
    Mat44f GLPipeline::currentProjection() const { return m_projection; }

    Mat44f GLPipeline::currentModelview() const { return m_modelView; }

    void GLPipeline::currentViewport(float (&v)[4]) const
    {
        for (size_t i = 0; i < 4; ++i)
            v[i] = m_viewport[i];
    }

    //----------------------------------------------------------------------
    // private
    // check if a uniform of name 'name' exists in this gl program
    bool GLPipeline::uniformIsValid(const string& name) const
    {
        return m_program->uniformLocation(name) != -1;
    }

} // namespace TwkGLF
