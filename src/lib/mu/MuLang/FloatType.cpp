//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/FloatType.h>
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
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

    FloatType::FloatType(Context* c)
        : PrimitiveType(c, "float", FloatRep::rep())
    {
    }

    FloatType::~FloatType() {}

    PrimitiveObject* FloatType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value FloatType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._floatFunc)(*n, thread));
    }

    void FloatType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        float* fp = reinterpret_cast<float*>(p);
        *fp = (*n->func()._floatFunc)(*n, thread);
    }

    void FloatType::outputValue(ostream& o, const Value& value, bool full) const
    {
        float f = value._float;
        o << f << (floorf(f) == f ? ".0" : "");
    }

    void FloatType::outputValueRecursive(ostream& o, const ValuePointer p,
                                         ValueOutputState& state) const
    {
        float f = *reinterpret_cast<const float*>(p);
        o << f << (floorf(f) == f ? ".0" : "");
    }

    //
    //  These are the non-inlined versions of the functions
    //  in Native.h. They are passed in as the Compiled function
    //  for the ints.
    //

#define T Thread&

    float __C_float_float(T) { return 0; }

    float __C_float_float_floatAmp_(T, float& a) { return a; }

    float __C_Minus__float_float(T, float a) { return -a; }

    float __C_Plus__float_float_float(T, float a, float b) { return a + b; }

    float __C_Minus__float_float_float(T, float a, float b) { return a - b; }

    float __C_Star__float_float_float(T, float a, float b) { return a * b; }

    float __C_Slash__float_float_float(T, float a, float b) { return a / b; }

    float __C_PCent__float_float_float(T, float a, float b)
    {
        return ::fmod(a, b);
    }

    float __C_Caret__float_float_float(T, float a, float b)
    {
        return ::pow(a, b);
    }

    bool __C_GT__bool_float_float(T, float a, float b) { return a > b; }

    bool __C_LT__bool_float_float(T, float a, float b) { return a < b; }

    bool __C_GT_EQ__bool_float_float(T, float a, float b) { return a >= b; }

    bool __C_LT_EQ__bool_float_float(T, float a, float b) { return a <= b; }

    bool __C_EQ_EQ__bool_float_float(T, float a, float b) { return a == b; }

    bool __C_Bang_EQ__bool_float_float(T, float a, float b) { return a != b; }

    float& __C_EQ__floatAmp__floatAmp__float(T, float& a, float b)
    {
        return a = b;
    }

    float& __C_Plus_EQ__floatAmp__floatAmp__float(T, float& a, float b)
    {
        return a += b;
    }

    float& __C_Minus_EQ__floatAmp__floatAmp__float(T, float& a, float b)
    {
        return a -= b;
    }

    float& __C_Star_EQ__floatAmp__floatAmp__float(T, float& a, float b)
    {
        return a *= b;
    }

    float& __C_Slash_EQ__floatAmp__floatAmp__float(T, float& a, float b)
    {
        return a /= b;
    }

    float& __C_PCent_EQ__floatAmp__floatAmp__float(T, float& a, float b)
    {
        return a = ::fmod(a, b);
    }

    float& __C_Caret_EQ__floatAmp__floatAmp__float(T, float& a, float b)
    {
        return a = ::pow(a, b);
    }

    float __C_prePlus_Plus__float_floatAmp_(T, float& a) { return ++a; }

    float __C_postPlus_Plus__float_floatAmp_(T, float& a) { return a++; }

    float __C_preMinus_Minus__float_floatAmp_(T, float& a) { return --a; }

    float __C_postMinus_Minus__float_floatAmp_(T, float& a) { return a--; }

    float __C_float_float_int(T, int a) { return a; }

    float __C_float_float_int64(T, int64 a) { return a; }

    float __C_float_float_double(T, double a) { return a; }

    void __C_print_void_float(T, float a)
    {
        cout << "PRINT: " << a << endl << flush;
    }

    //
    //  This function is delt with directly by muc, it translates
    //  to an if statement. But we still need a compiled version for
    //  lambda expressions.
    //

    float __C_QMark_Colon__bool_float_float(T, bool p, float a, float b)
    {
        return p ? a : b;
    }

#undef T

    void FloatType::load()
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

#if ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 96))
        addSymbols(new SymbolicConstant(c, "max", this, Value(FLT_MAX)),
                   new SymbolicConstant(c, "min", this, Value(FLT_MIN)),
                   EndArguments);
#else
        addSymbols(
            new SymbolicConstant(c, "integral", this, Value(false)),
            new SymbolicConstant(c, "max", this,
                                 Value(numeric_limits<float>::max())),
            new SymbolicConstant(c, "min", this,
                                 Value(numeric_limits<float>::min())),
            new SymbolicConstant(c, "epsilon", this,
                                 Value(numeric_limits<float>::epsilon())),
            new SymbolicConstant(c, "digits", this,
                                 Value(numeric_limits<float>::digits)),
            new SymbolicConstant(c, "digits10", this,
                                 Value(numeric_limits<float>::digits10)),
            new SymbolicConstant(c, "infinity", this,
                                 Value(numeric_limits<float>::infinity())),
            new SymbolicConstant(c, "quiet_NaN", this,
                                 Value(numeric_limits<float>::quiet_NaN())),
            new SymbolicConstant(c, "signaling_NaN", this,
                                 Value(numeric_limits<float>::signaling_NaN())),
            new SymbolicConstant(c, "denorm_min", this,
                                 Value(numeric_limits<float>::denorm_min())),
            EndArguments);
#endif

        s->addSymbols(
            new ReferenceType(c, "float&", this),

            new Function(c, "float", FloatType::defaultFloat, Mapped, Compiled,
                         __C_float_float, Return, "float", End),

            new Function(c, "float", FloatType::dereference, Cast, Compiled,
                         __C_float_float_floatAmp_, Return, "float", Args,
                         "float&", End),

            new Function(c, "+", FloatType::add, CommOp, Compiled,
                         __C_Plus__float_float_float, Return, "float", Args,
                         "float", "float", End),

            new Function(c, "-", FloatType::sub, Op, Compiled,
                         __C_Minus__float_float_float, Return, "float", Args,
                         "float", "float", End),

            new Function(c, "-", FloatType::negate, Op, Compiled,
                         __C_Minus__float_float, Return, "float", Args, "float",
                         End),

            new Function(c, "*", FloatType::mult, CommOp, Compiled,
                         __C_Star__float_float_float, Return, "float", Args,
                         "float", "float", End),

            new Function(c, "/", FloatType::div, Op, Compiled,
                         __C_Slash__float_float_float, Return, "float", Args,
                         "float", "float", End),

            new Function(c, "%", FloatType::mod, Op, Compiled,
                         __C_PCent__float_float_float, Return, "float", Args,
                         "float", "float", End),

            new Function(c, "float", FloatType::int2float, Cast, Compiled,
                         __C_float_float_int, Return, "float", Args, "int",
                         End),

            new Function(c, "float", FloatType::double2float, Lossy, Compiled,
                         __C_float_float_double, Return, "float", Args,
                         "double", End),

            new Function(c, "float", FloatType::int642float, Cast, Compiled,
                         __C_float_float_int64, Return, "float", Args, "int64",
                         End),

            new Function(c, "=", FloatType::assign, AsOp, Compiled,
                         __C_EQ__floatAmp__floatAmp__float, Return, "float&",
                         Args, "float&", "float", End),

            new Function(c, "+=", FloatType::assignPlus, AsOp, Compiled,
                         __C_Plus_EQ__floatAmp__floatAmp__float, Return,
                         "float&", Args, "float&", "float", End),

            new Function(c, "-=", FloatType::assignSub, AsOp, Compiled,
                         __C_Minus_EQ__floatAmp__floatAmp__float, Return,
                         "float&", Args, "float&", "float", End),

            new Function(c, "*=", FloatType::assignMult, AsOp, Compiled,
                         __C_Star_EQ__floatAmp__floatAmp__float, Return,
                         "float&", Args, "float&", "float", End),

            new Function(c, "/=", FloatType::assignDiv, AsOp, Compiled,
                         __C_Slash_EQ__floatAmp__floatAmp__float, Return,
                         "float&", Args, "float&", "float", End),

            new Function(c, "%=", FloatType::assignMod, AsOp, Compiled,
                         __C_PCent_EQ__floatAmp__floatAmp__float, Return,
                         "float&", Args, "float&", "float", End),

            new Function(c, "?:", FloatType::conditionalExpr, Op, Compiled,
                         __C_QMark_Colon__bool_float_float, Return, "float",
                         Args, "bool", "float", "float", End),

            new Function(c, "print", FloatType::print, None, Compiled,
                         __C_print_void_float, Return, "void", Args, "float",
                         End),

            new Function(c, "==", FloatType::equals, CommOp, Compiled,
                         __C_EQ_EQ__bool_float_float, Return, "bool", Args,
                         "float", "float", End),

            new Function(c, "!=", FloatType::notEquals, CommOp, Compiled,
                         __C_Bang_EQ__bool_float_float, Return, "bool", Args,
                         "float", "float", End),

            new Function(c, ">=", FloatType::greaterThanEq, Op, Compiled,
                         __C_GT_EQ__bool_float_float, Return, "bool", Args,
                         "float", "float", End),

            new Function(c, "<=", FloatType::lessThanEq, Op, Compiled,
                         __C_LT_EQ__bool_float_float, Return, "bool", Args,
                         "float", "float", End),

            new Function(c, "<", FloatType::lessThan, Op, Compiled,
                         __C_LT__bool_float_float, Return, "bool", Args,
                         "float", "float", End),

            new Function(c, ">", FloatType::greaterThan, Op, Compiled,
                         __C_GT__bool_float_float, Return, "bool", Args,
                         "float", "float", End),

            new Function(c, "pre++", FloatType::preInc, Op, Compiled,
                         __C_prePlus_Plus__float_floatAmp_, Return, "float",
                         Args, "float&", End),

            new Function(c, "post++", FloatType::postInc, Op, Compiled,
                         __C_postPlus_Plus__float_floatAmp_, Return, "float",
                         Return, "float", Args, "float&", End),

            new Function(c, "pre--", FloatType::preDec, Op, Compiled,
                         __C_preMinus_Minus__float_floatAmp_, Return, "float",
                         Args, "float&", End),

            new Function(c, "post--", FloatType::postDec, Op, Compiled,
                         __C_postMinus_Minus__float_floatAmp_, Return, "float",
                         Args, "float&", End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(FloatType::dereference, float)
    {
        using namespace Mu;
        float* fp = reinterpret_cast<float*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*fp);
    }

    NODE_IMPLEMENTATION(FloatType::int2float, float)
    {
        NODE_RETURN(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(FloatType::double2float, float)
    {
        NODE_RETURN(NODE_ARG(0, double));
    }

    NODE_IMPLEMENTATION(FloatType::int642float, float)
    {
        NODE_RETURN(NODE_ARG(0, int64));
    }

    NODE_IMPLEMENTATION(FloatType::defaultFloat, float) { NODE_RETURN(0.0f); }

    NODE_IMPLEMENTATION(FloatType::print, void)
    {
        float f = NODE_ARG(0, float);
        cout << "PRINT: " << f << endl << flush;
    }

#define OP(name, op)                                           \
    NODE_IMPLEMENTATION(name, float)                           \
    {                                                          \
        NODE_RETURN(NODE_ARG(0, float) op NODE_ARG(1, float)); \
    }

    OP(FloatType::add, +)
    OP(FloatType::sub, -)
    OP(FloatType::mult, *)
    OP(FloatType::div, /)
#undef OP

#define RELOP(name, op)                                        \
    NODE_IMPLEMENTATION(name, bool)                            \
    {                                                          \
        NODE_RETURN(NODE_ARG(0, float) op NODE_ARG(1, float)); \
    }

    RELOP(FloatType::equals, ==)
    RELOP(FloatType::notEquals, !=)
    RELOP(FloatType::lessThan, <)
    RELOP(FloatType::greaterThan, >)
    RELOP(FloatType::lessThanEq, <=)
    RELOP(FloatType::greaterThanEq, >=)
#undef RELOP

    NODE_IMPLEMENTATION(FloatType::mod, float)
    {
        NODE_RETURN(::fmod(NODE_ARG(0, float), NODE_ARG(1, float)));
    }

    NODE_IMPLEMENTATION(FloatType::negate, float)
    {
        NODE_RETURN(-NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(FloatType::conditionalExpr, float)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, float)
                                      : NODE_ARG(2, float));
    }

    NODE_IMPLEMENTATION(FloatType::preInc, float)
    {
        using namespace Mu;
        float* fp = reinterpret_cast<float*>(NODE_ARG(0, Pointer));
        return ++(*fp);
    }

    NODE_IMPLEMENTATION(FloatType::postInc, float)
    {
        using namespace Mu;
        float* fp = reinterpret_cast<float*>(NODE_ARG(0, Pointer));
        return (*fp)++;
    }

    NODE_IMPLEMENTATION(FloatType::preDec, float)
    {
        using namespace Mu;
        float* fp = reinterpret_cast<float*>(NODE_ARG(0, Pointer));
        return --(*fp);
    }

    NODE_IMPLEMENTATION(FloatType::postDec, float)
    {
        using namespace Mu;
        float* fp = reinterpret_cast<float*>(NODE_ARG(0, Pointer));
        return (*fp)--;
    }

#define ASOP(name, op)                                              \
    NODE_IMPLEMENTATION(name, Pointer)                              \
    {                                                               \
        using namespace Mu;                                         \
        float* fp = reinterpret_cast<float*>(NODE_ARG(0, Pointer)); \
        float f = NODE_ARG(1, float);                               \
        (*fp) op f;                                                 \
        NODE_RETURN((Pointer)fp);                                   \
    }

    ASOP(FloatType::assign, =)
    ASOP(FloatType::assignPlus, +=)
    ASOP(FloatType::assignSub, -=)
    ASOP(FloatType::assignMult, *=)
    ASOP(FloatType::assignDiv, /=)
#undef ASOP

    NODE_IMPLEMENTATION(FloatType::assignMod, Pointer)
    {
        using namespace Mu;
        float* fp = reinterpret_cast<float*>(NODE_ARG(0, Pointer));
        (*fp) = ::fmod(*fp, NODE_ARG(1, float));
        NODE_RETURN((Pointer)fp);
    }

} // namespace Mu
