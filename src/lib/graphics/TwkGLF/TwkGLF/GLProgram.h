//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#pragma once
#ifndef __TwkGLF__GLPROGRAM__h__
#define __TwkGLF__GLPROGRAM__h__

#include <TwkGLF/GL.h>
#include <map>

namespace TwkGLF
{

    //
    // GLProgram maintains the program id, and a list of uniforms, and
    // their locations.  ShaderProgram.cpp Program and BasicGLProgram are
    // both derived from this.
    //

    class GLProgram
    {
    public:
        //
        //  Types
        //

        typedef std::map<std::string, unsigned int> UniformLocationMap;
        typedef std::map<std::string, unsigned int> AttribLocationMap;

        GLProgram()
            : m_programId(0)
        {
        }

        ~GLProgram() {}

        //
        //  Base API
        //

        virtual bool compile() = 0;

        void collectUniforms();
        void collectAttribs();
        unsigned int uniformLocation(const std::string& name) const;
        unsigned int attribLocation(const std::string& name) const;

        const UniformLocationMap& uniformLocationMap() const
        {
            return m_uniformLocationMap;
        }

        const AttribLocationMap& attribLocationMap() const
        {
            return m_attribLocationMap;
        }

        const size_t programId() const { return m_programId; }

        bool isCompiled() const { return m_programId > 0; }

        void use() const;

    protected:
        AttribLocationMap m_attribLocationMap;
        UniformLocationMap m_uniformLocationMap;
        size_t m_programId;
    };

} // namespace TwkGLF

#endif // __TwkGLF__GLPROGRAM__h__
