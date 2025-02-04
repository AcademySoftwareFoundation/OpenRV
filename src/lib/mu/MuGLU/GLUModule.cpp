//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <limits>

#include <Mu/Symbol.h>
#include <Mu/SymbolTable.h>
#include <Mu/Function.h>
#include <Mu/Exception.h>
#include <Mu/FunctionObject.h>
#include <Mu/Module.h>
#include <Mu/Class.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/Signature.h>
#include <Mu/Thread.h>
#include <Mu/SymbolicConstant.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/FixedArray.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>

// AJG - CLASSIC
#ifdef _MSC_VER
//
//  As of 1/30/2011, defining this causes link errors
//
//  #define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gl/glew.h> // For GL_BGR, GL_BGRA
#endif

#if defined(PLATFORM_DARWIN)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#elif defined(PLATFORM_LINUX)
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#elif defined(PLATFORM_WINDOWS)
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <MuGLU/GLUModule.h>

namespace Mu
{

    using namespace std;

    void GLUModule::init()
    {
        static bool initialized = false;

        if (!initialized)
        {
            initialized = true;
        }
    }

    GLUModule::GLUModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    GLUModule::~GLUModule() {}

    void GLUModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        MuLangContext* c = (MuLangContext*)globalModule()->context();

        //
        // Instantate types used later...
        //
        c->arrayType(c->stringType(), 1, 0);   // string[]
        c->arrayType(c->boolType(), 1, 0);     // bool[]
        c->arrayType(c->intType(), 1, 0);      // int[]
        c->arrayType(c->floatType(), 1, 0);    // float[]
        c->arrayType(c->byteType(), 1, 0);     // byte[]
        c->arrayType(c->vec2fType(), 1, 0);    // vec2f[]
        c->arrayType(c->vec3fType(), 1, 0);    // vec3f[]
        c->arrayType(c->vec4fType(), 1, 0);    // vec4f[]
        c->arrayType(c->floatType(), 2, 4, 4); // float[4,4]

#define newGlConstant(name) new SymbolicConstant(c, #name, "int", Value(name))

        addSymbols(
            //
            // Note: These are ALL of the GLU 1.3 constants!
            //

            /* AJG - Many missing windows things for GLU */

            /* Extensions */
            //        newGlConstant( GLU_EXT_object_space_tess ),
            //        newGlConstant( GLU_EXT_nurbs_tessellator ),

            /* Boolean */
            newGlConstant(GLU_FALSE), newGlConstant(GLU_TRUE),

            /* Version */
            newGlConstant(GLU_VERSION_1_1), newGlConstant(GLU_VERSION_1_2),
            //        newGlConstant( GLU_VERSION_1_3 ),

            /* StringName */
            newGlConstant(GLU_VERSION), newGlConstant(GLU_EXTENSIONS),

            /* ErrorCode */
            newGlConstant(GLU_INVALID_ENUM), newGlConstant(GLU_INVALID_VALUE),
            newGlConstant(GLU_OUT_OF_MEMORY),
            //        newGlConstant( GLU_INVALID_OPERATION ),

            /* NurbsDisplay */
            /* ),
            newGlConstant( GLU_OUTLINE_POLYGON ),
            newGlConstant( GLU_OUTLINE_PATCH ),
            */

            /* NurbsCallback */
            //        newGlConstant( GLU_NURBS_ERROR ),
            newGlConstant(GLU_ERROR),
            //        newGlConstant( GLU_NURBS_BEGIN ),
            //        newGlConstant( GLU_NURBS_BEGIN_EXT ),
            //        newGlConstant( GLU_NURBS_VERTEX ),
            //        newGlConstant( GLU_NURBS_VERTEX_EXT ),
            //        newGlConstant( GLU_NURBS_NORMAL ),
            //        newGlConstant( GLU_NURBS_NORMAL_EXT ),
            //        newGlConstant( GLU_NURBS_COLOR ),
            //        newGlConstant( GLU_NURBS_COLOR_EXT ),
            //        newGlConstant( GLU_NURBS_TEXTURE_COORD ),
            //        newGlConstant( GLU_NURBS_TEX_COORD_EXT ),
            //        newGlConstant( GLU_NURBS_END ),
            //        newGlConstant( GLU_NURBS_END_EXT ),
            //        newGlConstant( GLU_NURBS_BEGIN_DATA ),
            //        newGlConstant( GLU_NURBS_BEGIN_DATA_EXT ),
            //        newGlConstant( GLU_NURBS_VERTEX_DATA ),
            //        newGlConstant( GLU_NURBS_VERTEX_DATA_EXT ),
            //        newGlConstant( GLU_NURBS_NORMAL_DATA ),
            //        newGlConstant( GLU_NURBS_NORMAL_DATA_EXT ),
            //        newGlConstant( GLU_NURBS_COLOR_DATA ),
            //        newGlConstant( GLU_NURBS_COLOR_DATA_EXT ),
            //        newGlConstant( GLU_NURBS_TEXTURE_COORD_DATA ),
            //        newGlConstant( GLU_NURBS_TEX_COORD_DATA_EXT ),
            //        newGlConstant( GLU_NURBS_END_DATA ),
            //        newGlConstant( GLU_NURBS_END_DATA_EXT ),

            /* NurbsError */
            newGlConstant(GLU_NURBS_ERROR1), newGlConstant(GLU_NURBS_ERROR2),
            newGlConstant(GLU_NURBS_ERROR3), newGlConstant(GLU_NURBS_ERROR4),
            newGlConstant(GLU_NURBS_ERROR5), newGlConstant(GLU_NURBS_ERROR6),
            newGlConstant(GLU_NURBS_ERROR7), newGlConstant(GLU_NURBS_ERROR8),
            newGlConstant(GLU_NURBS_ERROR9), newGlConstant(GLU_NURBS_ERROR10),
            newGlConstant(GLU_NURBS_ERROR11), newGlConstant(GLU_NURBS_ERROR12),
            newGlConstant(GLU_NURBS_ERROR13), newGlConstant(GLU_NURBS_ERROR14),
            newGlConstant(GLU_NURBS_ERROR15), newGlConstant(GLU_NURBS_ERROR16),
            newGlConstant(GLU_NURBS_ERROR17), newGlConstant(GLU_NURBS_ERROR18),
            newGlConstant(GLU_NURBS_ERROR19), newGlConstant(GLU_NURBS_ERROR20),
            newGlConstant(GLU_NURBS_ERROR21), newGlConstant(GLU_NURBS_ERROR22),
            newGlConstant(GLU_NURBS_ERROR23), newGlConstant(GLU_NURBS_ERROR24),
            newGlConstant(GLU_NURBS_ERROR25), newGlConstant(GLU_NURBS_ERROR26),
            newGlConstant(GLU_NURBS_ERROR27), newGlConstant(GLU_NURBS_ERROR28),
            newGlConstant(GLU_NURBS_ERROR29), newGlConstant(GLU_NURBS_ERROR30),
            newGlConstant(GLU_NURBS_ERROR31), newGlConstant(GLU_NURBS_ERROR32),
            newGlConstant(GLU_NURBS_ERROR33), newGlConstant(GLU_NURBS_ERROR34),
            newGlConstant(GLU_NURBS_ERROR35), newGlConstant(GLU_NURBS_ERROR36),
            newGlConstant(GLU_NURBS_ERROR37),

            /* NurbsProperty */
            newGlConstant(GLU_AUTO_LOAD_MATRIX), newGlConstant(GLU_CULLING),
            newGlConstant(GLU_SAMPLING_TOLERANCE),
            newGlConstant(GLU_DISPLAY_MODE),
            newGlConstant(GLU_PARAMETRIC_TOLERANCE),
            newGlConstant(GLU_SAMPLING_METHOD), newGlConstant(GLU_U_STEP),
            newGlConstant(GLU_V_STEP),
            //        newGlConstant( GLU_NURBS_MODE ),
            //        newGlConstant( GLU_NURBS_MODE_EXT ),
            //        newGlConstant( GLU_NURBS_TESSELLATOR ),
            //        newGlConstant( GLU_NURBS_TESSELLATOR_EXT ),
            //        newGlConstant( GLU_NURBS_RENDERER ),
            //        newGlConstant( GLU_NURBS_RENDERER_EXT ),

            /* NurbsSampling */
            //        newGlConstant( GLU_OBJECT_PARAMETRIC_ERROR ),
            //        newGlConstant( GLU_OBJECT_PARAMETRIC_ERROR_EXT ),
            //        newGlConstant( GLU_OBJECT_PATH_LENGTH ),
            //        newGlConstant( GLU_OBJECT_PATH_LENGTH_EXT ),
            newGlConstant(GLU_PATH_LENGTH), newGlConstant(GLU_PARAMETRIC_ERROR),
            newGlConstant(GLU_DOMAIN_DISTANCE),

            /* NurbsTrim */
            newGlConstant(GLU_MAP1_TRIM_2), newGlConstant(GLU_MAP1_TRIM_3),

            /* QuadricDrawStyle */
            newGlConstant(GLU_POINT), newGlConstant(GLU_LINE),
            newGlConstant(GLU_FILL), newGlConstant(GLU_SILHOUETTE),

            /* QuadricCallback */
            /* ), */

            /* QuadricNormal */
            newGlConstant(GLU_SMOOTH), newGlConstant(GLU_FLAT),
            newGlConstant(GLU_NONE),

            /* QuadricOrientation */
            newGlConstant(GLU_OUTSIDE), newGlConstant(GLU_INSIDE),

            /* TessCallback */
            newGlConstant(GLU_TESS_BEGIN), newGlConstant(GLU_BEGIN),
            newGlConstant(GLU_TESS_VERTEX), newGlConstant(GLU_VERTEX),
            newGlConstant(GLU_TESS_END), newGlConstant(GLU_END),
            newGlConstant(GLU_TESS_ERROR), newGlConstant(GLU_TESS_EDGE_FLAG),
            newGlConstant(GLU_EDGE_FLAG), newGlConstant(GLU_TESS_COMBINE),
            newGlConstant(GLU_TESS_BEGIN_DATA),
            newGlConstant(GLU_TESS_VERTEX_DATA),
            newGlConstant(GLU_TESS_END_DATA),
            newGlConstant(GLU_TESS_ERROR_DATA),
            newGlConstant(GLU_TESS_EDGE_FLAG_DATA),
            newGlConstant(GLU_TESS_COMBINE_DATA),

            /* TessContour */
            newGlConstant(GLU_CW), newGlConstant(GLU_CCW),
            newGlConstant(GLU_INTERIOR), newGlConstant(GLU_EXTERIOR),
            newGlConstant(GLU_UNKNOWN),

            /* TessProperty */
            newGlConstant(GLU_TESS_WINDING_RULE),
            newGlConstant(GLU_TESS_BOUNDARY_ONLY),
            newGlConstant(GLU_TESS_TOLERANCE),

            /* TessError */
            newGlConstant(GLU_TESS_ERROR1), newGlConstant(GLU_TESS_ERROR2),
            newGlConstant(GLU_TESS_ERROR3), newGlConstant(GLU_TESS_ERROR4),
            newGlConstant(GLU_TESS_ERROR5), newGlConstant(GLU_TESS_ERROR6),
            newGlConstant(GLU_TESS_ERROR7), newGlConstant(GLU_TESS_ERROR8),
            newGlConstant(GLU_TESS_MISSING_BEGIN_POLYGON),
            newGlConstant(GLU_TESS_MISSING_BEGIN_CONTOUR),
            newGlConstant(GLU_TESS_MISSING_END_POLYGON),
            newGlConstant(GLU_TESS_MISSING_END_CONTOUR),
            newGlConstant(GLU_TESS_COORD_TOO_LARGE),
            newGlConstant(GLU_TESS_NEED_COMBINE_CALLBACK),

            /* TessWinding */
            newGlConstant(GLU_TESS_WINDING_ODD),
            newGlConstant(GLU_TESS_WINDING_NONZERO),
            newGlConstant(GLU_TESS_WINDING_POSITIVE),
            newGlConstant(GLU_TESS_WINDING_NEGATIVE),
            newGlConstant(GLU_TESS_WINDING_ABS_GEQ_TWO),

            new Function(c, "gluBuild2DMipmaps", GLUModule::gluBuild2DMipmaps,
                         None, Return, "int", Args, "int", "int", "int", "int",
                         "int", "?class", End),

            new Function(c, "gluCheckExtension", GLUModule::gluCheckExtension,
                         None, Return, "bool", Args, "string", "string", End),

            new Function(c, "gluErrorString", GLUModule::gluErrorString, None,
                         Return, "string", Args, "int", End),

            new Function(c, "gluGetString", GLUModule::gluGetString, None,
                         Return, "string", Args, "int", End),

            new Function(c, "gluLookAt", GLUModule::gluLookAt, None, Return,
                         "void", Args, "float", "float", "float", "float",
                         "float", "float", "float", "float", "float", End),

            new Function(c, "gluLookAt", GLUModule::gluLookAtv, None, Return,
                         "void", Args, "vector float[3]", "vector float[3]",
                         "vector float[3]", End),

            new Function(c, "gluOrtho2D", GLUModule::gluOrtho2D, None, Return,
                         "void", Args, "float", "float", "float", "float", End),

            new Function(c, "gluPerspective", GLUModule::gluPerspective, None,
                         Return, "void", Args, "float", "float", "float",
                         "float", End),

            new Function(c, "gluScaleImage", GLUModule::gluScaleImage, None,
                         Return, "int", Args, "int", "int", "int", "?dyn_array",
                         "int", "int", "?dyn_array", End),

            EndArguments);
    }

    // *****************************************************************************

    NODE_IMPLEMENTATION(GLUModule::gluBuild2DMipmaps, int)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int target = NODE_ARG(0, int);
        int internalFormat = NODE_ARG(1, int);
        int width = NODE_ARG(2, int);
        int height = NODE_ARG(3, int);
        int format = NODE_ARG(4, int);

        DynamicArray* data = NODE_ARG_OBJECT(5, DynamicArray);
        //     if( DynamicArray *data = dynamic_cast<DynamicArray*>(arg5) )
        //     {
        const Type* dataType = data->elementType();

        GLenum type = 0;
        if (dataType == c->floatType() || dataType == c->vec3fType()
            || dataType == c->vec4fType())
        {
            type = GL_FLOAT;
        }
        else if (dataType == c->byteType())
        {
            type = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        NODE_RETURN(::gluBuild2DMipmaps(target, internalFormat, width, height,
                                        format, type, data->data<void>()));
        //     }
        //     else
        //     {
        //         throw IncompatableArraysException( NODE_THREAD );
        //     }
    }

    NODE_IMPLEMENTATION(GLUModule::gluCheckExtension, bool)
    {
        const StringType::String* a =
            NODE_ARG_OBJECT(0, const StringType::String);
        const StringType::String* b =
            NODE_ARG_OBJECT(1, const StringType::String);
        const char* extName = a->c_str();
        const char* extString = b->c_str();
        /* AJG - gluCheckExtension - doesn't exist? */
        //    NODE_RETURN( ::gluCheckExtension( extName, extString ) );
        NODE_RETURN(0);
    }

    NODE_IMPLEMENTATION(GLUModule::gluErrorString, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        if (const unsigned char* str = ::gluErrorString(NODE_ARG(0, int)))
        {
            NODE_RETURN(c->stringType()->allocate((const char*)str));
        }
        else
        {
            throw BadArgumentException(NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(GLUModule::gluGetString, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        if (const unsigned char* str = ::gluGetString(NODE_ARG(0, int)))
        {
            NODE_RETURN(c->stringType()->allocate((const char*)str));
        }
        else
        {
            throw BadArgumentException(NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(GLUModule::gluLookAt, void)
    {
        ::gluLookAt(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float),
                    NODE_ARG(3, float), NODE_ARG(4, float), NODE_ARG(5, float),
                    NODE_ARG(6, float), NODE_ARG(7, float), NODE_ARG(8, float));
    }

    NODE_IMPLEMENTATION(GLUModule::gluLookAtv, void)
    {
        const Vector3f& eye = NODE_ARG(0, Vector3f);
        const Vector3f& center = NODE_ARG(1, Vector3f);
        const Vector3f& up = NODE_ARG(2, Vector3f);
        ::gluLookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2],
                    up[0], up[1], up[2]);
    }

    NODE_IMPLEMENTATION(GLUModule::gluOrtho2D, void)
    {
        ::gluOrtho2D(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float),
                     NODE_ARG(3, float));
    }

    NODE_IMPLEMENTATION(GLUModule::gluPerspective, void)
    {
        ::gluPerspective(NODE_ARG(0, float), NODE_ARG(1, float),
                         NODE_ARG(2, float), NODE_ARG(3, float));
    }

    NODE_IMPLEMENTATION(GLUModule::gluScaleImage, int)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int format = NODE_ARG(0, int);
        int wIn = NODE_ARG(1, int);
        int hIn = NODE_ARG(2, int);

        DynamicArray* dataIn = NODE_ARG_OBJECT(3, DynamicArray);
        const Type* dataInType = dataIn->elementType();

        GLenum typeIn = 0;
        if (dataInType == c->floatType() || dataInType == c->vec3fType()
            || dataInType == c->vec4fType())
        {
            typeIn = GL_FLOAT;
        }
        else if (dataInType == c->byteType())
        {
            typeIn = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        int wOut = NODE_ARG(4, int);
        int hOut = NODE_ARG(5, int);

        DynamicArray* dataOut = NODE_ARG_OBJECT(6, DynamicArray);
        const Type* dataOutType = dataOut->elementType();

        GLenum typeOut = 0;
        if (dataOutType == c->floatType() || dataOutType == c->vec3fType()
            || dataOutType == c->vec4fType())
        {
            typeOut = GL_FLOAT;
        }
        else if (dataOutType == c->byteType())
        {
            typeOut = GL_UNSIGNED_BYTE;
        }
        else
        {
            throw IncompatableArraysException(NODE_THREAD);
        }

        int numElements = 0;
        switch (format)
        {
        case GL_COLOR_INDEX:
        case GL_STENCIL_INDEX:
        case GL_DEPTH_COMPONENT:
        case GL_RED:
        case GL_GREEN:
        case GL_BLUE:
        case GL_ALPHA:
        case GL_LUMINANCE:
            numElements = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            numElements = 2;
            break;
        case GL_RGB:
        case GL_BGR:
            numElements = 3;
            break;
        case GL_RGBA:
        case GL_BGRA:
            numElements = 4;
            break;
        default:
            throw BadArgumentException(NODE_THREAD);
        }

        dataOut->resize(wOut * hOut * numElements);

        NODE_RETURN(::gluScaleImage(format, wIn, hIn, typeIn,
                                    dataIn->data<void>(), wOut, hOut, typeOut,
                                    dataOut->data<void>()));
    }

} // namespace Mu

#ifdef _MSC_VER
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif
