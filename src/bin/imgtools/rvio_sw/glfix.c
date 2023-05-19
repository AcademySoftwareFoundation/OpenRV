#include <stdlib.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glext.h>


void glBlitFramebufferEXT (GLint srcX0,
                           GLint srcY0,
                           GLint srcX1,
                           GLint srcY1,
                           GLint dstX0,
                           GLint dstY0,
                           GLint dstX1,
                           GLint dstY1,
                           GLbitfield mask,
                           GLenum filter)
{
    glBlitFramebuffer (srcX0,
                       srcY0,
                       srcX1,
                       srcY1,
                       dstX0,
                       dstY0,
                       dstX1,
                       dstY1,
                       mask,
                       filter);
}

#ifdef PLATFORM_LINUX

//
//  On linux, GL_NV_fence gets defined (not sure where), but the
//  corresponding symbols are not provided by MesaGL.  We don't
//  actually need them, so provide the symbols here. -- Alan
//  

void glSetFenceNV       (GLuint u, GLenum e)          {}
void glFinishFenceNV    (GLuint u)                    {}
void glGenFencesNV      (GLsizei s, GLuint *ip)       {}
void glDeleteFencesNV   (GLsizei s, const GLuint *ip) {}
GLboolean glTestFenceNV (GLuint u)                    {}

#endif

#ifdef PLATFORM_DARWIN

//
//  Same as above but for AppleFence
//  

void glSetFenceAPPLE       (unsigned int u, unsigned int e)          {}
void glFinishFenceAPPLE    (unsigned int u)                    {}
void glGenFencesAPPLE      (size_t s, unsigned int *ip)       {}
void glDeleteFencesAPPLE   (size_t s, const unsigned int *ip) {}

#endif
