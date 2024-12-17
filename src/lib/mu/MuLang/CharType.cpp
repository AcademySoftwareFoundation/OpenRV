//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuLang/CharType.h>
#include <MuLang/IntType.h>
#include <MuLang/StringType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <Mu/Function.h>
#include <Mu/MuProcess.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/Thread.h>
#include <Mu/ReferenceType.h>
#include <Mu/Value.h>
#include <Mu/MachineRep.h>
#include <iostream>
#include <sstream>
#include <Mu/utf8_v2/unchecked.h>
#include <Mu/utf8_v2/checked.h>

namespace Mu
{
    using namespace std;

    CharType::CharType(Context* c)
        : PrimitiveType(c, "char", IntRep::rep())
    {
    }

    CharType::~CharType() {}

    PrimitiveObject* CharType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value CharType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._intFunc)(*n, thread));
    }

    void CharType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        int* ip = reinterpret_cast<int*>(p);
        *ip = (*n->func()._intFunc)(*n, thread);
    }

    void CharType::outputValue(ostream& o, const Value& value, bool full) const
    {
        String s;
        int c = value._int;
        utf8::utf32to8(&c, &c + 1, back_inserter(s));
        StringType::outputQuotedString(o, s, '\'');
    }

    void CharType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                        ValueOutputState& state) const
    {
        String s;
        int c = *reinterpret_cast<const int*>(vp);
        utf8::utf32to8(&c, &c + 1, back_inserter(s));
        StringType::outputQuotedString(o, s, '\'');
    }

    //
    //  These are the non-inlined versions of the functions in Native.h.
    //  Be aware of the Mu "char" versus C "char" issue.
    //

#define T Thread&

    int __C_char_char(T) { return 0; }

    int __C_char_char_charAmp_(T, int& a) { return a; }

    int __C_Minus__char_char(T, int a) { return -a; }

    int __C_Plus__char_char_int(T, int a, int b) { return a + b; }

    int __C_Minus__char_char_int(T, int a, int b) { return a - b; }

    int __C_Minus__int_char_char(T, int a, int b) { return a - b; }

    bool __C_GT__bool_char_char(T, int a, int b) { return a > b; }

    bool __C_LT__bool_char_char(T, int a, int b) { return a < b; }

    bool __C_GT_EQ__bool_char_char(T, int a, int b) { return a >= b; }

    bool __C_LT_EQ__bool_char_char(T, int a, int b) { return a <= b; }

    bool __C_EQ_EQ__bool_char_char(T, int a, int b) { return a == b; }

    bool __C_Bang_EQ__bool_char_char(T, int a, int b) { return a != b; }

    int& __C_EQ__charAmp__charAmp__char(T, int& a, int b) { return a = b; }

    int& __C_Plus_EQ__charAmp__charAmp__int(T, int& a, int b) { return a += b; }

    int& __C_Minus_EQ__charAmp__charAmp__int(T, int& a, int b)
    {
        return a -= b;
    }

    int __C_prePlus_Plus__char_charAmp_(T, int& a) { return ++a; }

    int __C_postPlus_Plus__char_charAmp_(T, int& a) { return a++; }

    int __C_preMinus_Minus__char_charAmp_(T, int& a) { return --a; }

    int __C_postMinus_Minus__char_charAmp_(T, int& a) { return a--; }

    int __C_char_char_int(T, int a) { return a; }

    int __C_int_int_char(T, int a) { return a; }

    void __C_print_void_char(T, int a)
    {
        cout << "PRINT: " << a << endl << flush;
    }

    //
    //  This function is delt with directly by muc, it translates
    //  to an if statement. But we still need a compiled version for
    //  lambda expressions.
    //

    char __C_QMark_Colon__bool_char_char(T, bool p, int a, int b)
    {
        return p ? a : b;
    }

#undef T

    void CharType::load()
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
            new ReferenceType(c, "char&", this),

            new Function(c, "char", IntType::defaultInt, Mapped, Compiled,
                         __C_char_char, Return, "char", End),

            new Function(c, "char", IntType::dereference, Cast, Return, "char",
                         Args, "char&", End),

            new Function(c, "char", CharType::fromInt, Mapped, Compiled,
                         __C_char_char_int, Return, "char", Args, "int", End),

            // pass through same as fromInt
            new Function(c, "int", CharType::fromInt, Mapped, Compiled,
                         __C_int_int_char, Return, "int", Args, "char", End),

            new Function(c, "string", CharType::toString, Cast, Return,
                         "string", Args, "char", End),

            new Function(c, "=", IntType::assign, AsOp, Compiled,
                         __C_EQ__charAmp__charAmp__char, Return, "char&", Args,
                         "char&", "char", End),

            new Function(c, "?:", IntType::conditionalExpr, Op, Compiled,
                         __C_QMark_Colon__bool_char_char, Return, "char", Args,
                         "bool", "char", "char", End),

            new Function(c, "+", IntType::add, CommOp, Compiled,
                         __C_Plus__char_char_int, Return, "char", Args, "char",
                         "int", End),

            new Function(c, "-", IntType::sub, Op, Compiled,
                         __C_Minus__char_char_int, Return, "int", Args, "char",
                         "char", End),

            new Function(c, "-", IntType::sub, Op, Compiled,
                         __C_Minus__char_char_int, Return, "char", Args, "char",
                         "int", End),

            new Function(c, "+=", IntType::assignPlus, AsOp, Compiled,
                         __C_Plus_EQ__charAmp__charAmp__int, Return, "char&",
                         Args, "char&", "int", End),

            new Function(c, "-=", IntType::assignSub, AsOp, Compiled,
                         __C_Minus_EQ__charAmp__charAmp__int, Return, "char&",
                         Args, "char&", "int", End),

            new Function(c, "==", IntType::equals, CommOp, Compiled,
                         __C_EQ_EQ__bool_char_char, Return, "bool", Args,
                         "char", "char", End),

            new Function(c, "!=", IntType::notEquals, CommOp, Compiled,
                         __C_Bang_EQ__bool_char_char, Return, "bool", Args,
                         "char", "char", End),

            new Function(c, ">=", IntType::greaterThanEq, Op, Compiled,
                         __C_GT_EQ__bool_char_char, Return, "bool", Args,
                         "char", "char", End),

            new Function(c, "<=", IntType::lessThanEq, Op, Compiled,
                         __C_LT_EQ__bool_char_char, Return, "bool", Args,
                         "char", "char", End),

            new Function(c, "<", IntType::lessThan, Op, Compiled,
                         __C_LT__bool_char_char, Return, "bool", Args, "char",
                         "char", End),

            new Function(c, ">", IntType::greaterThan, Op, Compiled,
                         __C_GT__bool_char_char, Return, "bool", Args, "char",
                         "char", End),

            new Function(c, "pre++", IntType::preInc, Op, Compiled,
                         __C_prePlus_Plus__char_charAmp_, Return, "char", Args,
                         "char&", End),

            new Function(c, "post++", IntType::postInc, Op, Compiled,
                         __C_postPlus_Plus__char_charAmp_, Return, "char", Args,
                         "char&", End),

            new Function(c, "pre--", IntType::preDec, Op, Compiled,
                         __C_preMinus_Minus__char_charAmp_, Return, "char",
                         Args, "char&", End),

            new Function(c, "post--", IntType::postDec, Op, Compiled,
                         __C_postMinus_Minus__char_charAmp_, Return, "char",
                         Args, "char&", End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(CharType::toString, Pointer)
    {
        int c = NODE_ARG(0, int);
        Process* p = NODE_THREAD.process();
        const StringType* stype =
            static_cast<const StringType*>(NODE_THIS.type());

        String o;
        utf8::utf32to8(&c, &c + 1, back_inserter(o));
        NODE_RETURN(stype->allocate(o));
    }

    NODE_IMPLEMENTATION(CharType::fromInt, int)
    {
        int i = NODE_ARG(0, int);

        // if (utf8::is_valid(&i, &i+1))
        {
            NODE_RETURN(i);
        }
        // else
        // {
        //     ostringstream str;
        //     const Mu::MuLangContext* context =
        //         static_cast<const Mu::MuLangContext*>(NODE_THREAD.context());
        //     ExceptionType::Exception *e =
        //         new ExceptionType::Exception(context->exceptionType());
        //     str << "Invalid UTF32 code value: 0x" << hex << (unsigned int)i;
        //     e->string() = str.str().c_str();
        //     NODE_THREAD.setException(e);
        //     throw BadArgumentException(NODE_THREAD, e);
        // }
    }

} // namespace Mu
