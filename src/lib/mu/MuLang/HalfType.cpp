//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/HalfType.h>
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/ParameterVariable.h>
#include <Mu/Value.h>
#include <iostream>
#include <math.h>
#if ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 96))
#include <float.h>
#else
#include <limits>
#endif
namespace Mu
{
    using namespace std;

    HalfType::HalfType(Context* c)
        : PrimitiveType(c, "half", ShortRep::rep())
    {
    }

    HalfType::~HalfType() {}

    PrimitiveObject* HalfType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value HalfType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._shortFunc)(*n, thread));
    }

    void HalfType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        short* sp = reinterpret_cast<short*>(p);
        *sp = (*n->func()._shortFunc)(*n, thread);
    }

    void HalfType::outputValue(ostream& o, const Value& value, bool full) const
    {
        half h = shortToHalf(value._short);
        float f = h;
        o << h << (floorf(f) == f ? ".0h" : "h");
    }

    void HalfType::outputValueRecursive(ostream& o, const ValuePointer p,
                                        ValueOutputState& state) const
    {
        half h = shortToHalf(*reinterpret_cast<const short*>(p));
        float f = h;
        o << h << (floorf(f) == f ? ".0h" : "h");
    }

    //
    //  These are the non-inlined versions of the functions
    //  in Native.h. They are passed in as the Compiled function
    //  for the ints.
    //

#define T Thread&

    short __C_half_half(T) { return 0; }

    short __C_half_half_halfAmp_(T, short& a) { return a; }

    short __C_Minus__half_half(T, short a)
    {
        return halfToShort(-shortToHalf(a));
    }

    short __C_Plus__half_half_half(T, short a, short b)
    {
        return short(shortToHalf(a) + shortToHalf(b));
    }

    short __C_Minus__half_half_half(T, short a, short b)
    {
        return halfToShort(shortToHalf(a) - shortToHalf(b));
    }

    short __C_Star__half_half_half(T, short a, short b)
    {
        return halfToShort(shortToHalf(a) * shortToHalf(b));
    }

    short __C_Slash__half_half_half(T, short a, short b)
    {
        return halfToShort(shortToHalf(a) / shortToHalf(b));
    }

    short __C_PCent__half_half_half(T, short a, short b)
    {
        return halfToShort(::fmod(shortToHalf(a), shortToHalf(b)));
    }

    short __C_Caret__half_half_half(T, short a, short b)
    {
        return halfToShort(::pow(shortToHalf(a), shortToHalf(b)));
    }

    bool __C_GT__bool_half_half(T, short a, short b)
    {
        return shortToHalf(a) > shortToHalf(b);
    }

    bool __C_LT__bool_half_half(T, short a, short b)
    {
        return shortToHalf(a) < shortToHalf(b);
    }

    bool __C_GT_EQ__bool_half_half(T, short a, short b)
    {
        return shortToHalf(a) >= shortToHalf(b);
    }

    bool __C_LT_EQ__bool_half_half(T, short a, short b)
    {
        return shortToHalf(a) <= shortToHalf(b);
    }

    bool __C_EQ_EQ__bool_half_half(T, short a, short b)
    {
        return shortToHalf(a) == shortToHalf(b);
    }

    bool __C_Bang_EQ__bool_half_half(T, short a, short b)
    {
        return shortToHalf(a) != shortToHalf(b);
    }

    short& __C_EQ__halfAmp__halfAmp__half(T, short& a, short b)
    {
        return a = b;
    }

    short& __C_Plus_EQ__halfAmp__halfAmp__half(T, short& a, short b)
    {
        return a = halfToShort(shortToHalf(a) + shortToHalf(b));
    }

    short& __C_Minus_EQ__halfAmp__halfAmp__half(T, short& a, short b)
    {
        return a = halfToShort(shortToHalf(a) - shortToHalf(b));
    }

    short& __C_Star_EQ__halfAmp__halfAmp__half(T, short& a, short b)
    {
        return a = halfToShort(shortToHalf(a) * shortToHalf(b));
    }

    short& __C_Slash_EQ__halfAmp__halfAmp__half(T, short& a, short b)
    {
        return a = halfToShort(shortToHalf(a) / shortToHalf(b));
    }

    short& __C_PCent_EQ__halfAmp__halfAmp__half(T, short& a, short b)
    {
        return a = halfToShort(::fmod(shortToHalf(a), shortToHalf(b)));
    }

    short& __C_Caret_EQ__halfAmp__halfAmp__half(T, short& a, short b)
    {
        return a = halfToShort(::pow(shortToHalf(a), shortToHalf(b)));
    }

    short __C_prePlus_Plus__half_halfAmp_(T, short& a)
    {
        half h = shortToHalf(a) + 1.f;
        a = halfToShort(h);
        return short(h - 1.0);
    }

    short __C_postPlus_Plus__half_halfAmp_(T, short& a)
    {
        half h = shortToHalf(a) + 1.f;
        a = halfToShort(h);
        return a;
    }

    short __C_preMinus_Minus__half_halfAmp_(T, short& a)
    {
        half h = shortToHalf(a) - 1.f;
        a = halfToShort(h);
        return short(h + 1.0);
    }

    short __C_postMinus_Minus__half_halfAmp_(T, short& a)
    {
        half h = shortToHalf(a) - 1.f;
        a = halfToShort(h);
        return a;
    }

    short __C_half_half_int(T, int a) { return halfToShort(float(a)); }

    short __C_half_half_int64(T, int64 a) { return halfToShort(float(a)); }

    short __C_half_half_float(T, float a) { return halfToShort(half(a)); }

    short __C_half_half_double(T, double a) { return halfToShort(half(a)); }

    float __C_float_float_half(T, short a) { return float(shortToHalf(a)); }

    void __C_print_void_half(T, half a)
    {
        cout << "PRINT: " << shortToHalf(a) << endl << flush;
    }

    //
    //  This function is delt with directly by muc, it translates
    //  to an if statement. But we still need a compiled version for
    //  lambda expressions.
    //

    short __C_QMark_Colon__bool_half_half(T, bool p, short a, short b)
    {
        return p ? a : b;
    }

#undef T

    static inline Value HValue(half h) { return Value(halfToShort(h)); }

    void HalfType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        //
        //  All of the Int functions are inlined in Native.h
        //  So here the NativeInlined attribute is set in
        //  for all function attributes.
        //

        Mapped |= NativeInlined;
        CommOp |= NativeInlined;
        Op |= NativeInlined;
        AsOp |= NativeInlined;
        Lossy |= NativeInlined;
        Cast |= NativeInlined;

        addSymbols(
            new SymbolicConstant(c, "integral", this, Value(false)),
            new SymbolicConstant(c, "max", this,
                                 HValue(numeric_limits<half>::max())),
            new SymbolicConstant(c, "min", this,
                                 HValue(numeric_limits<half>::min())),
            new SymbolicConstant(c, "epsilon", this,
                                 HValue(numeric_limits<half>::epsilon())),
            new SymbolicConstant(c, "digits", this,
                                 Value(numeric_limits<half>::digits)),
            new SymbolicConstant(c, "digits10", this,
                                 Value(numeric_limits<half>::digits10)),
            new SymbolicConstant(c, "infinity", this,
                                 HValue(numeric_limits<half>::infinity())),
            new SymbolicConstant(c, "quiet_NaN", this,
                                 HValue(numeric_limits<half>::quiet_NaN())),
            new SymbolicConstant(c, "signaling_NaN", this,
                                 HValue(numeric_limits<half>::signaling_NaN())),
            new SymbolicConstant(c, "denorm_min", this,
                                 HValue(numeric_limits<half>::denorm_min())),

            new Function(c, "round", HalfType::round, Pure, Return, "half",
                         Parameters, new ParameterVariable(c, "value", "half"),
                         new ParameterVariable(c, "bits", "int"), End),

            new Function(c, "bits", HalfType::bits, Pure, Return, "short", Args,
                         "half", End),

            new Function(c, "convert", HalfType::convert, Pure, Return, "half",
                         Args, "short", End),

            EndArguments);

        s->addSymbols(
            new ReferenceType(c, "half&", this),

            new Function(c, "half", HalfType::defaultHalf, Mapped, Compiled,
                         __C_half_half, Return, "half", End),

            new Function(c, "float", HalfType::toFloat, Cast, Compiled,
                         __C_float_float_half, Return, "float", Args, "half",
                         End),

            new Function(c, "half", HalfType::dereference, Cast, Compiled,
                         __C_half_half_halfAmp_, Return, "half", Args, "half&",
                         End),

            new Function(c, "+", HalfType::add, CommOp, Compiled,
                         __C_Plus__half_half_half, Return, "half", Args, "half",
                         "half", End),

            new Function(c, "-", HalfType::sub, Op, Compiled,
                         __C_Minus__half_half_half, Return, "half", Args,
                         "half", "half", End),

            new Function(c, "-", HalfType::negate, Op, Compiled,
                         __C_Minus__half_half, Return, "half", Args, "half",
                         End),

            new Function(c, "*", HalfType::mult, CommOp, Compiled,
                         __C_Star__half_half_half, Return, "half", Args, "half",
                         "half", End),

            new Function(c, "/", HalfType::div, Op, Compiled,
                         __C_Slash__half_half_half, Return, "half", Args,
                         "half", "half", End),

            new Function(c, "%", HalfType::mod, Op, Compiled,
                         __C_PCent__half_half_half, Return, "half", Args,
                         "half", "half", End),

            new Function(c, "half", HalfType::int2half, Cast, Compiled,
                         __C_half_half_int, Return, "half", Args, "int", End),

            new Function(c, "half", HalfType::int642half, Cast, Compiled,
                         __C_half_half_int64, Return, "half", Args, "int64",
                         End),

            new Function(c, "half", HalfType::float2half, Cast, Compiled,
                         __C_half_half_float, Return, "half", Args, "float",
                         End),

            new Function(c, "half", HalfType::double2half, Cast, Compiled,
                         __C_half_half_double, Return, "half", Args, "double",
                         End),

            new Function(c, "=", HalfType::assign, AsOp, Compiled,
                         __C_EQ__halfAmp__halfAmp__half, Return, "half&", Args,
                         "half&", "half", End),

            new Function(c, "+=", HalfType::assignPlus, AsOp, Compiled,
                         __C_Plus_EQ__halfAmp__halfAmp__half, Return, "half&",
                         Args, "half&", "half", End),

            new Function(c, "-=", HalfType::assignSub, AsOp, Compiled,
                         __C_Minus_EQ__halfAmp__halfAmp__half, Return, "half&",
                         Args, "half&", "half", End),

            new Function(c, "*=", HalfType::assignMult, AsOp, Compiled,
                         __C_Star_EQ__halfAmp__halfAmp__half, Return, "half&",
                         Args, "half&", "half", End),

            new Function(c, "/=", HalfType::assignDiv, AsOp, Compiled,
                         __C_Slash_EQ__halfAmp__halfAmp__half, Return, "half&",
                         Args, "half&", "half", End),

            new Function(c, "%=", HalfType::assignMod, AsOp, Compiled,
                         __C_PCent_EQ__halfAmp__halfAmp__half, Return, "half&",
                         Args, "half&", "half", End),

            new Function(c, "?:", HalfType::conditionalExpr, Op, Compiled,
                         __C_QMark_Colon__bool_half_half, Return, "half", Args,
                         "bool", "half", "half", End),

            new Function(c, "print", HalfType::print, None, Compiled,
                         __C_print_void_half, Return, "void", Args, "half",
                         End),

            new Function(c, "==", HalfType::equals, CommOp, Compiled,
                         __C_EQ_EQ__bool_half_half, Return, "bool", Args,
                         "half", "half", End),

            new Function(c, "!=", HalfType::notEquals, CommOp, Compiled,
                         __C_Bang_EQ__bool_half_half, Return, "bool", Args,
                         "half", "half", End),

            new Function(c, ">=", HalfType::greaterThanEq, Op, Compiled,
                         __C_GT_EQ__bool_half_half, Return, "bool", Args,
                         "half", "half", End),

            new Function(c, "<=", HalfType::lessThanEq, Op, Compiled,
                         __C_LT_EQ__bool_half_half, Return, "bool", Args,
                         "half", "half", End),

            new Function(c, "<", HalfType::lessThan, Op, Compiled,
                         __C_LT__bool_half_half, Return, "bool", Args, "half",
                         "half", End),

            new Function(c, ">", HalfType::greaterThan, Op, Compiled,
                         __C_GT__bool_half_half, Return, "bool", Args, "half",
                         "half", End),

            new Function(c, "pre++", HalfType::preInc, Op, Compiled,
                         __C_prePlus_Plus__half_halfAmp_, Return, "half", Args,
                         "half&", End),

            new Function(c, "post++", HalfType::postInc, Op, Compiled,
                         __C_postPlus_Plus__half_halfAmp_, Return, "half",
                         Return, "half", Args, "half&", End),

            new Function(c, "pre--", HalfType::preDec, Op, Compiled,
                         __C_preMinus_Minus__half_halfAmp_, Return, "half",
                         Args, "half&", End),

            new Function(c, "post--", HalfType::postDec, Op, Compiled,
                         __C_postMinus_Minus__half_halfAmp_, Return, "half",
                         Args, "half&", End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(HalfType::dereference, short)
    {
        half* hp = reinterpret_cast<half*>(NODE_ARG(0, Pointer));
        NODE_RETURN(halfToShort(*hp));
    }

    NODE_IMPLEMENTATION(HalfType::int2half, short)
    {
        NODE_RETURN_HALF(float(NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(HalfType::int642half, short)
    {
        NODE_RETURN_HALF(float(NODE_ARG(0, int64)));
    }

    NODE_IMPLEMENTATION(HalfType::float2half, short)
    {
        NODE_RETURN_HALF(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(HalfType::double2half, short)
    {
        NODE_RETURN_HALF(NODE_ARG(0, double));
    }

    NODE_IMPLEMENTATION(HalfType::defaultHalf, short)
    {
        NODE_RETURN(half(0.0f));
    }

    NODE_IMPLEMENTATION(HalfType::toFloat, float)
    {
        half h = NODE_ARG_HALF(0);
        NODE_RETURN(float(h));
    }

    NODE_IMPLEMENTATION(HalfType::print, void)
    {
        half f = NODE_ARG_HALF(0);
        cout << "PRINT: " << f << endl << flush;
    }

#define OP(name, op)                                                    \
    NODE_IMPLEMENTATION(name, short)                                    \
    {                                                                   \
        NODE_RETURN(halfToShort(NODE_ARG_HALF(0) op NODE_ARG_HALF(1))); \
    }

    OP(HalfType::add, +)
    OP(HalfType::sub, -)
    OP(HalfType::mult, *)
    OP(HalfType::div, /)
#undef OP

#define RELOP(name, op)                                    \
    NODE_IMPLEMENTATION(name, bool)                        \
    {                                                      \
        NODE_RETURN(NODE_ARG_HALF(0) op NODE_ARG_HALF(1)); \
    }

    RELOP(HalfType::equals, ==)
    RELOP(HalfType::notEquals, !=)
    RELOP(HalfType::lessThan, <)
    RELOP(HalfType::greaterThan, >)
    RELOP(HalfType::lessThanEq, <=)
    RELOP(HalfType::greaterThanEq, >=)
#undef RELOP

    NODE_IMPLEMENTATION(HalfType::mod, short)
    {
        NODE_RETURN_HALF(::fmod(NODE_ARG_HALF(0), NODE_ARG_HALF(1)));
    }

    NODE_IMPLEMENTATION(HalfType::negate, short)
    {
        NODE_RETURN_HALF(-NODE_ARG_HALF(0));
    }

    NODE_IMPLEMENTATION(HalfType::conditionalExpr, short)
    {
        NODE_RETURN_HALF(NODE_ARG(0, bool) ? NODE_ARG_HALF(1)
                                           : NODE_ARG_HALF(2));
    }

    NODE_IMPLEMENTATION(HalfType::preInc, short)
    {
        short* p = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        short n = *p;
        half h = shortToHalf(n);
        *p = halfToShort(h + 1.0f);
        NODE_RETURN(n);
    }

    NODE_IMPLEMENTATION(HalfType::postInc, short)
    {
        short* p = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        short n = *p;
        half h = shortToHalf(n);
        *p = halfToShort(h + 1.0f);
        NODE_RETURN(halfToShort(*p));
    }

    NODE_IMPLEMENTATION(HalfType::preDec, short)
    {
        short* p = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        short n = *p;
        half h = shortToHalf(n);
        *p = halfToShort(h - 1.0f);
        NODE_RETURN(n);
    }

    NODE_IMPLEMENTATION(HalfType::postDec, short)
    {
        short* p = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        short n = *p;
        half h = shortToHalf(n);
        *p = halfToShort(h - 1.0f);
        NODE_RETURN(halfToShort(*p));
    }

#define ASOP(name, op)                                            \
    NODE_IMPLEMENTATION(name, Pointer)                            \
    {                                                             \
        half* fp = reinterpret_cast<half*>(NODE_ARG(0, Pointer)); \
        half f = NODE_ARG_HALF(1);                                \
        (*fp) op f;                                               \
        NODE_RETURN((Pointer)fp);                                 \
    }

    ASOP(HalfType::assign, =)
    ASOP(HalfType::assignPlus, +=)
    ASOP(HalfType::assignSub, -=)
    ASOP(HalfType::assignMult, *=)
    ASOP(HalfType::assignDiv, /=)
#undef ASOP

    NODE_IMPLEMENTATION(HalfType::assignMod, Pointer)
    {
        half* fp = reinterpret_cast<half*>(NODE_ARG(0, Pointer));
        (*fp) = ::fmod(*fp, NODE_ARG_HALF(1));
        NODE_RETURN((Pointer)fp);
    }

    //-- members

    NODE_IMPLEMENTATION(HalfType::round, short)
    {
        half h = NODE_ARG_HALF(0);
        int bits = NODE_ARG(1, int);
        NODE_RETURN_HALF(h.round(bits));
    }

    NODE_IMPLEMENTATION(HalfType::bits, short)
    {
        half h = NODE_ARG_HALF(0);
        NODE_RETURN_HALF(h.bits());
    }

    NODE_IMPLEMENTATION(HalfType::convert, short)
    {
        half h;
        h.setBits(NODE_ARG(0, short));
        NODE_RETURN_HALF(h);
    }

} // namespace Mu
