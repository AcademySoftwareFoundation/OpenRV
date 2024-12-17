//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/Int64Type.h>
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

    Int64Type::Int64Type(Context* c)
        : PrimitiveType(c, "int64", Int64Rep::rep())
    {
    }

    Int64Type::~Int64Type() {}

    PrimitiveObject* Int64Type::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value Int64Type::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._int64Func)(*n, thread));
    }

    void Int64Type::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        int64* ip = reinterpret_cast<int64*>(p);
        *ip = (*n->func()._int64Func)(*n, thread);
    }

    void Int64Type::outputValue(ostream& o, const Value& value, bool full) const
    {
        o << value._int64;
    }

    void Int64Type::outputValueRecursive(ostream& o, const ValuePointer value,
                                         ValueOutputState& state) const
    {
        o << *reinterpret_cast<const int64*>(value);
    }

    //
    //  These are the non-inlined versions of the functions
    //  in Native.h. They are passed in as the Compiled function
    //  for the ints.
    //

#define T Thread&

    int64 __C_int64_int64(T) { return 0; }

    int64 __C_int64_int64_int64Amp_(T, int64& a) { return a; }

    int64 __C_Minus__int64_int64(T, int64 a) { return -a; }

    int64 __C_Tilde__int64_int64(T, int64 a) { return ~a; }

    int64 __C_Plus__int64_int64_int64(T, int64 a, int64 b) { return a + b; }

    int64 __C_Minus__int64_int64_int64(T, int64 a, int64 b) { return a - b; }

    int64 __C_Star__int64_int64_int64(T, int64 a, int64 b) { return a * b; }

    int64 __C_Slash__int64_int64_int64(T, int64 a, int64 b) { return a / b; }

    int64 __C_PCent__int64_int64_int64(T, int64 a, int64 b) { return a % b; }

    int64 __C_Caret__int64_int64_int64(T, int64 a, int64 b) { return a ^ b; }

    int64 __C_Amp__int64_int64_int64(T, int64 a, int64 b) { return a & b; }

    int64 __C_Pipe__int64_int64_int64(T, int64 a, int64 b) { return a | b; }

    int64 __C_GT_GT___int64_int64_int64(T, int64 a, int64 b) { return a >> b; }

    int64 __C_LT_LT___int64_int64_int64(T, int64 a, int64 b) { return a << b; }

    bool __C_GT__bool_int64_int64(T, int64 a, int64 b) { return a > b; }

    bool __C_LT__bool_int64_int64(T, int64 a, int64 b) { return a < b; }

    bool __C_GT_EQ__bool_int64_int64(T, int64 a, int64 b) { return a >= b; }

    bool __C_LT_EQ__bool_int64_int64(T, int64 a, int64 b) { return a <= b; }

    bool __C_EQ_EQ__bool_int64_int64(T, int64 a, int64 b) { return a == b; }

    bool __C_Bang_EQ__bool_int64_int64(T, int64 a, int64 b) { return a != b; }

    int64& __C_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a = b;
    }

    int64& __C_Plus_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a += b;
    }

    int64& __C_Minus_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a -= b;
    }

    int64& __C_Star_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a *= b;
    }

    int64& __C_Slash_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a /= b;
    }

    int64& __C_PCent_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a %= b;
    }

    int64& __C_Caret_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a ^= b;
    }

    int64& __C_GT_GT_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a >>= b;
    }

    int64& __C_LT_LT_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a <<= b;
    }

    int64& __C_Pipe_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a |= b;
    }

    int64& __C_Amp_EQ__int64Amp__int64Amp__int64(T, int64& a, int64 b)
    {
        return a &= b;
    }

    int64 __C_prePlus_Plus__int64_int64Amp_(T, int64& a) { return ++a; }

    int64 __C_postPlus_Plus__int64_int64Amp_(T, int64& a) { return a++; }

    int64 __C_preMinus_Minus__int64_int64Amp_(T, int64& a) { return --a; }

    int64 __C_postMinus_Minus__int64_int64Amp_(T, int64& a) { return a--; }

    int64 __C_int64_int64_float(T, float a) { return int64(a); }

    int __C_int_int_int64(T, int64 a) { return int(a); }

    int64 __C_int64_int64_int(T, int a) { return int64(a); }

    //
    //  This function is delt with directly by muc, it translates
    //  to an if statement. But we still need a compiled version for
    //  lambda expressions.
    //

    int64 __C_QMark_Colon__bool_int64_int64(T, bool p, int64 a, int64 b)
    {
        return p ? a : b;
    }

#undef T

    void Int64Type::load()
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

        s->addSymbols(
            new ReferenceType(c, "int64&", this),

            new Function(c, "int64", Int64Type::defaultInt64, Mapped, Compiled,
                         __C_int64_int64, Return, "int64", End),

            new Function(c, "int64", Int64Type::dereference, Cast, Compiled,
                         __C_int64_int64_int64Amp_, Return, "int64", Args,
                         "int64&", End),

            new Function(c, "int", Int64Type::int642int, Cast | Lossy, Compiled,
                         __C_int_int_int64, Return, "int", Args, "int64", End),

            new Function(c, "int64", Int64Type::int2int64, Cast, Compiled,
                         __C_int64_int64_int, Return, "int64", Args, "int",
                         End),

            new Function(c, "+", Int64Type::add, CommOp, Compiled,
                         __C_Plus__int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "-", Int64Type::sub, Op, Compiled,
                         __C_Minus__int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "-", Int64Type::negate, Op, Compiled,
                         __C_Minus__int64_int64, Return, "int64", Args, "int64",
                         End),

            new Function(c, "*", Int64Type::mult, CommOp, Compiled,
                         __C_Star__int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "/", Int64Type::div, Op, Compiled,
                         __C_Slash__int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "%", Int64Type::mod, Op, Compiled,
                         __C_PCent__int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "int64", Int64Type::float2int64, Lossy, Compiled,
                         __C_int64_int64_float, Return, "int64", Args, "float",
                         End),

            new Function(c, "=", Int64Type::assign, AsOp, Compiled,
                         __C_EQ__int64Amp__int64Amp__int64, Return, "int64&",
                         Args, "int64&", "int64", End),

            new Function(c, "+=", Int64Type::assignPlus, AsOp, Compiled,
                         __C_Plus_EQ__int64Amp__int64Amp__int64, Return,
                         "int64&", Args, "int64&", "int64", End),

            new Function(c, "-=", Int64Type::assignSub, AsOp, Compiled,
                         __C_Minus_EQ__int64Amp__int64Amp__int64, Return,
                         "int64&", Args, "int64&", "int64", End),

            new Function(c, "*=", Int64Type::assignMult, AsOp, Compiled,
                         __C_Star_EQ__int64Amp__int64Amp__int64, Return,
                         "int64&", Args, "int64&", "int64", End),

            new Function(c, "/=", Int64Type::assignDiv, AsOp, Compiled,
                         __C_Slash_EQ__int64Amp__int64Amp__int64, Return,
                         "int64&", Args, "int64&", "int64", End),

            new Function(c, "%=", Int64Type::assignMod, AsOp, Compiled,
                         __C_PCent_EQ__int64Amp__int64Amp__int64, Return,
                         "int64&", Args, "int64&", "int64", End),

            new Function(c, "?:", Int64Type::conditionalExpr,
                         Op ^ NativeInlined, // not inlined
                         Compiled, __C_QMark_Colon__bool_int64_int64, Return,
                         "int64", Args, "bool", "int64", "int64", End),

            new Function(c, "==", Int64Type::equals, CommOp, Compiled,
                         __C_EQ_EQ__bool_int64_int64, Return, "bool", Args,
                         "int64", "int64", End),

            new Function(c, "!=", Int64Type::notEquals, CommOp, Compiled,
                         __C_Bang_EQ__bool_int64_int64, Return, "bool", Args,
                         "int64", "int64", End),

            new Function(c, ">=", Int64Type::greaterThanEq, Op, Compiled,
                         __C_GT_EQ__bool_int64_int64, Return, "bool", Args,
                         "int64", "int64", End),

            new Function(c, "<=", Int64Type::lessThanEq, Op, Compiled,
                         __C_LT_EQ__bool_int64_int64, Return, "bool", Args,
                         "int64", "int64", End),

            new Function(c, "<", Int64Type::lessThan, Op, Compiled,
                         __C_LT__bool_int64_int64, Return, "bool", Args,
                         "int64", "int64", End),

            new Function(c, ">", Int64Type::greaterThan, Op, Compiled,
                         __C_GT__bool_int64_int64, Return, "bool", Args,
                         "int64", "int64", End),

            new Function(c, "|", Int64Type::bitOr, CommOp, Compiled,
                         __C_Pipe__int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "&", Int64Type::bitAnd, CommOp, Compiled,
                         __C_Amp__int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "^", Int64Type::bitXor, CommOp, Compiled,
                         __C_Caret__int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "~", Int64Type::bitNot, Op, Compiled,
                         __C_Tilde__int64_int64, Return, "int64", Args, "int64",
                         End),

            new Function(c, "<<", Int64Type::shiftLeft, Op, Compiled,
                         __C_LT_LT___int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, ">>", Int64Type::shiftRight, Op, Compiled,
                         __C_GT_GT___int64_int64_int64, Return, "int64", Args,
                         "int64", "int64", End),

            new Function(c, "pre++", Int64Type::preInc, Op, Compiled,
                         __C_prePlus_Plus__int64_int64Amp_, Return, "int64",
                         Args, "int64&", End),

            new Function(c, "post++", Int64Type::postInc, Op, Compiled,
                         __C_postPlus_Plus__int64_int64Amp_, Return, "int64",
                         Args, "int64&", End),

            new Function(c, "pre--", Int64Type::preDec, Op, Compiled,
                         __C_preMinus_Minus__int64_int64Amp_, Return, "int64",
                         Args, "int64&", End),

            new Function(c, "post--", Int64Type::postDec, Op, Compiled,
                         __C_postMinus_Minus__int64_int64Amp_, Return, "int64",
                         Args, "int64&", End),

            EndArguments);

        this->addSymbols(
            new SymbolicConstant(c, "max", "int64",
                                 Value(numeric_limits<int64>::max())),

            new SymbolicConstant(c, "min", "int64",
                                 Value(numeric_limits<int64>::min())),
            EndArguments);
    }

    NODE_IMPLEMENTATION(Int64Type::dereference, int64)
    {
        using namespace Mu;
        int64* ip = reinterpret_cast<int64*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*ip);
    }

    NODE_IMPLEMENTATION(Int64Type::defaultInt64, int64) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(Int64Type::int642int, int)
    {
        NODE_RETURN(int(NODE_ARG(0, int64)));
    }

    NODE_IMPLEMENTATION(Int64Type::int2int64, int64)
    {
        NODE_RETURN(int64(NODE_ARG(0, int)));
    }

    NODE_IMPLEMENTATION(Int64Type::float2int64, int64)
    {
        NODE_RETURN(int64(NODE_ARG(0, float)));
    }

    NODE_IMPLEMENTATION(Int64Type::bitNot, int64)
    {
        NODE_RETURN(int64(~(NODE_ARG(0, int64))));
    }

#define OP(name, op)                                           \
    NODE_IMPLEMENTATION(name, int64)                           \
    {                                                          \
        NODE_RETURN(NODE_ARG(0, int64) op NODE_ARG(1, int64)); \
    }

    OP(Int64Type::add, +)
    OP(Int64Type::sub, -)
    OP(Int64Type::mult, *)
    OP(Int64Type::div, /)
    OP(Int64Type::mod, %)
    OP(Int64Type::bitAnd, &)
    OP(Int64Type::bitOr, |)
    OP(Int64Type::bitXor, ^)
    OP(Int64Type::shiftLeft, <<)
    OP(Int64Type::shiftRight, >>)
#undef OP

#define RELOP(name, op)                                        \
    NODE_IMPLEMENTATION(name, bool)                            \
    {                                                          \
        NODE_RETURN(NODE_ARG(0, int64) op NODE_ARG(1, int64)); \
    }

    RELOP(Int64Type::equals, ==)
    RELOP(Int64Type::notEquals, !=)
    RELOP(Int64Type::lessThan, <)
    RELOP(Int64Type::greaterThan, >)
    RELOP(Int64Type::lessThanEq, <=)
    RELOP(Int64Type::greaterThanEq, >=)
#undef RELOP

    NODE_IMPLEMENTATION(Int64Type::conditionalExpr, int64)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, int64)
                                      : NODE_ARG(2, int64));
    }

    NODE_IMPLEMENTATION(Int64Type::negate, int64)
    {
        NODE_RETURN(-NODE_ARG(0, int64));
    }

    NODE_IMPLEMENTATION(Int64Type::preInc, int64)
    {
        using namespace Mu;
        int64* ip = reinterpret_cast<int64*>(NODE_ARG(0, Pointer));
        return ++(*ip);
    }

    NODE_IMPLEMENTATION(Int64Type::postInc, int64)
    {
        using namespace Mu;
        int64* ip = reinterpret_cast<int64*>(NODE_ARG(0, Pointer));
        return (*ip)++;
    }

    NODE_IMPLEMENTATION(Int64Type::preDec, int64)
    {
        using namespace Mu;
        int64* ip = reinterpret_cast<int64*>(NODE_ARG(0, Pointer));
        return --(*ip);
    }

    NODE_IMPLEMENTATION(Int64Type::postDec, int64)
    {
        using namespace Mu;
        int64* ip = reinterpret_cast<int64*>(NODE_ARG(0, Pointer));
        return (*ip)--;
    }

#define ASOP(name, op)                                              \
    NODE_IMPLEMENTATION(name, Pointer)                              \
    {                                                               \
        using namespace Mu;                                         \
        int64* ip = reinterpret_cast<int64*>(NODE_ARG(0, Pointer)); \
        *ip op NODE_ARG(1, int64);                                  \
        NODE_RETURN((Pointer)ip);                                   \
    }

    ASOP(Int64Type::assign, =)
    ASOP(Int64Type::assignPlus, +=)
    ASOP(Int64Type::assignSub, -=)
    ASOP(Int64Type::assignMult, *=)
    ASOP(Int64Type::assignDiv, /=)
    ASOP(Int64Type::assignMod, %=)
#undef ASOP

} // namespace Mu
