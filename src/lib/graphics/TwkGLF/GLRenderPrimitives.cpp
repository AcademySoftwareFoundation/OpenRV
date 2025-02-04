//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkGLF/GLVBO.h>

#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>

namespace TwkGLF
{

    namespace
    {

        //
        //  Template function here replaces macro
        //

        template <typename T> char* bufferOffset(T i)
        {
            return reinterpret_cast<char*>(NULL) + i;
        }

    } // namespace

    RenderPrimitives::RenderPrimitives(const GLProgram* p, PrimitiveData& data,
                                       VertexAttributeList& attributes,
                                       GLVBO::GLVBOVector& vbos)
        : m_program(p)
        , m_primitiveData(data)
        , m_attributeList(attributes)
        , m_dataVBO(NULL)
        , m_indexVBO(NULL)
    {
        init(vbos);
    }

    RenderPrimitives::~RenderPrimitives() { clear(); }

    void RenderPrimitives::init(GLVBO::GLVBOVector& vbos)
    {
        m_hasIndices = m_primitiveData.m_indexBuffer != NULL;

        // main VBO
        m_dataVBO = GLVBO::availableVBO(vbos);

        // indices VBO if necessary
        if (m_hasIndices)
        {
            m_indexVBO = GLVBO::availableVBO(vbos);
            m_indexDataType = GL_UNSIGNED_INT;
        }

        // initial setup
        switch (m_primitiveData.m_primitiveType)
        {
        case GL_TRIANGLES:
            m_verticesPerPrimitive = 3;
            break;
        case GL_QUADS:
            m_verticesPerPrimitive = 4;
            break;
        case GL_POINTS:
            m_verticesPerPrimitive = 1;
            break;
        }
    }

    void RenderPrimitives::pushDataToBuffer()
    {
        m_dataVBO->setupData(m_primitiveData.m_dataBuffer, GL_ARRAY_BUFFER,
                             m_primitiveData.m_size);

        if (m_hasIndices)
        {
            m_indexVBO->setupData(
                m_primitiveData.m_indexBuffer, GL_ELEMENT_ARRAY_BUFFER,
                sizeof(unsigned int) * m_primitiveData.m_primitiveNo
                    * m_verticesPerPrimitive);
        }

        TWK_GLDEBUG;
    }

    void RenderPrimitives::bindBuffer() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_dataVBO->vbo());
        if (m_hasIndices)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexVBO->vbo());
        }
    }

    void RenderPrimitives::unbindBuffer() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        TWK_GLDEBUG;
        if (m_hasIndices)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            TWK_GLDEBUG;
        }
    }

    void RenderPrimitives::render() const
    {
        HOP_CALL(glFinish();)
        HOP_PROF_FUNC();

        {
            HOP_CALL(glFinish();)
            HOP_PROF("RenderPrimitives::render - bindBuffer");

            bindBuffer();

            HOP_CALL(glFinish();)
        }

        for (size_t i = 0; i < m_attributeList.size(); ++i)
        {
            int loc = m_program->attribLocation(m_attributeList[i].m_name);
            if (loc >= 0)
            {
                glVertexAttribPointer(
                    loc, m_attributeList[i].m_dimension,
                    m_attributeList[i].m_dataType, GL_FALSE,
                    m_attributeList[i].m_stride,
                    bufferOffset(m_attributeList[i].m_offset));

                glEnableVertexAttribArray(loc);
            }
            else
            {
                std::cerr << "ERROR: Invalid attribute location in "
                             "RenderPrimitives::render."
                          << std::endl;
            }
        }
        TWK_GLDEBUG;

        if (m_hasIndices)
        {
            glDrawElements(m_primitiveData.m_primitiveType,
                           m_primitiveData.m_primitiveNo
                               * m_verticesPerPrimitive,
                           m_indexDataType, NULL);
            TWK_GLDEBUG;
        }
        else
        {
            HOP_CALL(glFinish();)
            HOP_PROF("RenderPrimitives::render - glDrawArrays");

            glDrawArrays(m_primitiveData.m_primitiveType, 0,
                         m_primitiveData.m_vertexNo);
            TWK_GLDEBUG;

            HOP_CALL(glFinish();)
        }

        for (size_t i = 0; i < m_attributeList.size(); ++i)
        {
            int loc = m_program->attribLocation(m_attributeList[i].m_name);
            if (loc >= 0)
            {
                glDisableVertexAttribArray(loc);
            }
            else
            {
                std::cerr << "ERROR: Invalid attribute location in "
                             "RenderPrimitives::render."
                          << std::endl;
            }
            TWK_GLDEBUG;
        }
        unbindBuffer();
        TWK_GLDEBUG;
        HOP_CALL(glFinish();)
    }

    void RenderPrimitives::clear()
    {
        m_hasIndices = false;
        m_attributeList.clear();

        if (m_dataVBO != NULL)
        {
            m_dataVBO->makeAvailable();
        }

        if (m_indexVBO != NULL)
        {
            m_indexVBO->makeAvailable();
        }
    }

    void RenderPrimitives::setupAndRender()
    {
        pushDataToBuffer();
        render();
        clear();
    }

} // namespace TwkGLF
