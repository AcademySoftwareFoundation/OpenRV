//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuIO/StreamType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <ctype.h>
#include <iostream>
#include <sstream>

namespace Mu
{
    using namespace std;
    using namespace Mu;

    StreamType::Stream::Stream(const Class* c)
        : ClassInstance(c)
    {
    }

    StreamType::Stream::~Stream() {}

    //----------------------------------------------------------------------

    StreamType::StreamType(Context* c, const char* name, Class* super)
        : Class(c, name, super)
    {
        _isSerializable = false;
    }

    StreamType::~StreamType() {}

    Object* StreamType::newObject() const { return new Stream(this); }

    void StreamType::deleteObject(Object* obj) const
    {
        delete static_cast<StreamType::Stream*>(obj);
    }

    void StreamType::outputValue(ostream& o, const Value& value,
                                 bool full) const
    {
        Stream* s = reinterpret_cast<Stream*>(value._Pointer);

        if (s)
        {
            o << "<#" << s->type()->fullyQualifiedName() << " ";
            StringType::outputQuotedString(o, s->string());
            o << ">";
        }
        else
        {
            o << "nil";
        }
    }

    void StreamType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                          ValueOutputState& state) const
    {
        if (vp)
        {
            const Stream* s = *reinterpret_cast<const Stream**>(vp);
            outputValueRecursive(o, ValuePointer(&s), state);
        }
        else
        {
            o << "nil";
        }
    }

    void StreamType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        String tname = fullyQualifiedName();
        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        s->addSymbols(new ReferenceType(c, "stream&", this),

                      new Function(c, "stream", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "print", StreamType::print, None, Return, "void",
                         Args, tn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            new Function(c, "!", StreamType::eval, None, Return, "bool", Args,
                         tn, End),

            new Function(c, "bool", StreamType::toBool, None, Return, "bool",
                         Args, tn, End),

            EndArguments);

        addSymbols(

            new SymbolicConstant(c, "GoodBit", "int", Value(int(ios::goodbit))),
            new SymbolicConstant(c, "BadBit", "int", Value(int(ios::badbit))),
            new SymbolicConstant(c, "FailBit", "int", Value(int(ios::failbit))),
            new SymbolicConstant(c, "EofBit", "int", Value(int(ios::eofbit))),

            new SymbolicConstant(c, "Beginning", "int", Value(int(ios::beg))),
            new SymbolicConstant(c, "Current", "int", Value(int(ios::cur))),
            new SymbolicConstant(c, "End", "int", Value(int(ios::end))),

            new SymbolicConstant(c, "Append", "int", Value(int(ios::app))),
            new SymbolicConstant(c, "AtEnd", "int", Value(int(ios::ate))),
            new SymbolicConstant(c, "Binary", "int", Value(int(ios::binary))),
            new SymbolicConstant(c, "In", "int", Value(int(ios::in))),
            new SymbolicConstant(c, "Out", "int", Value(int(ios::out))),
            new SymbolicConstant(c, "Truncate", "int", Value(int(ios::trunc))),

            new Function(c, "bad", StreamType::bad, None, Return, "bool", Args,
                         tn, End),

            new Function(c, "eof", StreamType::eof, None, Return, "bool", Args,
                         tn, End),

            new Function(c, "fail", StreamType::fail, None, Return, "bool",
                         Args, tn, End),

            new Function(c, "good", StreamType::good, None, Return, "bool",
                         Args, tn, End),

            new Function(c, "clear", StreamType::clear, None, Return, "void",
                         Args, tn, End),

            new Function(c, "clear", StreamType::clear, None, Return, "void",
                         Args, tn, "int", End),

            new Function(c, "rdstate", StreamType::rdstate, None, Return, "int",
                         Args, tn, End),

            new Function(c, "setstate", StreamType::setstate, None, Return,
                         "void", Args, tn, "int", End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(StreamType::eval, bool)
    {
        Stream* s = NODE_ARG_OBJECT(0, Stream);
        NODE_RETURN(!(*s->_ios));
    }

    NODE_IMPLEMENTATION(StreamType::toBool, bool)
    {
        Stream* s = NODE_ARG_OBJECT(0, Stream);
        NODE_RETURN(!!(*s->_ios));
    }

    NODE_IMPLEMENTATION(StreamType::print, void)
    {
        Stream* i = NODE_ARG_OBJECT(0, Stream);
        i->type()->outputValue(cout, Value(i));
    }

    NODE_IMPLEMENTATION(StreamType::clear, void)
    {
        Stream* s = NODE_ARG_OBJECT(0, Stream);
        s->_ios->clear();
    }

    NODE_IMPLEMENTATION(StreamType::clearflag, void)
    {
        Stream* s = NODE_ARG_OBJECT(0, Stream);
        int f = NODE_ARG(1, int);
        s->_ios->clear((ios::iostate)f);
    }

    NODE_IMPLEMENTATION(StreamType::setstate, void)
    {
        Stream* s = NODE_ARG_OBJECT(0, Stream);
        int f = NODE_ARG(1, int);
        s->_ios->setstate((ios::iostate)f);
    }

    NODE_IMPLEMENTATION(StreamType::rdstate, int)
    {
        Stream* s = NODE_ARG_OBJECT(0, Stream);
        NODE_RETURN(int(s->_ios->rdstate()));
    }

#define CHECK_FUNC(NAME)                        \
    NODE_IMPLEMENTATION(StreamType::NAME, bool) \
    {                                           \
        Stream* s = NODE_ARG_OBJECT(0, Stream); \
        NODE_RETURN(s->_ios->NAME());           \
    }

    CHECK_FUNC(bad)
    CHECK_FUNC(good)
    CHECK_FUNC(fail)
    CHECK_FUNC(eof)

} // namespace Mu
