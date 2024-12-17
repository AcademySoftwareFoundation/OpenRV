//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#pragma once
#ifndef __TwkGLF__GLDEFAULTPROGRAM__h__
#define __TwkGLF__GLDEFAULTPROGRAM__h__

#include <TwkGLF/GL.h>
#include <TwkGLF/GLProgram.h>
#include <vector>
#include <map>

namespace TwkGLF
{

    struct CompareStr
    {
        bool operator()(const char* A, const char* B) const
        {
            return strcmp(A, B) < 0;
        }
    };

    //
    // simple GLPrograms. these differ from the ShaderProgram::Program in that
    // these don't involve any expressions or rewriting of code.
    //
    class BasicGLProgram : public GLProgram
    {
    public:
        //------------------------typedefs and
        // structs------------------------------- the string is the vertex and
        // fragment shader string combined
        typedef std::map<const char*, BasicGLProgram*, CompareStr>
            ProgramCacheMap;

        //------------------------constructor and
        // destructor-------------------------
        BasicGLProgram(const std::string& vertexcode,
                       const std::string& fragmentcode);
        ~BasicGLProgram();

        //------------------------public
        // functions-----------------------------------
        const char* identifier() const;

        bool compile();

        void uninstall();

        const bool isBound() const;

        GLuint use() const;

        static const BasicGLProgram* select(const std::string& vertexcode,
                                            const std::string& fragmentcode);

    private:
        GLuint m_vertexShader;
        GLuint m_fragmentShader;
        std::string m_identifier;
        std::string m_fragmentCode;
        std::string m_vertexCode;
        static ProgramCacheMap m_programCache;
    };

    const GLProgram* defaultGLProgram();
    const GLProgram* textureGLProgram();
    const GLProgram* textureRectGLProgram();

    const GLProgram* checkerBGGLProgram();
    const GLProgram* crosshatchBGGLProgram();

    const GLProgram* stereoCheckerGLProgram();
    const GLProgram* stereoScanlineGLProgram();

    const GLProgram* paintOldReplaceGLProgram();
    const GLProgram* softPaintOldReplaceGLProgram();

    const GLProgram* paintReplaceGLProgram();
    const GLProgram* softPaintReplaceGLProgram();
    const GLProgram* paintScaleGLProgram();
    const GLProgram* softPaintScaleGLProgram();
    const GLProgram* paintEraseGLProgram();
    const GLProgram* softPaintEraseGLProgram();
    const GLProgram* paintCloneGLProgram();
    const GLProgram* softPaintCloneGLProgram();

    const GLProgram* paintTessellateGLProgram();

    const GLProgram* directionPaintGLProgram();
    const GLProgram* softDirectionPaintGLProgram();

} // namespace TwkGLF

#endif // __TwkGLF__GLDEFAULTPROGRAM__h__
