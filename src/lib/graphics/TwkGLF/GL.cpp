//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkGLF/GL.h>
using namespace TwkMath;
using namespace std;

namespace
{
    struct token_string
    {
        GLuint Token;
        const char* String;
    };

    static const struct token_string GLErrors[] = {
        {GL_NO_ERROR, "no error"},
        {GL_INVALID_ENUM, "invalid enumerant"},
        {GL_INVALID_VALUE, "invalid value"},
        {GL_INVALID_OPERATION, "invalid operation"},
        {GL_STACK_OVERFLOW, "stack overflow"},
        {GL_STACK_UNDERFLOW, "stack underflow"},
        {GL_OUT_OF_MEMORY, "out of memory"},
        {GL_TABLE_TOO_LARGE, "table too large"},
#ifdef GL_EXT_framebuffer_object
        {GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "invalid framebuffer operation"},
#endif
        {static_cast<GLuint>(~0), NULL} /* end of list indicator */
    };

} // namespace

namespace TwkGLF
{

    string safeGLGetString(GLenum name)
    {
        if (const GLubyte* s = glGetString(name))
            return string((const char*)s);
        else
            return string();
    }

    string errorString(GLenum errorCode)
    {
        for (size_t i = 0; GLErrors[i].String; i++)
        {
            if (GLErrors[i].Token == errorCode)
                return string(GLErrors[i].String);
        }

        ostringstream str;
        str << "unknown GL error code: " << errorCode;
        return str.str();
    }

} // namespace TwkGLF

bool glSupportsExtension(const char* extstring)
{
    char* s = (char*)glGetString(GL_EXTENSIONS);    // Get our extension-string
    char* temp = (s) ? strstr(s, extstring) : NULL; // Is our extension a
                                                    // string?
    return temp != NULL;                            // Return false.
}

void glDrawBox(const Box3f& bbox)
{
    Vec3f a(bbox.min.x, bbox.min.y, bbox.min.z);
    Vec3f b(bbox.min.x, bbox.min.y, bbox.max.z);
    Vec3f c(bbox.min.x, bbox.max.y, bbox.min.z);
    Vec3f d(bbox.min.x, bbox.max.y, bbox.max.z);
    Vec3f e(bbox.max.x, bbox.min.y, bbox.min.z);
    Vec3f f(bbox.max.x, bbox.min.y, bbox.max.z);
    Vec3f g(bbox.max.x, bbox.max.y, bbox.min.z);
    Vec3f h(bbox.max.x, bbox.max.y, bbox.max.z);

    glBegin(GL_QUADS);

    glNormal3f(0, 0, -1);
    glVertex(a);
    glVertex(e);
    glVertex(g);
    glVertex(c);
    glNormal3f(-1, 0, 0);
    glVertex(a);
    glVertex(c);
    glVertex(d);
    glVertex(b);
    glNormal3f(0, -1, 0);
    glVertex(a);
    glVertex(b);
    glVertex(f);
    glVertex(e);

    glNormal3f(0, 1, 0);
    glVertex(h);
    glVertex(d);
    glVertex(c);
    glVertex(g);
    glNormal3f(1, 0, 0);
    glVertex(h);
    glVertex(g);
    glVertex(e);
    glVertex(f);
    glNormal3f(0, 0, 1);
    glVertex(h);
    glVertex(f);
    glVertex(b);
    glVertex(d);

    glEnd();
}

#define PI_ 3.14159265358979323846

//
//  From SGI's OpenGL website
//

void glJitterFrustum(GLdouble left, GLdouble right, GLdouble bottom,
                     GLdouble top, GLdouble nearPlane, GLdouble farPlane,
                     GLdouble pixdx, GLdouble pixdy, GLdouble eyedx,
                     GLdouble eyedy, GLdouble focus)
{
    GLdouble xwsize, ywsize;
    GLdouble dx, dy;
    GLint viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);

    xwsize = right - left;
    ywsize = top - bottom;
    dx = -(pixdx * xwsize / (GLdouble)viewport[2] + eyedx * nearPlane / focus);
    dy = -(pixdy * ywsize / (GLdouble)viewport[3] + eyedy * nearPlane / focus);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(left + dx, right + dx, bottom + dy, top + dy, nearPlane,
              farPlane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-eyedx, -eyedy, 0.0);
}

void glJitterPerspective(GLdouble fovy, GLdouble aspect, GLdouble nearPlane,
                         GLdouble farPlane, GLdouble pixdx, GLdouble pixdy,
                         GLdouble eyedx, GLdouble eyedy, GLdouble focus)
{
    GLdouble fov2, left, right, bottom, top;
    fov2 = ((fovy * PI_) / 180.0) / 2.0;

    top = nearPlane / (cos(fov2) / sin(fov2));
    bottom = -top;
    right = top * aspect;
    left = -right;

    glJitterFrustum(left, right, bottom, top, nearPlane, farPlane, pixdx, pixdy,
                    eyedx, eyedy, focus);
}
