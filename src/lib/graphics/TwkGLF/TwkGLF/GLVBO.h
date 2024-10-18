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


namespace TwkGLF {

//----------------------------------------------------------------------
//
//  VBO
//
class GLVBO
{
public:
    typedef std::vector<GLVBO*> GLVBOVector;

    GLVBO();
    ~GLVBO();

    //size is in bytes
    void setupData(const GLvoid* data, GLenum type, size_t size);

    bool isValid() const;
    
    const GLuint vbo() const { return m_vbo; }

    const bool isAvailable() const { return m_available; }
    void makeAvailable() { m_available = true; }
    void makeUnavailable() { m_available = false; }

    size_t totalSize() const { return m_totalBytes; }
    
    static GLVBO* availableVBO(GLVBOVector& vbos);

private:
    GLuint             m_vbo;
    bool               m_available;
    size_t             m_totalBytes;
};

}

#endif // __TwkGLF__GLBUFFER__h__

