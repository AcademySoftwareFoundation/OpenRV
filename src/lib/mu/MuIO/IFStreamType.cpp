//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuIO/IFStreamType.h>
#include <MuIO/StreamType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <Mu/Context.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <ctype.h>
#include <iostream>
#include <sstream>

/* AJG - TIFF? */
// #include <tiffio.h>

namespace Mu
{
    using namespace std;

    void IFStreamType::finalizer(void* obj, void* data)
    {
        IFStreamType::IFStream* i =
            reinterpret_cast<IFStreamType::IFStream*>(obj);

        if (i->_ifstream != &cin)
        {
            delete i->_ifstream;
            i->_ifstream = 0;
        }
    }

    IFStreamType::IFStream::IFStream(const Class* c)
        : IStreamType::IStream(c)
    {
        GC_register_finalizer(this, IFStreamType::finalizer, 0, 0, 0);
    }

    IFStreamType::IFStream::~IFStream() {}

    //----------------------------------------------------------------------

    IFStreamType::IFStreamType(Context* c, const char* name, Class* super)
        : IStreamType(c, name, super)
    {
    }

    IFStreamType::~IFStreamType() {}

    Object* IFStreamType::newObject() const { return new IFStream(this); }

    void IFStreamType::deleteObject(Object* obj) const
    {
        delete static_cast<IFStreamType::IFStream*>(obj);
    }

    void IFStreamType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();
        MuLangContext* context = (MuLangContext*)globalModule()->context();

        String tname = fullyQualifiedName();
        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        s->addSymbols(new ReferenceType(c, "ifstream&", this),

                      new Function(c, "ifstream", IFStreamType::construct0,
                                   None, Return, tn, End),

                      new Function(c, "ifstream", IFStreamType::construct, None,
                                   Return, tn, Args, "string", End),

                      new Function(c, "ifstream", IFStreamType::construct1,
                                   None, Return, tn, Args, "string", "int",
                                   End),

                      new Function(c, "ifstream", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "print", StreamType::print, None, Return, "void",
                         Args, tn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);

        addSymbols(new Function(c, "close", IFStreamType::close, None, Return,
                                "void", Args, tn, End),

                   new Function(c, "open", IFStreamType::open, None, Return,
                                "void", Args, tn, "string", "int", End),

                   new Function(c, "is_open", IFStreamType::is_open, None,
                                Return, "bool", Args, tn, End),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(IFStreamType::construct0, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const IFStreamType*>(NODE_THIS.type());
        IFStreamType::IFStream* o = new IFStreamType::IFStream(c);
        o->_ios = o->_istream = o->_ifstream = new std::ifstream();
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(IFStreamType::construct, Pointer)
    {
        const StringType::String* file = NODE_ARG_OBJECT(0, StringType::String);

        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const IFStreamType*>(NODE_THIS.type());
        IFStreamType::IFStream* o = new IFStreamType::IFStream(c);
        o->setString(file->c_str());
        o->_ios = o->_istream = o->_ifstream =
            new std::ifstream(UNICODE_C_STR(file->c_str()));
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(IFStreamType::construct1, Pointer)
    {
        const StringType::String* file = NODE_ARG_OBJECT(0, StringType::String);
        ios_base::openmode mode = (ios_base::openmode)NODE_ARG(1, int);

        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const IFStreamType*>(NODE_THIS.type());
        IFStreamType::IFStream* o = new IFStreamType::IFStream(c);
        o->setString(file->c_str());
        o->_ios = o->_istream = o->_ifstream =
            new std::ifstream(UNICODE_C_STR(file->c_str()), mode);
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(IFStreamType::close, void)
    {
        IFStreamType::IFStream* stream =
            NODE_ARG_OBJECT(0, IFStreamType::IFStream);
        stream->_ifstream->close();

        if (stream->_ifstream != &cin)
        {
            delete stream->_ifstream;
            stream->_ifstream = 0;
        }
    }

    NODE_IMPLEMENTATION(IFStreamType::is_open, bool)
    {
        IFStreamType::IFStream* stream =
            NODE_ARG_OBJECT(0, IFStreamType::IFStream);
        NODE_RETURN(stream->_ifstream->is_open());
    }

    NODE_IMPLEMENTATION(IFStreamType::open, void)
    {
        IFStreamType::IFStream* stream =
            NODE_ARG_OBJECT(0, IFStreamType::IFStream);
        const StringType::String* file = NODE_ARG_OBJECT(1, StringType::String);
        int flags = NODE_ARG(2, int);
        stream->_ifstream->open(UNICODE_C_STR(file->c_str()),
                                ios::openmode(flags));
    }

} // namespace Mu
