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
#include <MuLang/FixedArrayType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef PLATFORM_DARWIN
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <TwkGLText/TwkGLText.h>

#include <MuGLText/GLTextModule.h>

namespace Mu
{

    using namespace std;
    using namespace TwkGLText;

    void GLTextModule::init()
    {
        static bool initialized = false;

        if (!initialized)
        {
            initialized = true;
        }
    }

    GLTextModule::GLTextModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    GLTextModule::~GLTextModule() {}

    void GLTextModule::load()
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

            new Function(c, "init", GLTextModule::TwkGLText_init, None, Return,
                         "void", Args, Optional, "string", End),

            new Function(c, "writeAt", GLTextModule::writeAt, None, Return,
                         "void", Args, "float", "float", "string", End),

            new Function(c, "writeAt", GLTextModule::writeAt, None, Return,
                         "void", Args, "vector float[2]", "string", End),

            new Function(c, "writeAtNL", GLTextModule::writeAtNLf, None, Return,
                         "void", Args, "float", "float", "string", Optional,
                         "float", End),

            new Function(c, "writeAtNL", GLTextModule::writeAtNLfv, None,
                         Return, "int", Args, "vector float[2]", "string",
                         Optional, "float", End),

            new Function(c, "size", GLTextModule::setSize, None, Return, "void",
                         Args, "int", End),

            new Function(c, "size", GLTextModule::getSize, None, Return, "int",
                         End),

            new Function(c, "bounds", GLTextModule::bounds, None, Return,
                         "float[4]", Args, "string", End),

            new Function(c, "boundsNL", GLTextModule::boundsNL, None, Return,
                         "float[4]", Args, "string", Optional, "float", End),

            new Function(c, "color", GLTextModule::color4f, None, Return,
                         "void", Args, "float", "float", "float", Optional,
                         "float", End),

            new Function(c, "color", GLTextModule::color3fv, None, Return,
                         "void", Args, "vector float[3]", End),

            new Function(c, "color", GLTextModule::color4fv, None, Return,
                         "void", Args, "vector float[4]", End),

            new Function(c, "width", GLTextModule::width, None, Return, "float",
                         Args, "string", End),

            new Function(c, "height", GLTextModule::height, None, Return,
                         "float", Args, "string", End),

            new Function(c, "widthNL", GLTextModule::widthNL, None, Return,
                         "float", Args, "string", End),

            new Function(c, "heightNL", GLTextModule::heightNL, None, Return,
                         "float", Args, "string", End),

            new Function(c, "ascenderHeight", GLTextModule::ascenderHeight,
                         None, Return, "float", End),

            new Function(c, "descenderDepth", GLTextModule::descenderDepth,
                         None, Return, "float", End),

            EndArguments);
    }

    // *****************************************************************************

    NODE_IMPLEMENTATION(GLTextModule::TwkGLText_init, void)
    {
        if (NODE_NUM_ARGS() == 0)
        {
            GLtext::init();
        }
        else if (NODE_NUM_ARGS() == 1)
        {
            const StringType::String* stext =
                NODE_ARG_OBJECT(0, const StringType::String);
            const char* fontName = stext->c_str();
            GLtext::init(fontName);
        }
    }

    static string removeReturns(string s)
    //
    //  Turn \r\n into \n, so that we don't get little boxes for the \r.  Add
    //  spaces to blank lines so they don't get collapsed.  Trim newlines from
    //  the end.
    //
    {
        size_t pos = 0;
        while ((pos = s.find("\r\n", pos)) != string::npos)
        {
            s.replace(pos, 2, "\n");
        }
        pos = 0;
        while ((pos = s.find("\n\n", pos)) != string::npos)
        {
            s.replace(pos, 2, "\n \n");
        }
        while (s.size() && s[s.size() - 1] == '\n')
            s.erase(s.size() - 1, 1);
        return s;
    }

    NODE_IMPLEMENTATION(GLTextModule::writeAt, void)
    {
        float x = 0;
        float y = 0;
        if (NODE_NUM_ARGS() == 2)
        {
            const Vector2f& p = NODE_ARG(0, Vector2f);
            x = p[0];
            y = p[1];
        }
        else
        {
            x = NODE_ARG(0, float);
            y = NODE_ARG(1, float);
        }
        const StringType::String* stext =
            NODE_ARG_OBJECT(2, const StringType::String);
        string s = removeReturns(stext->c_str());

        GLtext::writeAt(x, y, s.c_str());
    }

    NODE_IMPLEMENTATION(GLTextModule::writeAtNLf, int)
    {
        float x = NODE_ARG(0, float);
        float y = NODE_ARG(1, float);
        const StringType::String* stext =
            NODE_ARG_OBJECT(2, const StringType::String);
        string s = removeReturns(stext->c_str());
        float mult = 1.0f;
        if (NODE_NUM_ARGS() == 4)
        {
            mult = NODE_ARG(3, float);
        }

        NODE_RETURN(GLtext::writeAtNL(x, y, s.c_str(), mult));
    }

    NODE_IMPLEMENTATION(GLTextModule::writeAtNLfv, int)
    {
        const Vector2f& p = NODE_ARG(0, Vector2f);
        float x = p[0];
        float y = p[1];
        const StringType::String* stext =
            NODE_ARG_OBJECT(2, const StringType::String);
        string s = removeReturns(stext->c_str());
        float mult = 1.0f;
        if (NODE_NUM_ARGS() == 4)
        {
            mult = NODE_ARG(3, float);
        }

        NODE_RETURN(GLtext::writeAtNL(x, y, s.c_str(), mult));
    }

    NODE_IMPLEMENTATION(GLTextModule::setSize, void)
    {
        GLtext::size(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(GLTextModule::getSize, int)
    {
        NODE_RETURN(GLtext::size());
    }

    NODE_IMPLEMENTATION(GLTextModule::bounds, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const StringType::String* stext =
            NODE_ARG_OBJECT(0, const StringType::String);
        string s = removeReturns(stext->c_str());

        const FixedArrayType* atype =
            (FixedArrayType*)c->arrayType(c->floatType(), 1, 4);
        FixedArray* bounds = (FixedArray*)ClassInstance::allocate(atype);

        TwkMath::Box2f b = GLtext::bounds(s.c_str());

        bounds->element<float>(0) = b.min.x;
        bounds->element<float>(1) = b.min.y;
        bounds->element<float>(2) = b.max.x;
        bounds->element<float>(3) = b.max.y;

        NODE_RETURN(bounds);
    }

    NODE_IMPLEMENTATION(GLTextModule::boundsNL, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const StringType::String* stext =
            NODE_ARG_OBJECT(0, const StringType::String);
        string s = removeReturns(stext->c_str());

        float mult = 1.0f;
        if (NODE_NUM_ARGS() == 2)
        {
            mult = NODE_ARG(1, float);
        }

        const FixedArrayType* atype =
            (FixedArrayType*)c->arrayType(c->floatType(), 1, 4);
        FixedArray* bounds = (FixedArray*)ClassInstance::allocate(atype);

        TwkMath::Box2f b = GLtext::boundsNL(s.c_str(), mult);

        bounds->element<float>(0) = b.min.x;
        bounds->element<float>(1) = b.min.y;
        bounds->element<float>(2) = b.max.x;
        bounds->element<float>(3) = b.max.y;

        NODE_RETURN(bounds);
    }

    NODE_IMPLEMENTATION(GLTextModule::color4f, void)
    {
        float r = NODE_ARG(0, float);
        float g = NODE_ARG(1, float);
        float b = NODE_ARG(2, float);
        float a = 1.0f;
        if (NODE_NUM_ARGS() == 4)
        {
            a = NODE_ARG(3, float);
        }
        GLtext::color(r, g, b, a);
    }

    NODE_IMPLEMENTATION(GLTextModule::color3fv, void)
    {
        const Vector3f& c = NODE_ARG(0, Vector3f);
        GLtext::color(c[0], c[1], c[2]);
    }

    NODE_IMPLEMENTATION(GLTextModule::color4fv, void)
    {
        const Vector4f& c = NODE_ARG(0, Vector4f);
        GLtext::color(c[0], c[1], c[2], c[3]);
    }

    NODE_IMPLEMENTATION(GLTextModule::width, float)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const StringType::String* stext =
            NODE_ARG_OBJECT(0, const StringType::String);
        const char* text = stext->c_str();

        TwkMath::Box2f b = GLtext::bounds(text);
        NODE_RETURN(b.size().x);
    }

    NODE_IMPLEMENTATION(GLTextModule::height, float)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const StringType::String* stext =
            NODE_ARG_OBJECT(0, const StringType::String);
        const char* text = stext->c_str();

        TwkMath::Box2f b = GLtext::bounds(text);
        NODE_RETURN(b.size().y);
    }

    NODE_IMPLEMENTATION(GLTextModule::widthNL, float)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const StringType::String* stext =
            NODE_ARG_OBJECT(0, const StringType::String);
        const char* text = stext->c_str();

        TwkMath::Box2f b = GLtext::boundsNL(text);
        NODE_RETURN(b.size().x);
    }

    NODE_IMPLEMENTATION(GLTextModule::heightNL, float)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        const StringType::String* stext =
            NODE_ARG_OBJECT(0, const StringType::String);
        const char* text = stext->c_str();

        TwkMath::Box2f b = GLtext::boundsNL(text);
        NODE_RETURN(b.size().y);
    }

    NODE_IMPLEMENTATION(GLTextModule::ascenderHeight, float)
    {
        Process* p = NODE_THREAD.process();
        NODE_RETURN(GLtext::globalAscenderHeight());
    }

    NODE_IMPLEMENTATION(GLTextModule::descenderDepth, float)
    {
        Process* p = NODE_THREAD.process();
        NODE_RETURN(GLtext::globalDescenderHeight());
    }

} // namespace Mu

#ifdef _MSC_VER
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif
