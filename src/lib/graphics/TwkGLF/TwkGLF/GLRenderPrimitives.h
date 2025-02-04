//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#pragma once
#ifndef __TwkGLF__GLRENDERPRIMITIVES__h__
#define __TwkGLF__GLRENDERPRIMITIVES__h__

#include <TwkGLF/GL.h>
#include <TwkGLF/GLVBO.h>
#include <TwkGLF/GLProgram.h>
#include <vector>
#include <iostream>

namespace TwkGLF
{

    //
    // this will describe a vertex attribute, e.g., in_Position of the vertex
    // shader, and necessary properties of this attribute, e.g. dataType is
    // float or double, dimension might be 2, 3, 4
    //
    struct VertexAttribute
    {
        VertexAttribute(std::string name, GLenum datatype, int dimension,
                        int offset, int stride)
            : m_name(name)
            , m_dataType(datatype)
            , m_dimension(dimension)
            , m_offset(offset)
            , m_stride(stride)
        {
        }

        // copy constructor
        VertexAttribute(const VertexAttribute& v)
            : m_name(v.m_name)
            , m_dataType(v.m_dataType)
            , m_dimension(v.m_dimension)
            , m_offset(v.m_offset)
            , m_stride(v.m_stride)
        {
        }

        std::string m_name;
        GLenum m_dataType;
        int m_stride; // see glVertexAttribPointer for meaning
        int m_offset;
        int m_dimension; // dimension of vertex, textcoord, etc.
    };

    //
    // this contains
    // --type of primitives: quads, tris, etc
    // --number of primitives, number of vertices, datatype (float, double,
    // etc.)
    // --data pointer (in PrimitiveData) of the primitives: includes vertex
    // positions, texCoord, normals, etc.
    // --data pointer (in PrimitiveIndexBuffer) of the indices (optional):
    // indexed rendering requires an array of indices,
    //   all vertex attributes will use the same index from this index array. In
    //   our code, indices are unsigned ints.
    //
    struct PrimitiveData
    {
        PrimitiveData()
            : m_dataBuffer(NULL)
            , m_indexBuffer(NULL)
            , m_primitiveType(0)
            , m_vertexNo(0)
            , m_primitiveNo(0)
            , m_size(0)
        {
        }

        PrimitiveData(GLvoid* databuffer, GLvoid* indexbuffer,
                      GLenum primitivetype, size_t vertexno, size_t primitiveno,
                      size_t size)
            : m_dataBuffer(databuffer)
            , m_indexBuffer(indexbuffer)
            , m_primitiveType(primitivetype)
            , m_vertexNo(vertexno)
            , m_primitiveNo(primitiveno)
            , m_size(size)
        {
        }

        // copy constructor
        PrimitiveData(const PrimitiveData& v)
            : m_dataBuffer(v.m_dataBuffer)
            , m_indexBuffer(v.m_indexBuffer)
            , m_primitiveType(v.m_primitiveType)
            , m_vertexNo(v.m_vertexNo)
            , m_primitiveNo(v.m_primitiveNo)
            , m_size(v.m_size)
        {
        }

        GLvoid* m_dataBuffer;   // data pointer
        GLvoid* m_indexBuffer;  // indices for indexed rendering
        GLenum m_primitiveType; // GL_TRIANGLES, etc.
        size_t m_vertexNo;      // number of vertices
        size_t m_primitiveNo;   // number of primitives
        size_t m_size;          // buffer size in bytes
    };

    //
    // render a list of tris, quads, points, faces, etc.
    // NOTE right now renderPrimitives() will render all primitives, i.e., it is
    // not an option to render partial data in the future, the code can be
    // modified to support rendering of a partial list by default texture,
    // vertex, color, normal all in one VBO. indices in its own VBO in the
    // future, we can make it so that texture, vertex, color, normal, indices
    // each has its own VBO. However, that would also mean that we need to do a
    // glBufferData on each of these vbos, which can be costly.
    //
    class RenderPrimitives
    {
    public:
        typedef std::vector<VertexAttribute> VertexAttributeList;
        //
        // for gl to render primitives, it needs a seires of information
        // including the current program id, primitive types, data types, vertex
        // attributes, etc.
        //
        RenderPrimitives(const GLProgram*, PrimitiveData&, VertexAttributeList&,
                         GLVBO::GLVBOVector&);

        ~RenderPrimitives();

        //
        // fill in GL buffers with data and do proper GL setup
        //
        // the databuffer goes into the first vbo and the
        // PrimitiveIndexBuffer(if exist) is always into the second VBO
        void pushDataToBuffer();

        //
        // bind and unbind buffer to GL
        //
        void bindBuffer() const;
        void unbindBuffer() const;

        //
        // by default all primitives will be rendered
        // develper todo NOTE this is assuming the shader is setup in this way:
        // glBindAttribLocation(program, 0, "v_vertex"); 0 for vertex, 1 for
        // texture, 3 for color, 4 for normal
        //
        void render() const;

        // clear all states
        void clear();

        //
        // these take care of init setup render and cleanup altogether
        //
        void setupAndRender();

    private:
        // used by the constructor only
        void init(GLVBO::GLVBOVector&);

    private:
        const GLProgram* m_program;
        PrimitiveData m_primitiveData;
        VertexAttributeList m_attributeList;
        bool m_bufferIsSet;
        bool m_hasIndices;          // if there are indices to be used
        GLVBO* m_dataVBO;           // data vbo for this render
        GLVBO* m_indexVBO;          // vbo for indices(if exist)
        GLenum m_indexDataType;     // GL_UNSIGNED_INT etc.
        int m_verticesPerPrimitive; // 3 for triangles
    };

} // namespace TwkGLF
#endif // __TwkGLF__GLRENDERPRIMITIVES__h__
