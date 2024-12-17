//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkGLF__GL__h__
#define __TwkGLF__GL__h__
#include <TwkMath/Vec4.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Box.h>
#include <TwkMath/Color.h>
#include <TwkMath/Mat44.h>

//
//  GLEW is taking care of GL extensions (esp on Windows) and includes
//  the proper GL headers for us.
//
#ifndef TWK_USE_MESA
#ifdef TWK_USE_GLEW
#include <GL/glew.h>
#endif

//
//  Darwin has a different location for the native headers -- if you
//  use the GL/ headers you might pick up mesa.
//

// ajg
// mR - 10/28/07
#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif
#endif

#if defined(PLATFORM_DARWIN)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif defined(PLATFORM_LINUX)
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#ifdef TWK_USE_GLEW
#define TWK_GL_SUPPORTS(X) glewIsSupported(X)
#else
#define TWK_GL_SUPPORTS(X) glSupportsExtension(X)
#endif

#ifndef GL_TEXTURE_RECTANGLE
// for back port from core profile
#define GL_TEXTURE_RECTANGLE GL_TEXTURE_RECTANGLE_ARB
#endif

#include <iostream>

//----------------------------------------------------------------------
//
//  These are RAII structs to make sure GL state is properly handled
//  esp in the presence of thrown exceptions
//

struct GLPushAttrib
{
    GLPushAttrib(GLenum e) { glPushAttrib(e); }

    ~GLPushAttrib() { glPopAttrib(); }
};

struct GLPushClientAttrib
{
    GLPushClientAttrib(GLenum e) { glPushClientAttrib(e); }

    ~GLPushClientAttrib() { glPopClientAttrib(); }
};

struct GLPushMatrix
{
    GLPushMatrix() { glPushMatrix(); }

    ~GLPushMatrix() { glPopMatrix(); }
};

//----------------------------------------------------------------------
//
//  DEBUG macro
//

#ifdef NDEBUG
#define TWK_GLDEBUG ;
#else
// #define TWK_ABORT abort()
#define TWK_ABORT
#define TWK_GLDEBUG                                                \
    if (GLuint err = glGetError())                                 \
    {                                                              \
        std::cerr << "GL_ERROR: in " << __FILE__ << ", function "  \
                  << __FUNCTION__ << ", line " << __LINE__ << ": " \
                  << TwkGLF::errorString(err) << std::endl;        \
        TWK_ABORT;                                                 \
    }
#endif

//----------------------------------------------------------------------
//
//  Define GL_HALF_FLOAT_ARB if its not already defined
//

#ifndef GL_HALF_FLOAT_ARB
#ifdef GL_HALF_APPLE
#define GL_HALF_FLOAT_ARB GL_HALF_APPLE
#else
#ifdef GL_HALF_FLOAT_NV
#define GL_HALF_FLOAT_ARB GL_HALF_FLOAT_NV
#endif
#endif
#endif

//----------------------------------------------------------------------

namespace TwkGLF
{
    std::string errorString(GLenum errorCode);
    std::string safeGLGetString(GLenum name);
} // namespace TwkGLF

//
//  These functions have C++ linkage -- so its not necessary to put
//  them in a seperate namespace.
//

//
// COLOR
//
inline void glColor(const TwkMath::Vec3f& c) { glColor3fv((GLfloat*)(&c)); }

//
inline void glColor(const TwkMath::Vec4f& c) { glColor4fv((GLfloat*)(&c)); }

#if 0
//
// SECONDARY COLOR
//
#if GL_EXT_secondary_color
//
inline void glSecondaryColor( const TwkMath::Vec3f &c )
{ 
    glSecondaryColor3fvEXT( ( GLfloat * )( &c ) ); 
}

//
inline void glSecondardColor( const TwkMath::Vec4f &c )
{
    glSecondaryColor3fvEXT( ( GLfloat * )( &c ) );
}
#endif
#endif

//
// NORMAL
//
inline void glNormal(const TwkMath::Vec3f& n) { glNormal3fv((GLfloat*)(&n)); }

//
// TEX COORD
//
inline void glTexCoord(const GLfloat f) { glTexCoord1f(f); }

//
inline void glTexCoord(const TwkMath::Vec2f& t)
{
    glTexCoord2fv((GLfloat*)(&t));
}

inline void glTexCoord(const TwkMath::Vec3f& t)
{
    glTexCoord3fv((GLfloat*)(&t));
}

inline void glTexCoord(const TwkMath::Vec4f& t)
{
    glTexCoord4fv((GLfloat*)(&t));
}

//
// MULTI TEX COORD
//
#if GL_ARB_multitexture
//
inline void glMultiTexCoord(GLenum unit, GLfloat f)
{
    glMultiTexCoord1fARB(unit, f);
}

inline void glMultiTexCoord(GLenum unit, const TwkMath::Vec2f& t)
{
    glMultiTexCoord2fvARB(unit, (GLfloat*)(&t));
}

inline void glMultiTexCoord(GLenum unit, const TwkMath::Vec3f& t)
{
    glMultiTexCoord3fvARB(unit, (GLfloat*)(&t));
}

inline void glMultiTexCoord(GLenum unit, const TwkMath::Vec4f& t)
{
    glMultiTexCoord4fvARB(unit, (GLfloat*)(&t));
}
#endif

//
// VERTEX
//
inline void glVertex(const TwkMath::Vec2f& v) { glVertex2fv((GLfloat*)(&v)); }

inline void glVertex(const TwkMath::Vec3f& v) { glVertex3fv((GLfloat*)(&v)); }

inline void glVertex(const TwkMath::Vec4f& v) { glVertex4fv((GLfloat*)(&v)); }

//
// MATERIAL
//
inline void glMaterial(GLenum face, GLenum pname, GLint i)
{
    glMateriali(face, pname, i);
}

inline void glMaterial(GLenum face, GLenum pname, GLfloat f)
{
    glMaterialf(face, pname, f);
}

inline void glMaterial(GLenum face, GLenum pname, const TwkMath::Vec4f& v)
{
    glMaterialfv(face, pname, (GLfloat*)(&v));
}

//
// LIGHT
//
inline void glLight(GLenum light, GLenum pname, GLint i)
{
    glLighti(light, pname, i);
}

inline void glLight(GLenum light, GLenum pname, GLfloat f)
{
    glLightf(light, pname, f);
}

inline void glLight(GLenum light, GLenum pname, const TwkMath::Vec4f& v)
{
    glLightfv(light, pname, (GLfloat*)(&v));
}

inline void glLight(GLenum light, GLenum pname, const TwkMath::Vec3f& v,
                    float wa = 1.0f)
{
    TwkMath::Vec4f v4(v[0], v[1], v[2], wa);
    glLightfv(light, pname, (GLfloat*)(&v4));
}

//
// MATRIX
//
inline TwkMath::Mat44f getMatrix(GLenum matrix)
{
    GLfloat m[16];
    glGetFloatv(matrix, m);

    // Unfortunately, open GL stores matrices in COLUMN-major
    // format, whereas we store them in the much more sane ROW-major
    // format.
    return TwkMath::Mat44f(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13],
                           m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);
}

inline void glLoadMatrix(const TwkMath::Mat44f& m)
{
    GLfloat mv[16];

    mv[0] = m[0][0];
    mv[1] = m[1][0];
    mv[2] = m[2][0];
    mv[3] = m[3][0];

    mv[4] = m[0][1];
    mv[5] = m[1][1];
    mv[6] = m[2][1];
    mv[7] = m[3][1];

    mv[8] = m[0][2];
    mv[9] = m[1][2];
    mv[10] = m[2][2];
    mv[11] = m[3][2];

    mv[12] = m[0][3];
    mv[13] = m[1][3];
    mv[14] = m[2][3];
    mv[15] = m[3][3];

    glLoadMatrixf(mv);
}

inline void glMultMatrix(const TwkMath::Mat44f& m)
{
    GLfloat mv[16];

    mv[0] = m[0][0];
    mv[1] = m[1][0];
    mv[2] = m[2][0];
    mv[3] = m[3][0];

    mv[4] = m[0][1];
    mv[5] = m[1][1];
    mv[6] = m[2][1];
    mv[7] = m[3][1];

    mv[8] = m[0][2];
    mv[9] = m[1][2];
    mv[10] = m[2][2];
    mv[11] = m[3][2];

    mv[12] = m[0][3];
    mv[13] = m[1][3];
    mv[14] = m[2][3];
    mv[15] = m[3][3];

    glMultMatrixf(mv);
}

//
// TRANSFORM
//
#if 0
inline void glRotate( const TwkMath::Qtnf &r )
{
    float angle;
    TwkMath::Vec3f axis;
    r.value( axis, angle );
    glRotatef( TwkMath::radToDeg( angle ), axis[0], axis[1], axis[2] );
}
#endif

inline void glTranslate(const TwkMath::Vec3f& t)
{
    glTranslatef(t[0], t[1], t[2]);
}

inline void glScale(const TwkMath::Vec3f& s) { glScalef(s[0], s[1], s[2]); }

inline void setUniformGLMatrix(GLuint p, const unsigned int loc, GLfloat* mat,
                               bool transpose = true)
{
    glUniformMatrix4fv(loc, 1, transpose, mat);
    TWK_GLDEBUG;
}

inline void getUniformGLMatrix(GLuint p, const unsigned int loc, GLfloat* mat)
{
    glGetUniformfv(p, loc, mat);
    TWK_GLDEBUG;
}

inline bool setUniformGLFloat(GLuint p, const unsigned int loc, size_t no,
                              GLfloat* data)
{
    switch (no)
    {
    case 1:
        glUniform1fv(loc, 1, data);
        break;
    case 2:
        glUniform2fv(loc, 1, data);
        break;
    case 3:
        glUniform3fv(loc, 1, data);
        break;
    case 4:
        glUniform4fv(loc, 1, data);
        break;
    default:
        return false;
        break;
    }
    TWK_GLDEBUG;
    return true;
}

inline void getUniformGLFloat(GLuint p, const unsigned int loc, GLfloat* data)
{
    glGetUniformfv(p, loc, data);
    TWK_GLDEBUG;
}

inline bool setUniformGLInt(GLuint p, const unsigned int loc, size_t no,
                            GLint* data)
{
    switch (no)
    {
    case 1:
        glUniform1iv(loc, 1, data);
        break;
    case 2:
        glUniform2iv(loc, 1, data);
        break;
    case 3:
        glUniform3iv(loc, 1, data);
        break;
    case 4:
        glUniform4iv(loc, 1, data);
        break;
    default:
        return false;
        break;
    }
    TWK_GLDEBUG;
    return true;
}

inline void getUniformGLInt(GLuint p, const unsigned int loc, GLint* data)
{
    glGetUniformiv(p, loc, data);
    TWK_GLDEBUG;
}

void glDrawBox(const TwkMath::Box3f& bbox);

void glJitterFrustum(GLdouble left, GLdouble right, GLdouble bottom,
                     GLdouble top, GLdouble near, GLdouble far, GLdouble pixdx,
                     GLdouble pixdy, GLdouble eyedx, GLdouble eyedy,
                     GLdouble focus);

void glJitterPerspective(GLdouble fovy, GLdouble aspect, GLdouble near,
                         GLdouble far, GLdouble pixdx, GLdouble pixdy,
                         GLdouble eyedx, GLdouble eyedy, GLdouble focus);

bool glSupportsExtension(const char*);

#if !defined(TWK_USE_GLEW) && defined(WIN32)
#define TWK_INIT_GL_EXTENSIONS                                                \
    glMultiTexCoord1fARB = (PFNGLMULTITEXCOORD1FARBPROC)wglGetProcAddress(    \
        "glMultiTexCoord1fARB");                                              \
    glMultiTexCoord2fvARB = (PFNGLMULTITEXCOORD2FVARBPROC)wglGetProcAddress(  \
        "glMultiTexCoord2fvARB");                                             \
    glMultiTexCoord3fvARB = (PFNGLMULTITEXCOORD3FVARBPROC)wglGetProcAddress(  \
        "glMultiTexCoord3fvARB");                                             \
    glMultiTexCoord4fvARB = (PFNGLMULTITEXCOORD4FVARBPROC)wglGetProcAddress(  \
        "glMultiTexCoord4fvARB");                                             \
    glVertexAttrib2fvNV = (PFNGLMULTITEXCOORD2FVARBPROC)wglGetProcAddress(    \
        "glVertexAttrib2fvNV");                                               \
    glVertexAttrib3fvNV = (PFNGLMULTITEXCOORD3FVARBPROC)wglGetProcAddress(    \
        "glVertexAttrib3fvNV");                                               \
    glVertexAttrib4fvNV = (PFNGLMULTITEXCOORD4FVARBPROC)wglGetProcAddress(    \
        "glVertexAttrib4fvNV");                                               \
    glGetProgramivARB =                                                       \
        (PFNGLGETPROGRAMIVARBPROC)wglGetProcAddress("glGetProgramivARB");     \
    glActiveTextureARB =                                                      \
        (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");   \
    glMultiTexCoord2f =                                                       \
        (PFNGLMULTITEXCOORD2FPROC)wglGetProcAddress("glMultiTexCoord2f");     \
    glDeleteBuffers =                                                         \
        (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");         \
    glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");    \
    glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");    \
    glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");    \
    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer"); \
    glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");       \
    glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");    \
    glDeleteProgramsARB =                                                     \
        (PFNGLDELETEPROGRAMSARBPROC)wglGetProcAddress("glDeleteProgramsARB"); \
    glProgramStringARB =                                                      \
        (PFNGLPROGRAMSTRINGARBPROC)wglGetProcAddress("glProgramStringARB");   \
    glBindProgramARB =                                                        \
        (PFNGLBINDPROGRAMARBPROC)wglGetProcAddress("glBindProgramARB");       \
    glGenProgramsARB =                                                        \
        (PFNGLGENPROGRAMSARBPROC)wglGetProcAddress("glGenProgramsARB");       \
    glProgramLocalParameter4fARB =                                            \
        (PFNGLPROGRAMLOCALPARAMETER4FARBPROC)wglGetProcAddress(               \
            "glProgramLocalParameter4fARB");                                  \
    glColorTable = (PFNGLCOLORTABLEPROC)wglGetProcAddress("glColorTable");    \
    glBlendEquation =                                                         \
        (PFNGLBLENDEQUATIONEXTPROC)wglGetProcAddress("glBlendEquation")
#else
#define TWK_INIT_GL_EXTENSIONS
#endif

#endif // __TwkGLF__GL__h__
