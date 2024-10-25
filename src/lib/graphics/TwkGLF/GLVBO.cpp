//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkGLF/GLVBO.h>

namespace TwkGLF {
using namespace std;


//
//  Returns a VBO for the renderer to use. if none available in
//  m_vboList, then make a new one
//
//  NOTE: VBO caching mechanism is single threaded only. 
//

GLVBO::GLVBO() 
{
    glGenBuffers(1, &m_vbo);
    m_available = true;
    m_totalBytes = 0;
}

GLVBO::~GLVBO()
{
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
}

//size is in bytes
void 
GLVBO::setupData(const GLvoid* data, GLenum type, size_t size)
{
    glBindBuffer(type, m_vbo);
    glBufferData(type, size, data, GL_STATIC_DRAW);
    glBindBuffer(type, 0);
    m_totalBytes = size;
}

bool 
GLVBO::isValid() const
{
    return (m_vbo != 0);
}

GLVBO* 
GLVBO::availableVBO(GLVBOVector& vbos)
{
    using namespace std;

    //
    //  Returns a VBO for the renderer to use. if none available in
    //  m_vboList, then make a new one
    //
    //  NOTE: VBO caching mechanism is single threaded only.
    //

    GLVBO* GLVBO::availableVBO(GLVBOVector& vbos)
    {
        GLVBO* newvbo = NULL;

        for (size_t i = 0; i < vbos.size(); ++i)
        {
            if (vbos[i]->isAvailable())
            {
                newvbo = vbos[i];
                newvbo->makeUnavailable();
                return newvbo;
            }
        }

        // nothing in the list is available, make new one
        newvbo = new GLVBO();
        vbos.push_back(newvbo);
        newvbo->makeUnavailable();

        return newvbo;
    }

} // namespace TwkGLF
