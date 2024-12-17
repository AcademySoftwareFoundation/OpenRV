//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/DoubleType.h>
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

    DoubleType::DoubleType(Context* c)
        : PrimitiveType(c, "double", DoubleRep::rep())
    {
    }

    DoubleType::~DoubleType() {}

    PrimitiveObject* DoubleType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value DoubleType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._doubleFunc)(*n, thread));
    }

    void DoubleType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        double* fp = reinterpret_cast<double*>(p);
        *fp = (*n->func()._doubleFunc)(*n, thread);
    }

    void DoubleType::outputValue(ostream& o, const Value& value,
                                 bool full) const
    {
        double f = value._double;
        o << f << (floor(f) == f ? ".0" : "");
    }

    void DoubleType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                          ValueOutputState& state) const
    {
        double f = *reinterpret_cast<const double*>(vp);
        o << f << (floor(f) == f ? ".0" : "");
    }

    //
    //  These are the non-inlined versions of the functions
    //  in Native.h. They are passed in as the Compiled function
    //  for the ints.
    //

#define T Thread&

    double __C_double_double(T) { return 0; }

    double __C_double_double_doubleAmp_(T, double& a) { return a; }

    double __C_Minus__double_double(T, double a) { return -a; }

    double __C_Plus__double_double_double(T, double a, double b)
    {
        return a + b;
    }

    double __C_Minus__double_double_double(T, double a, double b)
    {
        return a - b;
    }

    double __C_Star__double_double_double(T, double a, double b)
    {
        return a * b;
    }

    double __C_Slash__double_double_double(T, double a, double b)
    {
        return a / b;
    }

    double __C_PCent__double_double_double(T, double a, double b)
    {
        return ::fmod(a, b);
    }

    double __C_Caret__double_double_double(T, double a, double b)
    {
        return ::pow(a, b);
    }

    bool __C_GT__bool_double_double(T, double a, double b) { return a > b; }

    bool __C_LT__bool_double_double(T, double a, double b) { return a < b; }

    bool __C_GT_EQ__bool_double_double(T, double a, double b) { return a >= b; }

    bool __C_LT_EQ__bool_double_double(T, double a, double b) { return a <= b; }

    bool __C_EQ_EQ__bool_double_double(T, double a, double b) { return a == b; }

    bool __C_Bang_EQ__bool_double_double(T, double a, double b)
    {
        return a != b;
    }

    double& __C_EQ__doubleAmp__doubleAmp__double(T, double& a, double b)
    {
        return a = b;
    }

    double& __C_Plus_EQ__doubleAmp__doubleAmp__double(T, double& a, double b)
    {
        return a += b;
    }

    double& __C_Minus_EQ__doubleAmp__doubleAmp__double(T, double& a, double b)
    {
        return a -= b;
    }

    double& __C_Star_EQ__doubleAmp__doubleAmp__double(T, double& a, double b)
    {
        return a *= b;
    }

    double& __C_Slash_EQ__doubleAmp__doubleAmp__double(T, double& a, double b)
    {
        return a /= b;
    }

    double& __C_PCent_EQ__doubleAmp__doubleAmp__double(T, double& a, double b)
    {
        return a = ::fmod(a, b);
    }

    double& __C_Caret_EQ__doubleAmp__doubleAmp__double(T, double& a, double b)
    {
        return a = ::pow(a, b);
    }

    double __C_prePlus_Plus__double_doubleAmp_(T, double& a) { return ++a; }

    double __C_postPlus_Plus__double_doubleAmp_(T, double& a) { return a++; }

    double __C_preMinus_Minus__double_doubleAmp_(T, double& a) { return --a; }

    double __C_postMinus_Minus__double_doubleAmp_(T, double& a) { return a--; }

    double __C_double_double_int(T, int a) { return a; }

    double __C_double_double_int64(T, int64 a) { return a; }

    double __C_double_double_float(T, float a) { return a; }

    void __C_print_void_double(T, double a)
    {
        cout << "PRINT: " << a << endl << flush;
    }

    //
    //  This function is delt with directly by muc, it translates
    //  to an if statement. But we still need a compiled version for
    //  lambda expressions.
    //

    double __C_QMark_Colon__bool_double_double(T, bool p, double a, double b)
    {
        return p ? a : b;
    }

#undef T

    void DoubleType::load()
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
        addSymbols(new SymbolicConstant(c, "max", this, Value(DBL_MAX)),
                   new SymbolicConstant(c, "min", this, Value(DBL_MIN)),
                   EndArguments);
#else
        addSymbols(
            new SymbolicConstant(c, "integral", this, Value(false)),
            new SymbolicConstant(c, "max", this,
                                 Value(numeric_limits<double>::max())),
            new SymbolicConstant(c, "min", this,
                                 Value(numeric_limits<double>::min())),
            new SymbolicConstant(c, "epsilon", this,
                                 Value(numeric_limits<double>::epsilon())),
            new SymbolicConstant(c, "digits", this,
                                 Value(numeric_limits<double>::digits)),
            new SymbolicConstant(c, "digits10", this,
                                 Value(numeric_limits<double>::digits10)),
            new SymbolicConstant(c, "infinity", this,
                                 Value(numeric_limits<double>::infinity())),
            new SymbolicConstant(c, "quiet_NaN", this,
                                 Value(numeric_limits<double>::quiet_NaN())),
            new SymbolicConstant(
                c, "signaling_NaN", this,
                Value(numeric_limits<double>::signaling_NaN())),
            new SymbolicConstant(c, "denorm_min", this,
                                 Value(numeric_limits<double>::denorm_min())),
            EndArguments);
#endif

        s->addSymbols(
            new ReferenceType(c, "double&", this),

            new Function(c, "double", DoubleType::defaultDouble, Mapped,
                         Compiled, __C_double_double, Return, "double", End),

            new Function(c, "double", DoubleType::dereference, Cast, Compiled,
                         __C_double_double_doubleAmp_, Return, "double", Args,
                         "double&", End),

            new Function(c, "+", DoubleType::add, CommOp, Compiled,
                         __C_Plus__double_double_double, Return, "double", Args,
                         "double", "double", End),

            new Function(c, "-", DoubleType::sub, Op, Compiled,
                         __C_Minus__double_double_double, Return, "double",
                         Args, "double", "double", End),

            new Function(c, "-", DoubleType::negate, Op, Compiled,
                         __C_Minus__double_double, Return, "double", Args,
                         "double", End),

            new Function(c, "*", DoubleType::mult, CommOp, Compiled,
                         __C_Star__double_double_double, Return, "double", Args,
                         "double", "double", End),

            new Function(c, "/", DoubleType::div, Op, Compiled,
                         __C_Slash__double_double_double, Return, "double",
                         Args, "double", "double", End),

            new Function(c, "%", DoubleType::mod, Op, Compiled,
                         __C_PCent__double_double_double, Return, "double",
                         Args, "double", "double", End),

            new Function(c, "double", DoubleType::int2double, Cast, Compiled,
                         __C_double_double_int, Return, "double", Args, "int",
                         End),

            new Function(c, "double", DoubleType::float2double, Cast, Compiled,
                         __C_double_double_float, Return, "double", Args,
                         "float", End),

            new Function(c, "double", DoubleType::int642double, Cast, Compiled,
                         __C_double_double_int64, Return, "double", Args,
                         "int64", End),

            new Function(c, "=", DoubleType::assign, AsOp, Compiled,
                         __C_EQ__doubleAmp__doubleAmp__double, Return,
                         "double&", Args, "double&", "double", End),

            new Function(c, "+=", DoubleType::assignPlus, AsOp, Compiled,
                         __C_Plus_EQ__doubleAmp__doubleAmp__double, Return,
                         "double&", Args, "double&", "double", End),

            new Function(c, "-=", DoubleType::assignSub, AsOp, Compiled,
                         __C_Minus_EQ__doubleAmp__doubleAmp__double, Return,
                         "double&", Args, "double&", "double", End),

            new Function(c, "*=", DoubleType::assignMult, AsOp, Compiled,
                         __C_Star_EQ__doubleAmp__doubleAmp__double, Return,
                         "double&", Args, "double&", "double", End),

            new Function(c, "/=", DoubleType::assignDiv, AsOp, Compiled,
                         __C_Slash_EQ__doubleAmp__doubleAmp__double, Return,
                         "double&", Args, "double&", "double", End),

            new Function(c, "%=", DoubleType::assignMod, AsOp, Compiled,
                         __C_PCent_EQ__doubleAmp__doubleAmp__double, Return,
                         "double&", Args, "double&", "double", End),

            new Function(c, "?:", DoubleType::conditionalExpr, Op, Compiled,
                         __C_QMark_Colon__bool_double_double, Return, "double",
                         Args, "bool", "double", "double", End),

            new Function(c, "print", DoubleType::print, None, Compiled,
                         __C_print_void_double, Return, "void", Args, "double",
                         End),

            new Function(c, "==", DoubleType::equals, CommOp, Compiled,
                         __C_EQ_EQ__bool_double_double, Return, "bool", Args,
                         "double", "double", End),

            new Function(c, "!=", DoubleType::notEquals, CommOp, Compiled,
                         __C_Bang_EQ__bool_double_double, Return, "bool", Args,
                         "double", "double", End),

            new Function(c, ">=", DoubleType::greaterThanEq, Op, Compiled,
                         __C_GT_EQ__bool_double_double, Return, "bool", Args,
                         "double", "double", End),

            new Function(c, "<=", DoubleType::lessThanEq, Op, Compiled,
                         __C_LT_EQ__bool_double_double, Return, "bool", Args,
                         "double", "double", End),

            new Function(c, "<", DoubleType::lessThan, Op, Compiled,
                         __C_LT__bool_double_double, Return, "bool", Args,
                         "double", "double", End),

            new Function(c, ">", DoubleType::greaterThan, Op, Compiled,
                         __C_GT__bool_double_double, Return, "bool", Args,
                         "double", "double", End),

            new Function(c, "pre++", DoubleType::preInc, Op, Compiled,
                         __C_prePlus_Plus__double_doubleAmp_, Return, "double",
                         Args, "double&", End),

            new Function(c, "post++", DoubleType::postInc, Op, Compiled,
                         __C_postPlus_Plus__double_doubleAmp_, Return, "double",
                         Return, "double", Args, "double&", End),

            new Function(c, "pre--", DoubleType::preDec, Op, Compiled,
                         __C_preMinus_Minus__double_doubleAmp_, Return,
                         "double", Args, "double&", End),

            new Function(c, "post--", DoubleType::postDec, Op, Compiled,
                         __C_postMinus_Minus__double_doubleAmp_, Return,
                         "double", Args, "double&", End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(DoubleType::dereference, double)
    {
        using namespace Mu;
        double* fp = reinterpret_cast<double*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*fp);
    }

    NODE_IMPLEMENTATION(DoubleType::int2double, double)
    {
        NODE_RETURN(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(DoubleType::float2double, double)
    {
        NODE_RETURN(NODE_ARG(0, float));
    }

    NODE_IMPLEMENTATION(DoubleType::int642double, double)
    {
        NODE_RETURN(NODE_ARG(0, int64));
    }

    NODE_IMPLEMENTATION(DoubleType::defaultDouble, double)
    {
        NODE_RETURN(0.0f);
    }

    NODE_IMPLEMENTATION(DoubleType::print, void)
    {
        double f = NODE_ARG(0, double);
        cout << "PRINT: " << f << endl << flush;
    }

#define OP(name, op)                                             \
    NODE_IMPLEMENTATION(name, double)                            \
    {                                                            \
        NODE_RETURN(NODE_ARG(0, double) op NODE_ARG(1, double)); \
    }

    OP(DoubleType::add, +)
    OP(DoubleType::sub, -)
    OP(DoubleType::mult, *)
    OP(DoubleType::div, /)
#undef OP

#define RELOP(name, op)                                          \
    NODE_IMPLEMENTATION(name, bool)                              \
    {                                                            \
        NODE_RETURN(NODE_ARG(0, double) op NODE_ARG(1, double)); \
    }

    RELOP(DoubleType::equals, ==)
    RELOP(DoubleType::notEquals, !=)
    RELOP(DoubleType::lessThan, <)
    RELOP(DoubleType::greaterThan, >)
    RELOP(DoubleType::lessThanEq, <=)
    RELOP(DoubleType::greaterThanEq, >=)
#undef RELOP

    NODE_IMPLEMENTATION(DoubleType::mod, double)
    {
        NODE_RETURN(::fmod(NODE_ARG(0, double), NODE_ARG(1, double)));
    }

    NODE_IMPLEMENTATION(DoubleType::negate, double)
    {
        NODE_RETURN(-NODE_ARG(0, double));
    }

    NODE_IMPLEMENTATION(DoubleType::conditionalExpr, double)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, double)
                                      : NODE_ARG(2, double));
    }

    NODE_IMPLEMENTATION(DoubleType::preInc, double)
    {
        using namespace Mu;
        double* fp = reinterpret_cast<double*>(NODE_ARG(0, Pointer));
        return ++(*fp);
    }

    NODE_IMPLEMENTATION(DoubleType::postInc, double)
    {
        using namespace Mu;
        double* fp = reinterpret_cast<double*>(NODE_ARG(0, Pointer));
        return (*fp)++;
    }

    NODE_IMPLEMENTATION(DoubleType::preDec, double)
    {
        using namespace Mu;
        double* fp = reinterpret_cast<double*>(NODE_ARG(0, Pointer));
        return --(*fp);
    }

    NODE_IMPLEMENTATION(DoubleType::postDec, double)
    {
        using namespace Mu;
        double* fp = reinterpret_cast<double*>(NODE_ARG(0, Pointer));
        return (*fp)--;
    }

#define ASOP(name, op)                                                \
    NODE_IMPLEMENTATION(name, Pointer)                                \
    {                                                                 \
        using namespace Mu;                                           \
        double* fp = reinterpret_cast<double*>(NODE_ARG(0, Pointer)); \
        double f = NODE_ARG(1, double);                               \
        (*fp) op f;                                                   \
        NODE_RETURN((Pointer)fp);                                     \
    }

    ASOP(DoubleType::assign, =)
    ASOP(DoubleType::assignPlus, +=)
    ASOP(DoubleType::assignSub, -=)
    ASOP(DoubleType::assignMult, *=)
    ASOP(DoubleType::assignDiv, /=)
#undef ASOP

    NODE_IMPLEMENTATION(DoubleType::assignMod, Pointer)
    {
        using namespace Mu;
        double* fp = reinterpret_cast<double*>(NODE_ARG(0, Pointer));
        (*fp) = ::fmod(*fp, NODE_ARG(1, double));
        NODE_RETURN((Pointer)fp);
    }

} // namespace Mu
