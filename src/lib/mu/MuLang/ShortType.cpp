//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/ShortType.h>
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Value.h>
#include <iostream>
#include <limits>

namespace Mu
{
    using namespace std;

    ShortType::ShortType(Context* c)
        : PrimitiveType(c, "short", ShortRep::rep())
    {
    }

    ShortType::~ShortType() {}

    PrimitiveObject* ShortType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value ShortType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._shortFunc)(*n, thread));
    }

    void ShortType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        short* ip = reinterpret_cast<short*>(p);
        *ip = (*n->func()._shortFunc)(*n, thread);
    }

    void ShortType::outputValue(ostream& o, const Value& value, bool full) const
    {
        o << value._short;
    }

    void ShortType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                         ValueOutputState&) const
    {
        o << *reinterpret_cast<const short*>(vp);
    }

    //
    //  These are the non-inlined versions of the functions
    //  in Native.h. They are passed in as the Compiled function
    //  for the ints.
    //

#define T Thread&

    short __C_short_short(T) { return 0; }

    short __C_short_short_shortAmp_(T, short& a) { return a; }

    short __C_Minus__short_short(T, short a) { return -a; }

    short __C_Tilde__short_short(T, short a) { return ~a; }

    short __C_Plus__short_short_short(T, short a, short b) { return a + b; }

    short __C_Minus__short_short_short(T, short a, short b) { return a - b; }

    short __C_Star__short_short_short(T, short a, short b) { return a * b; }

    short __C_Slash__short_short_short(T, short a, short b) { return a / b; }

    short __C_PCent__short_short_short(T, short a, short b) { return a % b; }

    short __C_Caret__short_short_short(T, short a, short b) { return a ^ b; }

    short __C_Amp__short_short_short(T, short a, short b) { return a & b; }

    short __C_Pipe__short_short_short(T, short a, short b) { return a | b; }

    short __C_GT_GT___short_short_short(T, short a, short b) { return a >> b; }

    short __C_LT_LT___short_short_short(T, short a, short b) { return a << b; }

    bool __C_GT__bool_short_short(T, short a, short b) { return a > b; }

    bool __C_LT__bool_short_short(T, short a, short b) { return a < b; }

    bool __C_GT_EQ__bool_short_short(T, short a, short b) { return a >= b; }

    bool __C_LT_EQ__bool_short_short(T, short a, short b) { return a <= b; }

    bool __C_EQ_EQ__bool_short_short(T, short a, short b) { return a == b; }

    bool __C_Bang_EQ__bool_short_short(T, short a, short b) { return a != b; }

    short& __C_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a = b;
    }

    short& __C_Plus_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a += b;
    }

    short& __C_Minus_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a -= b;
    }

    short& __C_Star_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a *= b;
    }

    short& __C_Slash_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a /= b;
    }

    short& __C_PCent_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a %= b;
    }

    short& __C_Caret_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a ^= b;
    }

    short& __C_GT_GT_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a >>= b;
    }

    short& __C_LT_LT_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a <<= b;
    }

    short& __C_Pipe_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a |= b;
    }

    short& __C_Amp_EQ__shortAmp__shortAmp__short(T, short& a, short b)
    {
        return a &= b;
    }

    short __C_prePlus_Plus__short_shortAmp_(T, short& a) { return ++a; }

    short __C_postPlus_Plus__short_shortAmp_(T, short& a) { return a++; }

    short __C_preMinus_Minus__short_shortAmp_(T, short& a) { return --a; }

    short __C_postMinus_Minus__short_shortAmp_(T, short& a) { return a--; }

    short __C_short_short_float(T, float a) { return short(a); }

    short __C_short_short_int(T, int a) { return short(a); }

    int __C_int_int_short(T, short a) { return int(a); }

    //
    //  This function is delt with directly by muc, it translates
    //  to an if statement. But we still need a compiled version for
    //  lambda expressions.
    //

    short __C_QMark_Colon__bool_short_short(T, bool p, short a, short b)
    {
        return p ? a : b;
    }

#undef T

    void ShortType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        //
        //  All of the functions are inlined in Native.h So here the
        //  NativeInlined attribute is set in for all function attributes.
        //

        Mapped |= NativeInlined;
        CommOp |= NativeInlined;
        Op |= NativeInlined;
        AsOp |= NativeInlined;
        Lossy |= NativeInlined;
        Cast |= NativeInlined;

        s->addSymbols(
            new ReferenceType(c, "short&", this),

            new Function(c, "short", ShortType::defaultShort, Mapped, Compiled,
                         __C_short_short, Return, "short", End),

            new Function(c, "short", ShortType::int2short, Lossy, Compiled,
                         __C_short_short_int, Return, "short", Args, "int",
                         End),

            new Function(c, "short", ShortType::dereference, Cast, Compiled,
                         __C_short_short_shortAmp_, Return, "short", Args,
                         "short&", End),

            new Function(c, "int", ShortType::fromShort, Cast, Compiled,
                         __C_int_int_short, Return, "int", Args, "short", End),

            new Function(c, "+", ShortType::add, CommOp, Compiled,
                         __C_Plus__short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "-", ShortType::sub, Op, Compiled,
                         __C_Minus__short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "-", ShortType::negate, Op, Compiled,
                         __C_Minus__short_short, Return, "short", Args, "short",
                         End),

            new Function(c, "*", ShortType::mult, CommOp, Compiled,
                         __C_Star__short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "/", ShortType::div, Op, Compiled,
                         __C_Slash__short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "%", ShortType::mod, Op, Compiled,
                         __C_PCent__short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "short", ShortType::float2short, Lossy, Compiled,
                         __C_short_short_float, Return, "short", Args, "float",
                         End),

            new Function(c, "=", ShortType::assign, AsOp, Compiled,
                         __C_EQ__shortAmp__shortAmp__short, Return, "short&",
                         Args, "short&", "short", End),

            new Function(c, "+=", ShortType::assignPlus, AsOp, Compiled,
                         __C_Plus_EQ__shortAmp__shortAmp__short, Return,
                         "short&", Args, "short&", "short", End),

            new Function(c, "-=", ShortType::assignSub, AsOp, Compiled,
                         __C_Minus_EQ__shortAmp__shortAmp__short, Return,
                         "short&", Args, "short&", "short", End),

            new Function(c, "*=", ShortType::assignMult, AsOp, Compiled,
                         __C_Star_EQ__shortAmp__shortAmp__short, Return,
                         "short&", Args, "short&", "short", End),

            new Function(c, "/=", ShortType::assignDiv, AsOp, Compiled,
                         __C_Slash_EQ__shortAmp__shortAmp__short, Return,
                         "short&", Args, "short&", "short", End),

            new Function(c, "%=", ShortType::assignMod, AsOp, Compiled,
                         __C_PCent_EQ__shortAmp__shortAmp__short, Return,
                         "short&", Args, "short&", "short", End),

            new Function(c, "?:", ShortType::conditionalExpr,
                         Op ^ NativeInlined, // not inlined
                         Compiled, __C_QMark_Colon__bool_short_short, Return,
                         "short", Args, "bool", "short", "short", End),

            new Function(c, "==", ShortType::equals, CommOp, Compiled,
                         __C_EQ_EQ__bool_short_short, Return, "bool", Args,
                         "short", "short", End),

            new Function(c, "!=", ShortType::notEquals, CommOp, Compiled,
                         __C_Bang_EQ__bool_short_short, Return, "bool", Args,
                         "short", "short", End),

            new Function(c, ">=", ShortType::greaterThanEq, Op, Compiled,
                         __C_GT_EQ__bool_short_short, Return, "bool", Args,
                         "short", "short", End),

            new Function(c, "<=", ShortType::lessThanEq, Op, Compiled,
                         __C_LT_EQ__bool_short_short, Return, "bool", Args,
                         "short", "short", End),

            new Function(c, "<", ShortType::lessThan, Op, Compiled,
                         __C_LT__bool_short_short, Return, "bool", Args,
                         "short", "short", End),

            new Function(c, ">", ShortType::greaterThan, Op, Compiled,
                         __C_GT__bool_short_short, Return, "bool", Args,
                         "short", "short", End),

            new Function(c, "|", ShortType::bitOr, CommOp, Compiled,
                         __C_Pipe__short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "&", ShortType::bitAnd, CommOp, Compiled,
                         __C_Amp__short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "^", ShortType::bitXor, CommOp, Compiled,
                         __C_Caret__short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "~", ShortType::bitNot, Op, Compiled,
                         __C_Tilde__short_short, Return, "short", Args, "short",
                         End),

            new Function(c, "<<", ShortType::shiftLeft, Op, Compiled,
                         __C_LT_LT___short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, ">>", ShortType::shiftRight, Op, Compiled,
                         __C_GT_GT___short_short_short, Return, "short", Args,
                         "short", "short", End),

            new Function(c, "pre++", ShortType::preInc, Op, Compiled,
                         __C_prePlus_Plus__short_shortAmp_, Return, "short",
                         Args, "short&", End),

            new Function(c, "post++", ShortType::postInc, Op, Compiled,
                         __C_postPlus_Plus__short_shortAmp_, Return, "short",
                         Args, "short&", End),

            new Function(c, "pre--", ShortType::preDec, Op, Compiled,
                         __C_preMinus_Minus__short_shortAmp_, Return, "short",
                         Args, "short&", End),

            new Function(c, "post--", ShortType::postDec, Op, Compiled,
                         __C_postMinus_Minus__short_shortAmp_, Return, "short",
                         Args, "short&", End),

            EndArguments);

        this->addSymbols(
            new SymbolicConstant(c, "max", "short",
                                 Value(numeric_limits<short>::max())),

            new SymbolicConstant(c, "min", "short",
                                 Value(numeric_limits<short>::min())),
            EndArguments);
    }

    NODE_IMPLEMENTATION(ShortType::dereference, short)
    {
        using namespace Mu;
        short* ip = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*ip);
    }

    NODE_IMPLEMENTATION(ShortType::defaultShort, short) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(ShortType::fromShort, int)
    {
        NODE_RETURN(NODE_ARG(0, short));
    }

    NODE_IMPLEMENTATION(ShortType::int2short, short)
    {
        short s = NODE_ARG(0, int);
        NODE_RETURN(s);
    }

    NODE_IMPLEMENTATION(ShortType::float2short, short)
    {
        NODE_RETURN(short(NODE_ARG(0, float)));
    }

    NODE_IMPLEMENTATION(ShortType::bitNot, short)
    {
        NODE_RETURN(short(~(NODE_ARG(0, short))));
    }

#define OP(name, op)                                           \
    NODE_IMPLEMENTATION(name, short)                           \
    {                                                          \
        NODE_RETURN(NODE_ARG(0, short) op NODE_ARG(1, short)); \
    }

    OP(ShortType::add, +)
    OP(ShortType::sub, -)
    OP(ShortType::mult, *)
    OP(ShortType::div, /)
    OP(ShortType::mod, %)
    OP(ShortType::bitAnd, &)
    OP(ShortType::bitOr, |)
    OP(ShortType::bitXor, ^)
    OP(ShortType::shiftLeft, <<)
    OP(ShortType::shiftRight, >>)
#undef OP

#define RELOP(name, op)                                        \
    NODE_IMPLEMENTATION(name, bool)                            \
    {                                                          \
        NODE_RETURN(NODE_ARG(0, short) op NODE_ARG(1, short)); \
    }

    RELOP(ShortType::equals, ==)
    RELOP(ShortType::notEquals, !=)
    RELOP(ShortType::lessThan, <)
    RELOP(ShortType::greaterThan, >)
    RELOP(ShortType::lessThanEq, <=)
    RELOP(ShortType::greaterThanEq, >=)
#undef RELOP

    NODE_IMPLEMENTATION(ShortType::conditionalExpr, short)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, short)
                                      : NODE_ARG(2, short));
    }

    NODE_IMPLEMENTATION(ShortType::negate, short)
    {
        NODE_RETURN(-NODE_ARG(0, short));
    }

    NODE_IMPLEMENTATION(ShortType::preInc, short)
    {
        using namespace Mu;
        short* ip = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        return ++(*ip);
    }

    NODE_IMPLEMENTATION(ShortType::postInc, short)
    {
        using namespace Mu;
        short* ip = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        return (*ip)++;
    }

    NODE_IMPLEMENTATION(ShortType::preDec, short)
    {
        using namespace Mu;
        short* ip = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        return --(*ip);
    }

    NODE_IMPLEMENTATION(ShortType::postDec, short)
    {
        using namespace Mu;
        short* ip = reinterpret_cast<short*>(NODE_ARG(0, Pointer));
        return (*ip)--;
    }

#define ASOP(name, op)                                              \
    NODE_IMPLEMENTATION(name, Pointer)                              \
    {                                                               \
        using namespace Mu;                                         \
        short* ip = reinterpret_cast<short*>(NODE_ARG(0, Pointer)); \
        *ip op NODE_ARG(1, short);                                  \
        NODE_RETURN(ip);                                            \
    }

    ASOP(ShortType::assign, =)
    ASOP(ShortType::assignPlus, +=)
    ASOP(ShortType::assignSub, -=)
    ASOP(ShortType::assignMult, *=)
    ASOP(ShortType::assignDiv, /=)
    ASOP(ShortType::assignMod, %=)
#undef ASOP

} // namespace Mu
