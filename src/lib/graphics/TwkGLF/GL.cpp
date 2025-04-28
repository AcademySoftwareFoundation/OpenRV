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

#include <QOpenGLContext>

namespace
{

    // #define TRACK_GL_CONTEXT

#ifdef TRACK_GL_CONTEXT

#define DECLARE_VAR(typ, var, def) \
    typ m_prev##var = def;         \
    typ m_curr##var = def;

#define PRINT_VAR(var)                                                       \
    std::cerr << "[" << #var << ": " << m_prev##var << " to " << m_curr##var \
              << "] ";

#define UPDATEVAR_VAL(var, val)     \
    m_prev##var = m_curr##var;      \
    m_curr##var = val;              \
    if (m_prev##var != m_curr##var) \
    {                               \
        changed = true;             \
    }

#define UPDATEVAR_GLGET(var, ident)     \
    m_prev##var = m_curr##var;          \
    glGetIntegerv(ident, &m_curr##var); \
    if (m_prev##var != m_curr##var)     \
    {                                   \
        changed = true;                 \
    }

    class GLContextStateTracker
    {
    public:
        GLContextStateTracker()
        {
            m_ctx = QOpenGLContext::currentContext();
            m_name = "????";

            std::cerr << "-------> [NEW CONTEXT: " << name() << "]"
                      << std::endl;
        }

        void update(std::string_view file, std::string_view function, int line)
        {
            bool changed = false;
            QOpenGLContext* context = QOpenGLContext::currentContext();

            UPDATEVAR_VAL(QT_FBO, context->defaultFramebufferObject());
            UPDATEVAR_GLGET(GL_FBO, GL_FRAMEBUFFER_BINDING);
            UPDATEVAR_GLGET(GL_RBO, GL_RENDERBUFFER_BINDING);

            if (changed)
            {
                std::cerr << "   [QT_CTX: " << name() << "] ";

                PRINT_VAR(QT_FBO);
                PRINT_VAR(GL_FBO);
                PRINT_VAR(GL_RBO);

                bool printDetails = true;
                if (printDetails)
                    std::cerr << " " << shorterPath(file).data() << " "
                              << function.data() << "@" << line;

                std::cerr << std::endl;
            }
        }

        void setName(std::string_view name) { m_name = name; }

        std::string name() const
        {
            std::ostringstream ss;
            ss << (void*)(m_ctx) << "/" << m_name.c_str();

            std::string s = ss.str();
            return s;
        }

    protected:
        DECLARE_VAR(int32_t, QT_FBO, -1);
        DECLARE_VAR(int32_t, GL_FBO, -1);
        DECLARE_VAR(int32_t, GL_RBO, -1);

        std::string m_name = "";
        void* m_ctx = nullptr;
    };

    class GLMultiContextStateTracker
    {
    public:
        GLMultiContextStateTracker() {}

        void setCurrentContextName(std::string_view name)
        {
            GLContextStateTracker& tracker =
                m_trackers[QOpenGLContext::currentContext()];
            tracker.setName(name);
        }

        void update(std::string_view file, std::string_view function, int line)
        {
            m_prevContext = m_currContext;
            m_currContext = QOpenGLContext::currentContext();

            if (m_prevContext == nullptr)
                m_prevContext = m_currContext;

            if (m_currContext != m_prevContext)
            {
                const GLContextStateTracker& t0 = m_trackers[m_prevContext];
                const GLContextStateTracker& t1 = m_trackers[m_currContext];

                std::cerr << "@@ [QT_CTX: " << t0.name() << " to " << t1.name()
                          << "]" << std::endl;
            }

            if (m_currContext)
            {
                m_trackers[m_currContext].update(file, function, line);
            }
        }

    protected:
        std::map<QOpenGLContext*, GLContextStateTracker> m_trackers;
        QOpenGLContext* m_prevContext = nullptr;
        QOpenGLContext* m_currContext = nullptr;
    };

    static GLMultiContextStateTracker s_multiTracker;
#endif // TRACK_GL_CONTEXT

    static std::string_view shorterPath(std::string_view path)
    {
        const bool veryshort = true;

        if (veryshort)
        {
            size_t pos = path.rfind('/');
            if (pos != std::string_view::npos)
            {
                std::string_view result = path.substr(pos + 1);
                return result;
            }
        }
        else
        {
            size_t pos = path.find("/src/");
            if (pos != std::string_view::npos)
                return path.substr(pos);
        }

        return path;
    }

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

bool twkGlPrintError(std::string_view file, std::string_view function,
                     const int line, const std::string_view msg)
{
    if (GLuint err = glGetError())
    {
        std::cerr << "GL_ERROR: " << shorterPath(file).data()
                  << "::" << function.data() << ":" << line << " ["
                  << TwkGLF::errorString(err) << "]" << std::endl;
        return false;
    }

    // Track OpenGL context updates.
    // Enable the line below to track what's going on with current contexts and
    // current FBOs.
#ifdef TRACK_GL_CONTEXT
    s_multiTracker.update(file, function, line);
#endif

    return true;
}

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
