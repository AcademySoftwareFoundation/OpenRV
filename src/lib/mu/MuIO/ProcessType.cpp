//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuIO/ProcessType.h>
#include <MuIO/IStreamType.h>
#include <MuIO/OStreamType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Exception.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <Mu/List.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <ctype.h>
#include <iostream>
#include <sstream>

namespace Mu
{
    using namespace std;

    ProcessType::PipeStream::PipeStream(const Class* c)
        : ClassInstance(c)
    {
    }

    ProcessType::PipeStream::~PipeStream() {}

    //----------------------------------------------------------------------

    ProcessType::ProcessType(Context* c, const char* name, Class* super)
        : Class(c, name, super)
    {
        _isSerializable = false;
    }

    ProcessType::~ProcessType() {}

    Object* ProcessType::newObject() const { return new PipeStream(this); }

    void ProcessType::deleteObject(Object* obj) const
    {
        delete static_cast<ProcessType::PipeStream*>(obj);
    }

    void ProcessType::outputValue(ostream& o, const Value& value,
                                  bool full) const
    {
        PipeStream* s = reinterpret_cast<PipeStream*>(value._Pointer);

        if (s)
        {
            o << "<#" << s->type()->fullyQualifiedName() << " 0x" << hex
              << s->exec_stream << dec << ">";
        }
        else
        {
            o << "nil";
        }
    }

    void ProcessType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                           ValueOutputState& state) const
    {
        if (vp)
        {
            const PipeStream* s = *reinterpret_cast<const PipeStream**>(vp);
            outputValueRecursive(o, ValuePointer(&s), state);
        }
        else
        {
            o << "nil";
        }
    }

    void ProcessType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        String tname = name();
        String rname = tname;
        rname += "&";

        const char* n = tname.c_str();
        const char* r = rname.c_str();

        String qtname = fullyQualifiedName();
        String qrname = qtname + "&";

        const char* tn = qtname.c_str();
        const char* rn = qrname.c_str();

        s->addSymbols(new ReferenceType(c, r, this),

                      new Function(c, n, BaseFunctions::dereference, Cast,
                                   Return, tn, Args, rn, End),

                      new Function(c, n, ProcessType::construct, None, Return,
                                   tn, Args, "string", "[string]", "int64",
                                   End),

                      EndArguments);

        globalScope()->addSymbols(
            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            new Function(c, "bool", ProcessType::toBool, None, Return, "bool",
                         Args, tn, End),

            EndArguments);

        addSymbols(new Function(c, "in", ProcessType::inStream, None, Return,
                                "io.ostream", Args, tn, End),

                   new Function(c, "out", ProcessType::outStream, None, Return,
                                "io.istream", Args, tn, End),

                   new Function(c, "err", ProcessType::errStream, None, Return,
                                "io.istream", Args, tn, End),

                   new Function(c, "close", ProcessType::close, None, Return,
                                "bool", Args, tn, End),

                   new Function(c, "close_in", ProcessType::close_in, None,
                                Return, "bool", Args, tn, End),

                   new Function(c, "kill", ProcessType::kill, None, Return,
                                "void", Args, tn, End),

                   new Function(c, "exit_code", ProcessType::exit_code, None,
                                Return, "int", Args, tn, End),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(ProcessType::construct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const Class*>(NODE_THIS.type());
        PipeStream* o = new PipeStream(c);

        vector<string> args;
        const StringType::String* program =
            NODE_ARG_OBJECT(0, StringType::String);

        for (List list(p, NODE_ARG_OBJECT(1, ClassInstance)); list.isNotNil();
             list++)
        {
            args.push_back(list.value<const StringType::String*>()->c_str());
        }

        o->exec_stream = new exec_stream_t();
        o->exec_stream->set_wait_timeout(exec_stream_t::s_all,
                                         NODE_ARG(2, int64));
        o->exec_stream->start(program->c_str(), args.begin(), args.end());

        OStreamType::OStream* oin = new OStreamType::OStream(c);
        oin->_ios = oin->_ostream = &o->exec_stream->in();

        IStreamType::IStream* oout = new IStreamType::IStream(c);
        oout->_ios = oout->_istream = &o->exec_stream->out();

        IStreamType::IStream* oerr = new IStreamType::IStream(c);
        oerr->_ios = oerr->_istream = &o->exec_stream->err();

        o->in = oin;
        o->out = oout;
        o->err = oerr;

        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(ProcessType::toBool, bool)
    {
        PipeStream* pstream = NODE_ARG_OBJECT(0, PipeStream);
        if (!pstream)
            throw NilArgumentException(NODE_THREAD);
        NODE_RETURN(pstream->exec_stream ? true : false);
    }

    NODE_IMPLEMENTATION(ProcessType::inStream, Pointer)
    {
        PipeStream* pstream = NODE_ARG_OBJECT(0, PipeStream);
        if (!pstream)
            throw NilArgumentException(NODE_THREAD);
        if (!pstream->exec_stream)
            throw BadArgumentException(NODE_THREAD);
        NODE_RETURN(pstream->in);
    }

    NODE_IMPLEMENTATION(ProcessType::outStream, Pointer)
    {
        PipeStream* pstream = NODE_ARG_OBJECT(0, PipeStream);
        if (!pstream)
            throw NilArgumentException(NODE_THREAD);
        if (!pstream->exec_stream)
            throw BadArgumentException(NODE_THREAD);
        NODE_RETURN(pstream->out);
    }

    NODE_IMPLEMENTATION(ProcessType::errStream, Pointer)
    {
        PipeStream* pstream = NODE_ARG_OBJECT(0, PipeStream);
        if (!pstream)
            throw NilArgumentException(NODE_THREAD);
        if (!pstream->exec_stream)
            throw BadArgumentException(NODE_THREAD);
        NODE_RETURN(pstream->err);
    }

    NODE_IMPLEMENTATION(ProcessType::close, bool)
    {
        PipeStream* pstream = NODE_ARG_OBJECT(0, PipeStream);
        if (!pstream)
            throw NilArgumentException(NODE_THREAD);
        if (!pstream->exec_stream)
            NODE_RETURN(true);
        bool b = pstream->exec_stream->close();
        delete pstream->exec_stream;
        pstream->exec_stream = 0;
        NODE_RETURN(b);
    }

    NODE_IMPLEMENTATION(ProcessType::close_in, bool)
    {
        PipeStream* pstream = NODE_ARG_OBJECT(0, PipeStream);
        if (!pstream)
            throw NilArgumentException(NODE_THREAD);
        NODE_RETURN(pstream->exec_stream->close_in());
    }

    NODE_IMPLEMENTATION(ProcessType::kill, void)
    {
        PipeStream* pstream = NODE_ARG_OBJECT(0, PipeStream);
        if (!pstream)
            throw NilArgumentException(NODE_THREAD);
        if (!pstream->exec_stream)
            return;
        pstream->exec_stream->kill();
        delete pstream->exec_stream;
        pstream->exec_stream = 0;
    }

    NODE_IMPLEMENTATION(ProcessType::exit_code, int)
    {
        PipeStream* pstream = NODE_ARG_OBJECT(0, PipeStream);
        if (!pstream)
            throw NilArgumentException(NODE_THREAD);
        if (!pstream->exec_stream)
            return 0;
        NODE_RETURN(pstream->exec_stream->exit_code());
    }

} // namespace Mu
