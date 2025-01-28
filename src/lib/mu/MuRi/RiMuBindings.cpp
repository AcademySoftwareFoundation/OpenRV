//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Function.h>
#include <Mu/Exception.h>
#include <Mu/GarbageCollector.h>
#include <Mu/MachineRep.h>
#include <Mu/NodePrinter.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/SymbolTable.h>
#include <Mu/Thread.h>
#include <Mu/Type.h>
#include <Mu/Value.h>
#include <Mu/Vector.h>
#include <Mu/Symbol.h>
#include <Mu/Variable.h>
#include <Mu/MemberVariable.h>
#include <MuLang/FixedArray.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>

#include <RiMu/RiMuBindings.h>
#include <RiMu/RiMuException.h>

// Maximum number of parameter tokens per Ri call
// (eg. "P", "facevarying float foo", etc.)
#define MAX_PARAMETER_TOKENS 32

namespace RiMu
{

    using namespace Mu;

    LightHandles RiMuBindings::m_lightHandles;

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiAttributeBegin, void)
    {
        ::RiAttributeBegin();
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiAttributeEnd, void)
    {
        ::RiAttributeEnd();
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiAttribute, void)
    {
        int nPoints = 0;
        int numTokens = 0;
        RtToken tokens[MAX_PARAMETER_TOKENS];
        RtPointer pointers[MAX_PARAMETER_TOKENS];
        const char* name = NULL;

        name = NODE_ARG_OBJECT(0, StringType::String)->string().c_str();
        if (!parseTokens(NODE_THIS, NODE_THREAD, tokens, pointers, numTokens,
                         nPoints, 1))
        {
            std::cerr << "Error in parameters for RiAttribute" << std::endl;
            return;
        }
        ::RiAttributeV((RtToken)name, numTokens, tokens, pointers);

        freeTokens(tokens, numTokens, 1);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiTransformBegin, void)
    {
        ::RiTransformBegin();
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiTransformEnd, void)
    {
        ::RiTransformEnd();
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiTranslatef, void)
    {
        ::RiTranslate(NODE_ARG(0, float), NODE_ARG(1, float),
                      NODE_ARG(2, float));
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiTranslatev, void)
    {
        Vector3f v = NODE_ARG(0, Vector3f);
        ::RiTranslate(v[0], v[1], v[2]);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiRotate, void)
    {
        ::RiRotate(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float),
                   NODE_ARG(3, float));
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiScale, void)
    {
        ::RiScale(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float));
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiTransformM, void)
    {
        FixedArray* t = NODE_ARG_OBJECT(0, FixedArray);
        RtMatrix rt;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                rt[r][c] = t->element<float>(r, c);

        ::RiTransform(rt);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiTransformF, void)
    {
        RtMatrix t;
        t[0][0] = NODE_ARG(0, float);
        t[0][1] = NODE_ARG(1, float);
        t[0][2] = NODE_ARG(2, float);
        t[0][3] = NODE_ARG(3, float);
        t[1][0] = NODE_ARG(4, float);
        t[1][1] = NODE_ARG(5, float);
        t[1][2] = NODE_ARG(6, float);
        t[1][3] = NODE_ARG(7, float);
        t[2][0] = NODE_ARG(8, float);
        t[2][1] = NODE_ARG(9, float);
        t[2][2] = NODE_ARG(10, float);
        t[2][3] = NODE_ARG(11, float);
        t[3][0] = NODE_ARG(12, float);
        t[3][1] = NODE_ARG(13, float);
        t[3][2] = NODE_ARG(14, float);
        t[3][3] = NODE_ARG(15, float);
        ::RiTransform(t);
    }

    NODE_IMPLEMENTATION(RiMuBindings::RiConcatTransformM, void)
    {
        FixedArray* t = NODE_ARG_OBJECT(0, FixedArray);
        RtMatrix rt;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                rt[r][c] = t->element<float>(r, c);

        ::RiConcatTransform(rt);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiConcatTransformF, void)
    {
        RtMatrix t;
        t[0][0] = NODE_ARG(0, float);
        t[0][1] = NODE_ARG(1, float);
        t[0][2] = NODE_ARG(2, float);
        t[0][3] = NODE_ARG(3, float);
        t[1][0] = NODE_ARG(4, float);
        t[1][1] = NODE_ARG(5, float);
        t[1][2] = NODE_ARG(6, float);
        t[1][3] = NODE_ARG(7, float);
        t[2][0] = NODE_ARG(8, float);
        t[2][1] = NODE_ARG(9, float);
        t[2][2] = NODE_ARG(10, float);
        t[2][3] = NODE_ARG(11, float);
        t[3][0] = NODE_ARG(12, float);
        t[3][1] = NODE_ARG(13, float);
        t[3][2] = NODE_ARG(14, float);
        t[3][3] = NODE_ARG(15, float);
        ::RiConcatTransform(t);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiMotionBegin, void)
    {
        ::RiMotionBegin(2, NODE_ARG(0, float), NODE_ARG(1, float), NULL);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiMotionEnd, void) { ::RiMotionEnd(); }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiColor, void)
    {
        RtColor Cs = {NODE_ARG(0, float), NODE_ARG(1, float),
                      NODE_ARG(2, float)};
        ::RiColor(Cs);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiColorf, void)
    {
        Vector3f v = NODE_ARG(0, Vector3f);
        RtColor Cs = {v[0], v[1], v[2]};
        ::RiColor(Cs);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiOpacity, void)
    {
        RtColor Os;
        Os[0] = NODE_ARG(0, float);

        if (NODE_NUM_ARGS() > 1)
        {
            Os[1] = NODE_ARG(1, float);
            Os[2] = NODE_ARG(2, float);
        }
        else
        {
            Os[1] = NODE_ARG(0, float);
            Os[2] = NODE_ARG(0, float);
        }

        ::RiOpacity(Os);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiOpacityf, void)
    {
        Vector3f v = NODE_ARG(0, Vector3f);
        RtColor Os = {v[0], v[1], v[2]};
        ::RiOpacity(Os);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiMatte, void)
    {
        ::RiMatte((RtBoolean)NODE_ARG(0, bool));
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiSurface, void)
    {
        const char* shaderName =
            NODE_ARG_OBJECT(0, StringType::String)->string().c_str();

        int nPoints = 0;
        int numTokens = 0;
        RtToken tokens[MAX_PARAMETER_TOKENS];
        RtPointer pointers[MAX_PARAMETER_TOKENS];

        if (!parseTokens(NODE_THIS, NODE_THREAD, tokens, pointers, numTokens,
                         nPoints, 1))
        {
            std::cerr << "Error in parameters for RiSurface" << std::endl;
            return;
        }

        ::RiSurfaceV((char*)shaderName, numTokens, tokens, pointers);

        freeTokens(tokens, numTokens, 1);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiDisplacement, void)
    {
        const char* shaderName =
            NODE_ARG_OBJECT(0, StringType::String)->string().c_str();

        int nPoints = 0;
        int numTokens = 0;
        RtToken tokens[MAX_PARAMETER_TOKENS];
        RtPointer pointers[MAX_PARAMETER_TOKENS];

        if (!parseTokens(NODE_THIS, NODE_THREAD, tokens, pointers, numTokens,
                         nPoints, 1))
        {
            std::cerr << "Error in parameters for RiDisplacement" << std::endl;
            return;
        }

        ::RiDisplacementV((char*)shaderName, numTokens, tokens, pointers);

        freeTokens(tokens, numTokens, 1);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiLightSource, int)
    {
        const char* shaderName =
            NODE_ARG_OBJECT(0, StringType::String)->string().c_str();

        int nPoints = 0;
        int numTokens = 0;
        RtToken tokens[MAX_PARAMETER_TOKENS];
        RtPointer pointers[MAX_PARAMETER_TOKENS];

        if (!parseTokens(NODE_THIS, NODE_THREAD, tokens, pointers, numTokens,
                         nPoints, 1))
        {
            std::cerr << "Error in parameters for RiLightSource" << std::endl;
            return -1;
        }

        RtLightHandle light =
            ::RiLightSourceV((char*)shaderName, numTokens, tokens, pointers);

        m_lightHandles.push_back(light);

        freeTokens(tokens, numTokens, 1);

        return m_lightHandles.size();
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiIlluminate, void)
    {
        int lightId = NODE_ARG(0, int);
        bool onOff = NODE_ARG(1, bool);

        if (lightId > m_lightHandles.size())
        {
            std::cerr << "Invalid light handle:" << lightId << std::endl;
            return;
        }

        ::RiIlluminate(m_lightHandles[lightId - 1], (RtBoolean)onOff);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiSphere, void)
    {
        ::RiSphere(NODE_ARG(0, float), NODE_ARG(1, float), NODE_ARG(2, float),
                   NODE_ARG(3, float), NULL);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiBasis, void)
    {
        StringType::String* uBasisName = NODE_ARG_OBJECT(0, StringType::String);
        int uStep = NODE_ARG(1, int);
        StringType::String* vBasisName = NODE_ARG_OBJECT(2, StringType::String);
        int vStep = NODE_ARG(3, int);

        ::RiBasis(basisFromStr(uBasisName->string().c_str()), uStep,
                  basisFromStr(vBasisName->string().c_str()), vStep);
    }

    // *****************************************************************************
    RtBasis& RiMuBindings::basisFromStr(std::string name)
    {
        if (name == "Bezier")
            return RiBezierBasis;
        if (name == "BSpline")
            return RiBSplineBasis;
        if (name == "CatmullRom")
            return RiCatmullRomBasis;
        if (name == "Hermite")
            return RiHermiteBasis;
        if (name == "Power")
            return RiPowerBasis;
        TWK_EXC_THROW_WHAT(Exception, "Invalid basis: " + name);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiPoints, void)
    {
        int nPoints = 0;
        int numTokens = 0;
        RtToken tokens[MAX_PARAMETER_TOKENS];
        RtPointer pointers[MAX_PARAMETER_TOKENS];

        if (!parseTokens(NODE_THIS, NODE_THREAD, tokens, pointers, numTokens,
                         nPoints))
        {
            std::cerr << "Error in parameters for RiPoints" << std::endl;
            return;
        }

        ::RiPointsV(nPoints, numTokens, tokens, pointers);

        freeTokens(tokens, numTokens);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiCurves, void)
    {
        int nPoints = 0;
        int numTokens = 0;
        RtToken tokens[MAX_PARAMETER_TOKENS];
        RtPointer pointers[MAX_PARAMETER_TOKENS];

        const char* degree = NULL;
        int nCurves = 0;
        int* nVerts = NULL;
        const char* wrap = NULL;

        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        degree = NODE_ARG_OBJECT(0, StringType::String)->string().c_str();
        nCurves = NODE_ARG(1, int);
        nVerts = NODE_ARG_OBJECT(2, DynamicArray)->data<int>();
        wrap = NODE_ARG_OBJECT(3, StringType::String)->string().c_str();

        if (!parseTokens(NODE_THIS, NODE_THREAD, tokens, pointers, numTokens,
                         nPoints, 4))
        {
            std::cerr << "Error in parameters for RiPoints" << std::endl;
            return;
        }

        ::RiCurvesV((RtToken)degree, nCurves, nVerts, (RtToken)wrap, numTokens,
                    tokens, pointers);

        freeTokens(tokens, numTokens, 4);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiPolygon, void)
    {
        int nPoints = 0;
        int numTokens = 0;
        RtToken tokens[MAX_PARAMETER_TOKENS];
        RtPointer pointers[MAX_PARAMETER_TOKENS];

        if (!parseTokens(NODE_THIS, NODE_THREAD, tokens, pointers, numTokens,
                         nPoints))
        {
            std::cerr << "Error in parameters for RiPolygon" << std::endl;
            return;
        }

        ::RiPolygonV(nPoints, numTokens, tokens, pointers);

        freeTokens(tokens, numTokens);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiProcedural, void)
    {
        StringType::String* pluginName = NODE_ARG_OBJECT(0, StringType::String);
        StringType::String* cfgString = NODE_ARG_OBJECT(1, StringType::String);
        RtBound bounds = {-1e6, 1e6, -1e6, 1e6, -1e6, 1e6};
        if (NODE_NUM_ARGS() == 3)
        {
            FixedArray* b = NODE_ARG_OBJECT(2, FixedArray);
            for (int i = 0; i < 6; ++i)
                bounds[i] = b->element<float>(i);
        }

        RtString* twoStrings = (RtString*)malloc(2 * sizeof(RtString));
        twoStrings[0] = strdup(pluginName->string().c_str());
        twoStrings[1] = strdup(cfgString->string().c_str());
        ::RiProcedural(twoStrings, bounds, RiProcDynamicLoad,
                       RiProceduralFinished);
    }

    // *****************************************************************************
    void RiMuBindings::RiProceduralFinished(RtPointer data)
    {
        RtString* twoStrings = (RtString*)data;
        free((void*)(twoStrings[0]));
        free((void*)(twoStrings[1]));
        free(data);
    }

    // *****************************************************************************
    NODE_IMPLEMENTATION(RiMuBindings::RiTransformPoints, bool)
    {
        std::string fromspace = NODE_ARG_OBJECT(0, StringType::String)->c_str();
        std::string tospace = NODE_ARG_OBJECT(1, StringType::String)->c_str();
        DynamicArray* verts = NODE_ARG_OBJECT(2, DynamicArray);

        RtPoint* result = ::RiTransformPoints(
            (char*)fromspace.c_str(), (char*)tospace.c_str(), verts->size(),
            verts->data<RtPoint>());
        if (!result)
        {
            NODE_RETURN(false);
        }
        NODE_RETURN(true);
    }

    // *****************************************************************************
    bool RiMuBindings::parseTokens(const Mu::Node& node_, Mu::Thread& thread_,
                                   RtToken* tokens, RtPointer* pointers,
                                   int& numTokens, int& numP, int startAtArg)
    {
        numTokens = 0;
        numP = 0;

        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        int nArgs = NODE_NUM_ARGS();
        for (int a = startAtArg; a < nArgs; ++a)
        {
            const Type* t = NODE_THIS.argNode(a)->type();
            if (t != c->stringType() && t != c->charType())
            {
                std::cerr << "Expected a string token, but got "
                          << "data of type '" << t->name().c_str() << "'"
                          << std::endl;
                return false;
            }

            bool castToFloat = false;
            if (t == c->stringType())
            {
                StringType::String* token =
                    NODE_ARG_OBJECT(a, StringType::String);
                tokens[numTokens] = strdup((char*)token->string().c_str());

                // This is needed if someone declares a parameter as "float",
                // but passes an int.  Valid only for single values!
                if (token->string().substr(0, 5) == "float")
                {
                    castToFloat = true;
                }
            }
            else
            {
                char tmpStr[2];
                tmpStr[0] = NODE_ARG(a, char);
                tmpStr[1] = '\0';
                tokens[numTokens] = strdup(tmpStr);
            }

            // Move to the next argument...
            ++a;

            if (tokens[numTokens] == std::string("P"))
            {
                DynamicArray* P = NODE_ARG_OBJECT(a, DynamicArray);
                numP = P->size();
            }

            t = NODE_THIS.argNode(a)->type();

            // Float arrays...
            if ((t == c->arrayType(c->floatType(), 1, 0))
                || (t == c->arrayType(c->vec3fType(), 1, 0)))
            {
                DynamicArray* da = NODE_ARG_OBJECT(a, DynamicArray);
                pointers[numTokens] = da->data<float>();
            }
            // Int arrays...
            else if (t == c->arrayType(c->intType(), 1, 0))
            {
                DynamicArray* da = NODE_ARG_OBJECT(a, DynamicArray);
                pointers[numTokens] = da->data<int>();
            }
            // String...
            else if (t == c->stringType())
            {
                StringType::String* str =
                    NODE_ARG_OBJECT(a, StringType::String);
                char** stringPointers = new (char*)[1];
                stringPointers[0] = strdup((char*)str->string().c_str());
                pointers[numTokens] = stringPointers;
            }
            // Single vec3
            else if (t == c->vec3fType())
            {
                Vector3f* v = new Vector3f;
                *v = NODE_ARG(a, Vector3f);
                pointers[numTokens] = v;
            }
            // Single float
            else if (t == c->floatType())
            {
                float* f = new float;
                *f = NODE_ARG(a, float);
                pointers[numTokens] = f;
            }
            // Single int
            else if (t == c->intType())
            {
                if (castToFloat) // See above
                {
                    float* f = new float;
                    *f = (float)(NODE_ARG(a, int));
                    pointers[numTokens] = f;
                }
                else
                {
                    int* i = new int;
                    *i = NODE_ARG(a, int);
                    pointers[numTokens] = i;
                }
            }
            else
            {
                std::cerr << "RiMu::Unknown data type: '" << t->name().c_str()
                          << "'" << std::endl;
                return false;
            }

            ++numTokens;
        }
        return true;
    }

    // *****************************************************************************
    void RiMuBindings::freeTokens(RtToken* tokens, int numTokens,
                                  int startAtArg)
    {
        for (size_t i = startAtArg; i < numTokens; ++i)
        {
            free(tokens[i]);
        }
    }

    // *****************************************************************************
    void RiMuBindings::addSymbols(Context* context)
    {
        USING_MU_FUNCTION_SYMBOLS;

        MuLangContext* mlc = static_cast<MuLangContext*>(context);
        mlc->arrayType(mlc->floatType(), 1, 6);
        mlc->arrayType(mlc->floatType(), 2, 4, 4);
        mlc->arrayType(mlc->intType(), 1, 0);
        mlc->arrayType(mlc->vec3fType(), 1, 0);

        Context* c = context;

        context->globalScope()->addSymbols(
            new Function(c, "RiAttributeBegin", RiMuBindings::RiAttributeBegin,
                         NoHint, Return, "void", End),

            new Function(c, "RiAttributeEnd", RiMuBindings::RiAttributeEnd,
                         NoHint, Return, "void", End),

            new Function(c, "RiAttribute", RiMuBindings::RiAttribute, NoHint,
                         Args, "string", "?varargs", Maximum,
                         MAX_PARAMETER_TOKENS, Return, "void", End),

            new Function(c, "RiTransformBegin", RiMuBindings::RiTransformBegin,
                         NoHint, Return, "void", End),

            new Function(c, "RiTransformEnd", RiMuBindings::RiTransformEnd,
                         NoHint, Return, "void", End),

            new Function(c, "RiTranslate", RiMuBindings::RiTranslatef, NoHint,
                         Return, "void", Args, "float", "float", "float", End),

            new Function(c, "RiTranslate", RiMuBindings::RiTranslatev, NoHint,
                         Return, "void", Args, "vector float[3]", End),

            new Function(c, "RiRotate", RiMuBindings::RiRotate, NoHint, Return,
                         "void", Args, "float", "float", "float", "float", End),

            new Function(c, "RiScale", RiMuBindings::RiScale, NoHint, Return,
                         "void", Args, "float", "float", "float", End),

            new Function(c, "RiTransform", RiMuBindings::RiTransformF, NoHint,
                         Return, "void", Args, "float", "float", "float",
                         "float", "float", "float", "float", "float", "float",
                         "float", "float", "float", "float", "float", "float",
                         "float", End),

            new Function(c, "RiTransform", RiMuBindings::RiTransformM, NoHint,
                         Return, "void", Args, "float[4,4]", End),

            new Function(
                c, "RiConcatTransform", RiMuBindings::RiConcatTransformF,
                NoHint, Return, "void", Args, "float", "float", "float",
                "float", "float", "float", "float", "float", "float", "float",
                "float", "float", "float", "float", "float", "float", End),

            new Function(c, "RiConcatTransform",
                         RiMuBindings::RiConcatTransformM, NoHint, Return,
                         "void", Args, "float[4,4]", End),

            new Function(c, "RiMotionBegin", RiMuBindings::RiMotionBegin,
                         NoHint, Return, "void", Args, "float", "float", End),

            new Function(c, "RiMotionEnd", RiMuBindings::RiMotionEnd, NoHint,
                         Return, "void", End),

            new Function(c, "RiColor", RiMuBindings::RiColor, NoHint, Return,
                         "void", Args, "float", "float", "float", End),

            new Function(c, "RiColor", RiMuBindings::RiColorf, NoHint, Return,
                         "void", Args, "vector float[3]", End),

            new Function(c, "RiOpacity", RiMuBindings::RiOpacity, NoHint,
                         Return, "void", Args, "float", Optional, "float",
                         "float", End),

            new Function(c, "RiOpacity", RiMuBindings::RiOpacityf, NoHint,
                         Return, "void", Args, "vector float[3]", End),

            new Function(c, "RiMatte", RiMuBindings::RiMatte, NoHint, Return,
                         "void", Args, "bool", End),

            new Function(c, "RiSurface", RiMuBindings::RiSurface, NoHint,
                         Return, "void", Args, "string", Optional, "?varargs",
                         Maximum, MAX_PARAMETER_TOKENS, End),

            new Function(c, "RiDisplacement", RiMuBindings::RiDisplacement,
                         NoHint, Return, "void", Args, "string", Optional,
                         "?varargs", Maximum, MAX_PARAMETER_TOKENS, End),

            new Function(c, "RiLightSource", RiMuBindings::RiLightSource,
                         NoHint, Return, "int", Args, "string", Optional,
                         "?varargs", Maximum, MAX_PARAMETER_TOKENS, End),

            new Function(c, "RiIlluminate", RiMuBindings::RiIlluminate, NoHint,
                         Return, "void", Args, "int", "bool", End),

            new Function(c, "RiSphere", RiMuBindings::RiSphere, NoHint, Return,
                         "void", Args, "float", "float", "float", "float", End),

            new Function(c, "RiBasis", RiMuBindings::RiBasis, NoHint, Return,
                         "void", Args, "string", "int", "string", "int", End),

            new Function(c, "RiPoints", RiMuBindings::RiPoints, NoHint, Return,
                         "void", Args, "string", "?varargs", Maximum,
                         MAX_PARAMETER_TOKENS, End),

            new Function(c, "RiCurves", RiMuBindings::RiCurves, NoHint, Return,
                         "void", Args, "string", "int", "int[]", "string",
                         "?varargs", Maximum, MAX_PARAMETER_TOKENS, End),

            new Function(c, "RiPolygon", RiMuBindings::RiPolygon, NoHint,
                         Return, "void", Args, "?varargs", Maximum,
                         MAX_PARAMETER_TOKENS, End),

            new Function(c, "RiProcedural", RiMuBindings::RiProcedural, NoHint,
                         Return, "void", Args, "string", "string", Optional,
                         "float[6]", End),

            new Function(c, "RiTransformPoints",
                         RiMuBindings::RiTransformPoints, NoHint, Return,
                         "bool", Args, "string", "string",
                         "(vector float[3])[]", End),

            Symbol::EndArguments); // End addSymbols
    }

} //  End namespace RiMu
