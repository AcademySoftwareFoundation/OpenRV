//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkGLF/GLProgram.h>
#include <vector>

namespace TwkGLF
{

    using namespace std;

    void GLProgram::collectUniforms()
    {
        // get all the active uniforms in this gl program, and their locations.
        // cache these for future use.
        // we want to avoid calling glGetUniformLocation at run time as much as
        // possible, because these commands force the system to execute all the
        // queued gl calls before it answers the query.
        int activeUniforms = 0;
        int nameLength = 0;
        int uniformSize = 0;
        GLenum uniformType = GL_ZERO;
        char uniformName[256];
        GLuint pid = programId();
        glGetProgramiv(pid, GL_ACTIVE_UNIFORMS, &activeUniforms);
        for (size_t i = 0; i < activeUniforms; ++i)
        {
            glGetActiveUniform(pid, i, sizeof(uniformName) - 1, &nameLength,
                               &uniformSize, &uniformType, uniformName);
            uniformName[nameLength] = 0;
            GLuint loc = glGetUniformLocation(pid, uniformName);
            m_uniformLocationMap[string(uniformName)] = loc;
        }
    }

    void GLProgram::collectAttribs()
    {
        //
        // get all the active attributes (in_Position, etc) in this gl
        // program, and their locations.  cache these for future use.  we
        // want to avoid calling glGetAttribLocation at run time as much
        // as possible, because these commands force the system to execute
        // all the queued gl calls before it answers the query.
        //

        int numActiveAttribs = 0;
        int nameLength = 0;
        int attribSize = 0;
        GLenum attribType = GL_ZERO;
        GLuint pid = programId();
        GLint maxNameSize = 0;

        glGetProgramiv(pid, GL_ACTIVE_ATTRIBUTES, &numActiveAttribs);
        glGetProgramiv(pid, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameSize);

        vector<char> nameBuffer(maxNameSize + 1);

        for (size_t i = 0; i < numActiveAttribs; ++i)
        {
            glGetActiveAttrib(pid, i, nameBuffer.size(), &nameLength,
                              &attribSize, &attribType, &nameBuffer.front());

            nameBuffer[nameLength] = 0;

            GLuint loc = glGetAttribLocation(pid, &nameBuffer.front());

            m_attribLocationMap[&nameBuffer.front()] = loc;
        }
    }

    unsigned int GLProgram::uniformLocation(const string& name) const
    {
        UniformLocationMap::const_iterator it = m_uniformLocationMap.find(name);

        if (it != m_uniformLocationMap.end())
        {
            return it->second;
        }

        return -1;
    }

    unsigned int GLProgram::attribLocation(const string& name) const
    {
        AttribLocationMap::const_iterator it = m_attribLocationMap.find(name);

        if (it != m_attribLocationMap.end())
        {
            return it->second;
        }

        return -1;
    }

    void GLProgram::use() const { glUseProgram(m_programId); }

} // namespace TwkGLF
