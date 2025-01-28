//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/ByteType.h>
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

    ByteType::ByteType(Context* c)
        : PrimitiveType(c, "byte", CharRep::rep())
    {
    }

    ByteType::~ByteType() {}

    PrimitiveObject* ByteType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value ByteType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._charFunc)(*n, thread));
    }

    void ByteType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        char* ip = reinterpret_cast<char*>(p);
        *ip = (*n->func()._charFunc)(*n, thread);
    }

    void ByteType::outputValue(ostream& o, const Value& value, bool full) const
    {
        o << int(byte(value._char));
    }

    void ByteType::outputValueRecursive(ostream& o, const ValuePointer p,
                                        ValueOutputState&) const
    {
        o << int(byte((*reinterpret_cast<const char*>(p))));
    }

    //
    //  These are the non-inlined versions of the functions
    //  in Native.h. They are passed in as the Compiled function
    //  for the ints.
    //

#define T Thread&
#define byte char

    byte __C_byte_byte(T) { return 0; }

    byte __C_byte_byte_byteAmp_(T, byte& a) { return a; }

    byte __C_Minus__byte_byte(T, byte a) { return -a; }

    byte __C_Tilde__byte_byte(T, byte a) { return ~a; }

    byte __C_Plus__byte_byte_byte(T, byte a, byte b) { return a + b; }

    byte __C_Minus__byte_byte_byte(T, byte a, byte b) { return a - b; }

    byte __C_Star__byte_byte_byte(T, byte a, byte b) { return a * b; }

    byte __C_Slash__byte_byte_byte(T, byte a, byte b) { return a / b; }

    byte __C_PCent__byte_byte_byte(T, byte a, byte b) { return a % b; }

    byte __C_Caret__byte_byte_byte(T, byte a, byte b) { return a ^ b; }

    byte __C_Amp__byte_byte_byte(T, byte a, byte b) { return a & b; }

    byte __C_Pipe__byte_byte_byte(T, byte a, byte b) { return a | b; }

    byte __C_GT_GT___byte_byte_byte(T, byte a, byte b) { return a >> b; }

    byte __C_LT_LT___byte_byte_byte(T, byte a, byte b) { return a << b; }

    bool __C_GT__bool_byte_byte(T, byte a, byte b) { return a > b; }

    bool __C_LT__bool_byte_byte(T, byte a, byte b) { return a < b; }

    bool __C_GT_EQ__bool_byte_byte(T, byte a, byte b) { return a >= b; }

    bool __C_LT_EQ__bool_byte_byte(T, byte a, byte b) { return a <= b; }

    bool __C_EQ_EQ__bool_byte_byte(T, byte a, byte b) { return a == b; }

    bool __C_Bang_EQ__bool_byte_byte(T, byte a, byte b) { return a != b; }

    byte& __C_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b) { return a = b; }

    byte& __C_Plus_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a += b;
    }

    byte& __C_Minus_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a -= b;
    }

    byte& __C_Star_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a *= b;
    }

    byte& __C_Slash_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a /= b;
    }

    byte& __C_PCent_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a %= b;
    }

    byte& __C_Caret_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a ^= b;
    }

    byte& __C_GT_GT_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a >>= b;
    }

    byte& __C_LT_LT_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a <<= b;
    }

    byte& __C_Pipe_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a |= b;
    }

    byte& __C_Amp_EQ__byteAmp__byteAmp__byte(T, byte& a, byte b)
    {
        return a &= b;
    }

    byte __C_prePlus_Plus__byte_byteAmp_(T, byte& a) { return ++a; }

    byte __C_postPlus_Plus__byte_byteAmp_(T, byte& a) { return a++; }

    byte __C_preMinus_Minus__byte_byteAmp_(T, byte& a) { return --a; }

    byte __C_postMinus_Minus__byte_byteAmp_(T, byte& a) { return a--; }

    byte __C_byte_byte_float(T, float a) { return byte(a); }

    byte __C_byte_byte_int(T, int a) { return byte(a); }

    byte __C_byte_byte_int64(T, int64 a) { return byte(a); }

    byte __C_byte_byte_char(T, int a) { return byte(a); }

    int __C_int_int_byte(T, byte a) { return int(a); }

    //
    //  This function is delt with directly by muc, it translates
    //  to an if statement. But we still need a compiled version for
    //  lambda expressions.
    //

    byte __C_QMark_Colon__bool_byte_byte(T, bool p, byte a, byte b)
    {
        return p ? a : b;
    }

#undef T
#undef byte

    void ByteType::load()
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
            new ReferenceType(c, "byte&", this),

            new Function(c, "byte", ByteType::defaultByte, Mapped, Compiled,
                         __C_byte_byte, Return, "byte", End),

            new Function(c, "byte", ByteType::dereference, Cast, Compiled,
                         __C_byte_byte_byteAmp_, Return, "byte", Args, "byte&",
                         End),

            new Function(c, "byte", ByteType::fromInt64, Cast | Lossy, Compiled,
                         __C_byte_byte_int64, Return, "byte", Args, "int64",
                         End),

            new Function(c, "byte", ByteType::fromInt, Cast | Lossy, Compiled,
                         __C_byte_byte_int, Return, "byte", Args, "int", End),

            new Function(c, "byte", ByteType::fromChar, Cast | Lossy, Compiled,
                         __C_byte_byte_char, Return, "byte", Args, "char", End),

            new Function(c, "int", ByteType::fromByte, Cast, Compiled,
                         __C_int_int_byte, Return, "int", Args, "byte", End),

            new Function(c, "+", ByteType::add, CommOp, Compiled,
                         __C_Plus__byte_byte_byte, Return, "byte", Args, "byte",
                         "byte", End),

            new Function(c, "-", ByteType::sub, Op, Compiled,
                         __C_Minus__byte_byte_byte, Return, "byte", Args,
                         "byte", "byte", End),

            new Function(c, "-", ByteType::negate, Op, Compiled,
                         __C_Minus__byte_byte, Return, "byte", Args, "byte",
                         End),

            new Function(c, "*", ByteType::mult, CommOp, Compiled,
                         __C_Star__byte_byte_byte, Return, "byte", Args, "byte",
                         "byte", End),

            new Function(c, "/", ByteType::div, Op, Compiled,
                         __C_Slash__byte_byte_byte, Return, "byte", Args,
                         "byte", "byte", End),

            new Function(c, "%", ByteType::mod, Op, Compiled,
                         __C_PCent__byte_byte_byte, Return, "byte", Args,
                         "byte", "byte", End),

            new Function(c, "=", ByteType::assign, AsOp, Compiled,
                         __C_EQ__byteAmp__byteAmp__byte, Return, "byte&", Args,
                         "byte&", "byte", End),

            new Function(c, "+=", ByteType::assignPlus, AsOp, Compiled,
                         __C_Plus_EQ__byteAmp__byteAmp__byte, Return, "byte&",
                         Args, "byte&", "byte", End),

            new Function(c, "-=", ByteType::assignSub, AsOp, Compiled,
                         __C_Minus_EQ__byteAmp__byteAmp__byte, Return, "byte&",
                         Args, "byte&", "byte", End),

            new Function(c, "*=", ByteType::assignMult, AsOp, Compiled,
                         __C_Star_EQ__byteAmp__byteAmp__byte, Return, "byte&",
                         Args, "byte&", "byte", End),

            new Function(c, "/=", ByteType::assignDiv, AsOp, Compiled,
                         __C_Slash_EQ__byteAmp__byteAmp__byte, Return, "byte&",
                         Args, "byte&", "byte", End),

            new Function(c, "%=", ByteType::assignDiv, AsOp, Compiled,
                         __C_PCent_EQ__byteAmp__byteAmp__byte, Return, "byte&",
                         Args, "byte&", "byte", End),

            new Function(c, "?:", ByteType::conditionalExpr,
                         Op ^ NativeInlined, // not inlined
                         Compiled, __C_QMark_Colon__bool_byte_byte, Return,
                         "byte", Args, "bool", "byte", "byte", End),

            new Function(c, "==", ByteType::equals, CommOp, Compiled,
                         __C_EQ_EQ__bool_byte_byte, Return, "bool", Args,
                         "byte", "byte", End),

            new Function(c, "!=", ByteType::notEquals, CommOp, Compiled,
                         __C_Bang_EQ__bool_byte_byte, Return, "bool", Args,
                         "byte", "byte", End),

            new Function(c, ">=", ByteType::greaterThanEq, Op, Compiled,
                         __C_GT_EQ__bool_byte_byte, Return, "bool", Args,
                         "byte", "byte", End),

            new Function(c, "<=", ByteType::lessThanEq, Op, Compiled,
                         __C_LT_EQ__bool_byte_byte, Return, "bool", Args,
                         "byte", "byte", End),

            new Function(c, "<", ByteType::lessThan, Op, Compiled,
                         __C_LT__bool_byte_byte, Return, "bool", Args, "byte",
                         "byte", End),

            new Function(c, ">", ByteType::greaterThan, Op, Compiled,
                         __C_GT__bool_byte_byte, Return, "bool", Args, "byte",
                         "byte", End),

            new Function(c, "|", ByteType::bitOr, CommOp, Compiled,
                         __C_Pipe__byte_byte_byte, Return, "byte", Args, "byte",
                         "byte", End),

            new Function(c, "&", ByteType::bitAnd, CommOp, Compiled,
                         __C_Amp__byte_byte_byte, Return, "byte", Args, "byte",
                         "byte", End),

            new Function(c, "^", ByteType::bitXor, CommOp, Compiled,
                         __C_Caret__byte_byte_byte, Return, "byte", Args,
                         "byte", "byte", End),

            new Function(c, "~", ByteType::bitNot, Op, Compiled,
                         __C_Tilde__byte_byte, Return, "byte", Args, "byte",
                         End),

            new Function(c, "<<", ByteType::shiftLeft, Op, Compiled,
                         __C_LT_LT___byte_byte_byte, Return, "byte", Args,
                         "byte", "byte", End),

            new Function(c, ">>", ByteType::shiftRight, Op, Compiled,
                         __C_GT_GT___byte_byte_byte, Return, "byte", Args,
                         "byte", "byte", End),

            new Function(c, "pre++", ByteType::preInc, Op, Compiled,
                         __C_prePlus_Plus__byte_byteAmp_, Return, "byte", Args,
                         "byte&", End),

            new Function(c, "post++", ByteType::postInc, Op, Compiled,
                         __C_postPlus_Plus__byte_byteAmp_, Return, "byte", Args,
                         "byte&", End),

            new Function(c, "pre--", ByteType::preDec, Op, Compiled,
                         __C_preMinus_Minus__byte_byteAmp_, Return, "byte",
                         Args, "byte&", End),

            new Function(c, "post--", ByteType::postDec, Op, Compiled,
                         __C_postMinus_Minus__byte_byteAmp_, Return, "byte",
                         Args, "byte&", End),

            EndArguments);

        this->addSymbols(
            new SymbolicConstant(c, "max", "byte",
                                 Value(numeric_limits<unsigned char>::max())),

            new SymbolicConstant(c, "min", "byte",
                                 Value(numeric_limits<unsigned char>::min())),
            EndArguments);
    }

    NODE_IMPLEMENTATION(ByteType::defaultByte, char) { NODE_RETURN(char(0)); }

    NODE_IMPLEMENTATION(ByteType::fromInt, char)
    {
        NODE_RETURN(NODE_ARG(0, int));
    }

    NODE_IMPLEMENTATION(ByteType::fromInt64, char)
    {
        NODE_RETURN(NODE_ARG(0, int64));
    }

    NODE_IMPLEMENTATION(ByteType::fromChar, char)
    {
        NODE_RETURN(NODE_ARG(0, char));
    }

    NODE_IMPLEMENTATION(ByteType::fromByte, int)
    {
        NODE_RETURN(NODE_ARG(0, char));
    }

    NODE_IMPLEMENTATION(ByteType::dereference, char)
    {
        char* ip = reinterpret_cast<char*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*ip);
    }

    NODE_IMPLEMENTATION(ByteType::bitNot, char)
    {
        NODE_RETURN(char(~(NODE_ARG(0, char))));
    }

#define OP(name, op)                                         \
    NODE_IMPLEMENTATION(name, char)                          \
    {                                                        \
        NODE_RETURN(NODE_ARG(0, char) op NODE_ARG(1, char)); \
    }

    OP(ByteType::add, +)
    OP(ByteType::sub, -)
    OP(ByteType::mult, *)
    OP(ByteType::div, /)
    OP(ByteType::mod, %)
    OP(ByteType::bitAnd, &)
    OP(ByteType::bitOr, |)
    OP(ByteType::bitXor, ^)
    OP(ByteType::shiftLeft, <<)
    OP(ByteType::shiftRight, >>)
#undef OP

#define RELOP(name, op)                                      \
    NODE_IMPLEMENTATION(name, bool)                          \
    {                                                        \
        NODE_RETURN(NODE_ARG(0, char) op NODE_ARG(1, char)); \
    }

    RELOP(ByteType::equals, ==)
    RELOP(ByteType::notEquals, !=)
    RELOP(ByteType::lessThan, <)
    RELOP(ByteType::greaterThan, >)
    RELOP(ByteType::lessThanEq, <=)
    RELOP(ByteType::greaterThanEq, >=)
#undef RELOP

    NODE_IMPLEMENTATION(ByteType::conditionalExpr, char)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, char) : NODE_ARG(2, char));
    }

    NODE_IMPLEMENTATION(ByteType::negate, char)
    {
        NODE_RETURN(-NODE_ARG(0, char));
    }

    NODE_IMPLEMENTATION(ByteType::preInc, char)
    {
        char* ip = reinterpret_cast<char*>(NODE_ARG(0, Pointer));
        return ++(*ip);
    }

    NODE_IMPLEMENTATION(ByteType::postInc, char)
    {
        char* ip = reinterpret_cast<char*>(NODE_ARG(0, Pointer));
        return (*ip)++;
    }

    NODE_IMPLEMENTATION(ByteType::preDec, char)
    {
        char* ip = reinterpret_cast<char*>(NODE_ARG(0, Pointer));
        return --(*ip);
    }

    NODE_IMPLEMENTATION(ByteType::postDec, char)
    {
        char* ip = reinterpret_cast<char*>(NODE_ARG(0, Pointer));
        return (*ip)--;
    }

    NODE_IMPLEMENTATION(ByteType::__assign, void)
    {
        for (int i = 0, s = NODE_NUM_ARGS(); i < s; i += 2)
        {
            char* ip = reinterpret_cast<char*>(NODE_ARG(i, Pointer));
            *ip = NODE_ARG(i + 1, char);
        }
    }

#define ASOP(name, op)                                            \
    NODE_IMPLEMENTATION(name, Pointer)                            \
    {                                                             \
        char* ip = reinterpret_cast<char*>(NODE_ARG(0, Pointer)); \
        *ip op NODE_ARG(1, char);                                 \
        NODE_RETURN(ip);                                          \
    }

    ASOP(ByteType::assign, =)
    ASOP(ByteType::assignPlus, +=)
    ASOP(ByteType::assignSub, -=)
    ASOP(ByteType::assignMult, *=)
    ASOP(ByteType::assignDiv, /=)
    ASOP(ByteType::assignMod, %=)
#undef ASOP

} // namespace Mu
