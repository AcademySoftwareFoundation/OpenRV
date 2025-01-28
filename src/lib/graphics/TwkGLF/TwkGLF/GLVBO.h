//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#pragma once
#ifndef __TwkGLF__GLBUFFER__h__
#define __TwkGLF__GLBUFFER__h__

#include <TwkGLF/GL.h>
#include <vector>

namespace TwkGLF
{

    //----------------------------------------------------------------------
    //
    //  VBO
    //
    class GLVBO
    {
    public:
        typedef std::vector<GLVBO*> GLVBOVector;

        GLVBO()
        {
            glGenBuffers(1, &m_vbo);
            m_available = true;
            m_totalBytes = 0;
        }

        ~GLVBO()
        {
            if (m_vbo != 0)
                glDeleteBuffers(1, &m_vbo);
        }

        // size is in bytes
        void setupData(const GLvoid* data, GLenum type, size_t size)
        {
            glBindBuffer(type, m_vbo);
            glBufferData(type, size, data, GL_STATIC_DRAW);
            glBindBuffer(type, 0);
            m_totalBytes = size;
        }

        bool isValid() const { return (m_vbo != 0); }

        const GLuint vbo() const { return m_vbo; }

        const bool isAvailable() const { return m_available; }

        void makeAvailable() { m_available = true; }

        void makeUnavailable() { m_available = false; }

        size_t totalSize() const { return m_totalBytes; }

        static GLVBO* availableVBO(GLVBOVector& vbos);

    private:
        GLuint m_vbo;
        bool m_available;
        size_t m_totalBytes;
    };

} // namespace TwkGLF

#endif // __TwkGLF__GLBUFFER__h__
