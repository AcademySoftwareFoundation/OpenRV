//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkGLF/BasicGLProgram.h>

extern const char* DefaultVertex_glsl;
extern const char* DefaultFrag_glsl;
extern const char* TextureVertex_glsl;
extern const char* TextureFrag_glsl;
extern const char* TexRectFrag_glsl;
extern const char* PaintColoredFrag_glsl;
extern const char* SoftPaintFrag_glsl;
extern const char* OldReplaceFrag_glsl;
extern const char* SoftOldReplaceFrag_glsl;
extern const char* ReplaceColoredVertex_glsl;
extern const char* ReplaceVertex_glsl;
extern const char* ReplaceFrag_glsl;
extern const char* SoftReplaceFrag_glsl;
extern const char* EraseVertex_glsl;
extern const char* EraseFrag_glsl;
extern const char* SoftEraseFrag_glsl;
extern const char* CloneVertex_glsl;
extern const char* CloneFrag_glsl;
extern const char* SoftCloneFrag_glsl;
extern const char* ScaleVertex_glsl;
extern const char* ScaleFrag_glsl;
extern const char* SoftScaleFrag_glsl;
extern const char* DirectionPaintVertex_glsl;
extern const char* DirectionPaintFrag_glsl;
extern const char* SoftDirectionPaintFrag_glsl;
extern const char* StereoScanlineFrag_glsl;
extern const char* StereoCheckerFrag_glsl;
extern const char* CrosshatchBGFrag_glsl;
extern const char* CheckerboardBGFrag_glsl;

namespace TwkGLF
{

    using namespace std;

    BasicGLProgram::ProgramCacheMap BasicGLProgram::m_programCache;

    //------------------------constructor and
    // destructor-------------------------
    BasicGLProgram::BasicGLProgram(const std::string& vertexcode,
                                   const std::string& fragmentcode)
        : m_vertexCode(vertexcode)
        , m_fragmentCode(fragmentcode)
    {
        m_identifier = vertexcode + fragmentcode;
    }

    BasicGLProgram::~BasicGLProgram() { uninstall(); }

    //------------------------public
    // functions-----------------------------------
    const char* BasicGLProgram::identifier() const
    {
        return m_identifier.c_str();
    }

    const BasicGLProgram*
    BasicGLProgram::select(const std::string& vertexcode,
                           const std::string& fragmentcode)
    {
        std::string identifier = vertexcode + fragmentcode;
        ProgramCacheMap::const_iterator i =
            m_programCache.find(identifier.c_str());

        if (i != m_programCache.end())
        {
            return i->second;
        }
        else
        {
            BasicGLProgram* p = new BasicGLProgram(vertexcode, fragmentcode);
            if (p->compile())
            {
                m_programCache[p->identifier()] = p;
            }
            else
            {
                delete p;
                p = 0;
            }
            return p;
        }
    }

    bool BasicGLProgram::compile()
    {
        GLuint v, f;
        m_programId = glCreateProgram();
        v = glCreateShader(GL_VERTEX_SHADER);
        f = glCreateShader(GL_FRAGMENT_SHADER);
        const char* vcode = m_vertexCode.c_str();
        glShaderSource(v, 1, &vcode, NULL);
        glCompileShader(v);
        const char* fcode = m_fragmentCode.c_str();
        glShaderSource(f, 1, &fcode, NULL);
        glCompileShader(f);

        GLint status = GL_TRUE;

        glGetShaderiv(v, GL_COMPILE_STATUS, &status);

        if (status != GL_TRUE)
        {
            GLint infologLength = 0;
            glGetShaderiv(v, GL_INFO_LOG_LENGTH, &infologLength);
            if (infologLength > 1)
            {
                int charsWritten = 0;
                char* infoLog = (char*)malloc(infologLength + 1);
                glGetShaderInfoLog(v, infologLength, &charsWritten, infoLog);
                cout << infoLog << endl;
                free(infoLog);
            }
        }

        status = GL_TRUE;
        glGetShaderiv(f, GL_COMPILE_STATUS, &status);

        if (status != GL_TRUE)
        {
            GLint infologLength = 0;
            glGetShaderiv(f, GL_INFO_LOG_LENGTH, &infologLength);
            if (infologLength > 1)
            {
                int charsWritten = 0;
                char* infoLog = (char*)malloc(infologLength + 1);
                glGetShaderInfoLog(f, infologLength, &charsWritten, infoLog);
                cout << infoLog << endl;
                free(infoLog);
            }
        }

        TWK_GLDEBUG;

        glAttachShader(m_programId, v);
        TWK_GLDEBUG;

        glAttachShader(m_programId, f);
        TWK_GLDEBUG;

        glLinkProgram(m_programId);

        status = GL_TRUE;
        glGetProgramiv(m_programId, GL_LINK_STATUS, &status);

        if (status != GL_TRUE)
        {
            GLint logsize = 0;
            glGetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &logsize);
            if (logsize > 1)
            {
                GLsizei rlen;
                vector<char> buffer(logsize);
                glGetProgramInfoLog(m_programId, logsize, &rlen,
                                    &buffer.front());
                cout << buffer.front() << endl;
            }
        }

        TWK_GLDEBUG;

        m_fragmentShader = f;
        m_vertexShader = v;

        collectUniforms();
        collectAttribs();

        return true;
    }

    void BasicGLProgram::uninstall()
    {
        if (m_vertexShader)
            glDeleteShader(m_vertexShader);
        if (m_fragmentShader)
            glDeleteShader(m_fragmentShader);
        if (m_programId)
            glDeleteProgram(m_programId);
    }

    GLuint BasicGLProgram::use() const
    {
        glUseProgram(m_programId);
        return m_programId;
    }

    namespace
    {

        const GLProgram* basicGLProgram(const char* vertexShader,
                                        const char* fragShader)
        {
            if (const BasicGLProgram* glprogram =
                    BasicGLProgram::select(vertexShader, fragShader))
            {
                return glprogram;
            }
            else
            {
                return NULL;
            }
        }
    } // namespace

    const GLProgram* defaultGLProgram()
    {
        return basicGLProgram(DefaultVertex_glsl, DefaultFrag_glsl);
    }

    // this is called TextureGL program because it is a simple program just like
    // the default one, except that it uses one texture
    const GLProgram* textureGLProgram()
    {
        return basicGLProgram(TextureVertex_glsl, TextureFrag_glsl);
    }

    // this is called TextureGL program because it is a simple program just like
    // the default one, except that it uses one texture2DRect
    const GLProgram* textureRectGLProgram()
    {
        return basicGLProgram(TextureVertex_glsl, TexRectFrag_glsl);
    }

    const GLProgram* checkerBGGLProgram()
    {
        return basicGLProgram(TextureVertex_glsl, CheckerboardBGFrag_glsl);
    }

    const GLProgram* crosshatchBGGLProgram()
    {
        return basicGLProgram(TextureVertex_glsl, CrosshatchBGFrag_glsl);
    }

    const GLProgram* stereoScanlineGLProgram()
    {
        return basicGLProgram(TextureVertex_glsl, StereoScanlineFrag_glsl);
    }

    const GLProgram* stereoCheckerGLProgram()
    {
        return basicGLProgram(TextureVertex_glsl, StereoCheckerFrag_glsl);
    }

    const GLProgram* paintOldReplaceGLProgram()
    {
        return basicGLProgram(ReplaceVertex_glsl, OldReplaceFrag_glsl);
    }

    const GLProgram* softPaintOldReplaceGLProgram()
    {
        return basicGLProgram(ReplaceVertex_glsl, SoftOldReplaceFrag_glsl);
    }

    const GLProgram* paintReplaceGLProgram()
    {
        return basicGLProgram(ReplaceVertex_glsl, ReplaceFrag_glsl);
    }

    const GLProgram* softPaintReplaceGLProgram()
    {
        return basicGLProgram(ReplaceVertex_glsl, SoftReplaceFrag_glsl);
    }

    const GLProgram* paintEraseGLProgram()
    {
        return basicGLProgram(EraseVertex_glsl, EraseFrag_glsl);
    }

    const GLProgram* softPaintEraseGLProgram()
    {
        return basicGLProgram(EraseVertex_glsl, SoftEraseFrag_glsl);
    }

    const GLProgram* paintCloneGLProgram()
    {
        return basicGLProgram(CloneVertex_glsl, CloneFrag_glsl);
    }

    const GLProgram* softPaintCloneGLProgram()
    {
        return basicGLProgram(CloneVertex_glsl, SoftCloneFrag_glsl);
    }

    const GLProgram* paintScaleGLProgram()
    {
        return basicGLProgram(ScaleVertex_glsl, ScaleFrag_glsl);
    }

    const GLProgram* softPaintScaleGLProgram()
    {
        return basicGLProgram(ScaleVertex_glsl, SoftScaleFrag_glsl);
    }

    const GLProgram* directionPaintGLProgram()
    {
        return basicGLProgram(DirectionPaintVertex_glsl,
                              DirectionPaintFrag_glsl);
    }

    const GLProgram* softDirectionPaintGLProgram()
    {
        return basicGLProgram(DirectionPaintVertex_glsl,
                              SoftDirectionPaintFrag_glsl);
    }

    const GLProgram* paintTessellateGLProgram()
    {
        return basicGLProgram(ReplaceColoredVertex_glsl, PaintColoredFrag_glsl);
    }

} // namespace TwkGLF
