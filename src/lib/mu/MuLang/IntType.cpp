//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/IntType.h>
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

    IntType::IntType(Context* c)
        : PrimitiveType(c, "int", IntRep::rep())
    {
    }

    IntType::~IntType() {}

    PrimitiveObject* IntType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value IntType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._intFunc)(*n, thread));
    }

    void IntType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        int* ip = reinterpret_cast<int*>(p);
        *ip = (*n->func()._intFunc)(*n, thread);
    }

    void IntType::outputValue(ostream& o, const Value& value, bool full) const
    {
        o << value._int;
    }

    void IntType::outputValueRecursive(ostream& o, const ValuePointer p,
                                       ValueOutputState& state) const
    {
        o << *reinterpret_cast<const int*>(p);
    }

    //
    //  These are the non-inlined versions of the functions
    //  in Native.h. They are passed in as the Compiled function
    //  for the ints.
    //

#define T Thread&

    int __C_int_int(T) { return 0; }

    int __C_int_int_intAmp_(T, int& a) { return a; }

    int __C_Minus__int_int(T, int a) { return -a; }

    int __C_Tilde__int_int(T, int a) { return ~a; }

    int __C_Plus__int_int_int(T, int a, int b) { return a + b; }

    int __C_Minus__int_int_int(T, int a, int b) { return a - b; }

    int __C_Star__int_int_int(T, int a, int b) { return a * b; }

    int __C_Slash__int_int_int(T, int a, int b) { return a / b; }

    int __C_PCent__int_int_int(T, int a, int b) { return a % b; }

    int __C_Caret__int_int_int(T, int a, int b) { return a ^ b; }

    int __C_Amp__int_int_int(T, int a, int b) { return a & b; }

    int __C_Pipe__int_int_int(T, int a, int b) { return a | b; }

    int __C_GT_GT___int_int_int(T, int a, int b) { return a >> b; }

    int __C_LT_LT___int_int_int(T, int a, int b) { return a << b; }

    bool __C_GT__bool_int_int(T, int a, int b) { return a > b; }

    bool __C_LT__bool_int_int(T, int a, int b) { return a < b; }

    bool __C_GT_EQ__bool_int_int(T, int a, int b) { return a >= b; }

    bool __C_LT_EQ__bool_int_int(T, int a, int b) { return a <= b; }

    bool __C_EQ_EQ__bool_int_int(T, int a, int b) { return a == b; }

    bool __C_Bang_EQ__bool_int_int(T, int a, int b) { return a != b; }

    int& __C_EQ__intAmp__intAmp__int(T, int& a, int b) { return a = b; }

    int& __C_Plus_EQ__intAmp__intAmp__int(T, int& a, int b) { return a += b; }

    int& __C_Minus_EQ__intAmp__intAmp__int(T, int& a, int b) { return a -= b; }

    int& __C_Star_EQ__intAmp__intAmp__int(T, int& a, int b) { return a *= b; }

    int& __C_Slash_EQ__intAmp__intAmp__int(T, int& a, int b) { return a /= b; }

    int& __C_PCent_EQ__intAmp__intAmp__int(T, int& a, int b) { return a %= b; }

    int& __C_Caret_EQ__intAmp__intAmp__int(T, int& a, int b) { return a ^= b; }

    int& __C_GT_GT_EQ__intAmp__intAmp__int(T, int& a, int b) { return a >>= b; }

    int& __C_LT_LT_EQ__intAmp__intAmp__int(T, int& a, int b) { return a <<= b; }

    int& __C_Pipe_EQ__intAmp__intAmp__int(T, int& a, int b) { return a |= b; }

    int& __C_Amp_EQ__intAmp__intAmp__int(T, int& a, int b) { return a &= b; }

    int __C_prePlus_Plus__int_intAmp_(T, int& a) { return ++a; }

    int __C_postPlus_Plus__int_intAmp_(T, int& a) { return a++; }

    int __C_preMinus_Minus__int_intAmp_(T, int& a) { return --a; }

    int __C_postMinus_Minus__int_intAmp_(T, int& a) { return a--; }

    int __C_int_int_float(T, float a) { return int(a); }

    int __C_int_int_double(T, double a) { return int(a); }

    //
    //  This function is delt with directly by muc, it translates
    //  to an if statement. But we still need a compiled version for
    //  lambda expressions.
    //

    int __C_QMark_Colon__bool_int_int(T, bool p, int a, int b)
    {
        return p ? a : b;
    }

#undef T

    void IntType::load()
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
            new ReferenceType(c, "int&", this),

            new Function(c, "int", IntType::defaultInt, Mapped, Compiled,
                         __C_int_int, Return, "int", End),

            new Function(c, "int", IntType::dereference, Cast, Compiled,
                         __C_int_int_intAmp_, Return, "int", Args, "int&", End),

            new Function(c, "+", IntType::add, CommOp, Compiled,
                         __C_Plus__int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "-", IntType::sub, Op, Compiled,
                         __C_Minus__int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "-", IntType::negate, Op, Compiled,
                         __C_Minus__int_int, Return, "int", Args, "int", End),

            new Function(c, "*", IntType::mult, CommOp, Compiled,
                         __C_Star__int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "/", IntType::div, Op, Compiled,
                         __C_Slash__int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "%", IntType::mod, Op, Compiled,
                         __C_PCent__int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "int", IntType::float2int, Lossy, Compiled,
                         __C_int_int_float, Return, "int", Args, "float", End),

            new Function(c, "int", IntType::double2int, Lossy, Compiled,
                         __C_int_int_double, Return, "int", Args, "double",
                         End),

            new Function(c, "=", IntType::assign, AsOp, Compiled,
                         __C_EQ__intAmp__intAmp__int, Return, "int&", Args,
                         "int&", "int", End),

            new Function(c, "+=", IntType::assignPlus, AsOp, Compiled,
                         __C_Plus_EQ__intAmp__intAmp__int, Return, "int&", Args,
                         "int&", "int", End),

            new Function(c, "-=", IntType::assignSub, AsOp, Compiled,
                         __C_Minus_EQ__intAmp__intAmp__int, Return, "int&",
                         Args, "int&", "int", End),

            new Function(c, "*=", IntType::assignMult, AsOp, Compiled,
                         __C_Star_EQ__intAmp__intAmp__int, Return, "int&", Args,
                         "int&", "int", End),

            new Function(c, "/=", IntType::assignDiv, AsOp, Compiled,
                         __C_Slash_EQ__intAmp__intAmp__int, Return, "int&",
                         Args, "int&", "int", End),

            new Function(c, "%=", IntType::assignMod, AsOp, Compiled,
                         __C_PCent_EQ__intAmp__intAmp__int, Return, "int&",
                         Args, "int&", "int", End),

            new Function(c, "?:", IntType::conditionalExpr,
                         Op ^ NativeInlined, // not inlined
                         Compiled, __C_QMark_Colon__bool_int_int, Return, "int",
                         Args, "bool", "int", "int", End),

            new Function(c, "==", IntType::equals, CommOp, Compiled,
                         __C_EQ_EQ__bool_int_int, Return, "bool", Args, "int",
                         "int", End),

            new Function(c, "!=", IntType::notEquals, CommOp, Compiled,
                         __C_Bang_EQ__bool_int_int, Return, "bool", Args, "int",
                         "int", End),

            new Function(c, ">=", IntType::greaterThanEq, Op, Compiled,
                         __C_GT_EQ__bool_int_int, Return, "bool", Args, "int",
                         "int", End),

            new Function(c, "<=", IntType::lessThanEq, Op, Compiled,
                         __C_LT_EQ__bool_int_int, Return, "bool", Args, "int",
                         "int", End),

            new Function(c, "<", IntType::lessThan, Op, Compiled,
                         __C_LT__bool_int_int, Return, "bool", Args, "int",
                         "int", End),

            new Function(c, ">", IntType::greaterThan, Op, Compiled,
                         __C_GT__bool_int_int, Return, "bool", Args, "int",
                         "int", End),

            new Function(c, "|", IntType::bitOr, CommOp, Compiled,
                         __C_Pipe__int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "&", IntType::bitAnd, CommOp, Compiled,
                         __C_Amp__int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "^", IntType::bitXor, CommOp, Compiled,
                         __C_Caret__int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "~", IntType::bitNot, Op, Compiled,
                         __C_Tilde__int_int, Return, "int", Args, "int", End),

            new Function(c, "<<", IntType::shiftLeft, Op, Compiled,
                         __C_LT_LT___int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, ">>", IntType::shiftRight, Op, Compiled,
                         __C_GT_GT___int_int_int, Return, "int", Args, "int",
                         "int", End),

            new Function(c, "pre++", IntType::preInc, Op, Compiled,
                         __C_prePlus_Plus__int_intAmp_, Return, "int", Args,
                         "int&", End),

            new Function(c, "post++", IntType::postInc, Op, Compiled,
                         __C_postPlus_Plus__int_intAmp_, Return, "int", Args,
                         "int&", End),

            new Function(c, "pre--", IntType::preDec, Op, Compiled,
                         __C_preMinus_Minus__int_intAmp_, Return, "int", Args,
                         "int&", End),

            new Function(c, "post--", IntType::postDec, Op, Compiled,
                         __C_postMinus_Minus__int_intAmp_, Return, "int", Args,
                         "int&", End),

            EndArguments);

        this->addSymbols(
            new SymbolicConstant(c, "max", "int",
                                 Value(numeric_limits<int>::max())),

            new SymbolicConstant(c, "min", "int",
                                 Value(numeric_limits<int>::min())),
            EndArguments);
    }

    NODE_IMPLEMENTATION(IntType::dereference, int)
    {
        using namespace Mu;
        int* ip = reinterpret_cast<int*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*ip);
    }

    NODE_IMPLEMENTATION(IntType::defaultInt, int) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(IntType::float2int, int)
    {
        NODE_RETURN(int(NODE_ARG(0, float)));
    }

    NODE_IMPLEMENTATION(IntType::double2int, int)
    {
        NODE_RETURN(int(NODE_ARG(0, double)));
    }

    NODE_IMPLEMENTATION(IntType::bitNot, int)
    {
        NODE_RETURN(int(~(NODE_ARG(0, int))));
    }

#define OP(name, op)                                       \
    NODE_IMPLEMENTATION(name, int)                         \
    {                                                      \
        NODE_RETURN(NODE_ARG(0, int) op NODE_ARG(1, int)); \
    }

    OP(IntType::add, +)
    OP(IntType::sub, -)
    OP(IntType::mult, *)
    OP(IntType::div, /)
    OP(IntType::mod, %)
    OP(IntType::bitAnd, &)
    OP(IntType::bitOr, |)
    OP(IntType::bitXor, ^)
    OP(IntType::shiftLeft, <<)
    OP(IntType::shiftRight, >>)
#undef OP

#define RELOP(name, op)                                    \
    NODE_IMPLEMENTATION(name, bool)                        \
    {                                                      \
        NODE_RETURN(NODE_ARG(0, int) op NODE_ARG(1, int)); \
    }

    RELOP(IntType::equals, ==)
    RELOP(IntType::notEquals, !=)
    RELOP(IntType::lessThan, <)
    RELOP(IntType::greaterThan, >)
    RELOP(IntType::lessThanEq, <=)
    RELOP(IntType::greaterThanEq, >=)
#undef RELOP

    NODE_IMPLEMENTATION(IntType::conditionalExpr, int)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, int) : NODE_ARG(2, int));
    }

    NODE_IMPLEMENTATION(IntType::negate, int)
    {
        NODE_RETURN(-NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(IntType::preInc, int)
    {
        using namespace Mu;
        int* ip = reinterpret_cast<int*>(NODE_ARG(0, Pointer));
        return ++(*ip);
    }

    NODE_IMPLEMENTATION(IntType::postInc, int)
    {
        using namespace Mu;
        int* ip = reinterpret_cast<int*>(NODE_ARG(0, Pointer));
        return (*ip)++;
    }

    NODE_IMPLEMENTATION(IntType::preDec, int)
    {
        using namespace Mu;
        int* ip = reinterpret_cast<int*>(NODE_ARG(0, Pointer));
        return --(*ip);
    }

    NODE_IMPLEMENTATION(IntType::postDec, int)
    {
        using namespace Mu;
        int* ip = reinterpret_cast<int*>(NODE_ARG(0, Pointer));
        return (*ip)--;
    }

#define ASOP(name, op)                                          \
    NODE_IMPLEMENTATION(name, Pointer)                          \
    {                                                           \
        using namespace Mu;                                     \
        int* ip = reinterpret_cast<int*>(NODE_ARG(0, Pointer)); \
        *ip op NODE_ARG(1, int);                                \
        NODE_RETURN((Pointer)ip);                               \
    }

    ASOP(IntType::assign, =)
    ASOP(IntType::assignPlus, +=)
    ASOP(IntType::assignSub, -=)
    ASOP(IntType::assignMult, *=)
    ASOP(IntType::assignDiv, /=)
    ASOP(IntType::assignMod, %=)
#undef ASOP

} // namespace Mu
