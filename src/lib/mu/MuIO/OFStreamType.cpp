//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuIO/OFStreamType.h>
#include <MuIO/StreamType.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Module.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <Mu/Context.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/StringType.h>
#include <MuLang/MuLangContext.h>
#include <ctype.h>
#include <iostream>
#include <sstream>

namespace Mu
{
    using namespace std;
    using namespace Mu;

    void OFStreamType::finalizer(void* obj, void* data)
    {
        OFStreamType::OFStream* i =
            reinterpret_cast<OFStreamType::OFStream*>(obj);

        if (i->_ofstream != &cout && i->_ofstream != &cerr)
        {
            delete i->_ofstream;
            i->_ofstream = 0;
        }
    }

    OFStreamType::OFStream::OFStream(const Class* c)
        : OStreamType::OStream(c)
    {
        GC_register_finalizer(this, OFStreamType::finalizer, 0, 0, 0);
    }

    OFStreamType::OFStream::~OFStream() {}

    //----------------------------------------------------------------------

    OFStreamType::OFStreamType(Context* c, const char* name, Class* super)
        : OStreamType(c, name, super)
    {
    }

    OFStreamType::~OFStreamType() {}

    Object* OFStreamType::newObject() const { return new OFStream(this); }

    void OFStreamType::deleteObject(Object* obj) const
    {
        delete static_cast<OFStreamType::OFStream*>(obj);
    }

    void OFStreamType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        String tname = fullyQualifiedName();
        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        s->addSymbols(new ReferenceType(c, "ofstream&", this),

                      new Function(c, "ofstream", OFStreamType::construct0,
                                   None, Return, tn, End),

                      new Function(c, "ofstream", OFStreamType::construct, None,
                                   Return, tn, Args, "string", End),

                      new Function(c, "ofstream", OFStreamType::construct1,
                                   None, Return, tn, Args, "string", "int",
                                   End),

                      new Function(c, "ofstream", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "print", StreamType::print, None, Return, "void",
                         Args, tn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);

        addSymbols(new Function(c, "close", OFStreamType::close, None, Return,
                                "void", Args, tn, End),

                   new Function(c, "open", OFStreamType::open, None, Return,
                                "void", Args, tn, "string", "int", End),

                   new Function(c, "is_open", OFStreamType::is_open, None,
                                Return, "bool", Args, tn, End),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(OFStreamType::construct0, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const OFStreamType*>(NODE_THIS.type());
        OFStreamType::OFStream* o = new OFStreamType::OFStream(c);
        o->_ios = o->_ostream = o->_ofstream = new std::ofstream();
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(OFStreamType::construct, Pointer)
    {
        const StringType::String* file = NODE_ARG_OBJECT(0, StringType::String);
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const OFStreamType*>(NODE_THIS.type());
        OFStreamType::OFStream* o = new OFStreamType::OFStream(c);
        o->setString(file->c_str());
        o->_ios = o->_ostream = o->_ofstream =
            new std::ofstream(UNICODE_C_STR(file->c_str()));
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(OFStreamType::construct1, Pointer)
    {
        const StringType::String* file = NODE_ARG_OBJECT(0, StringType::String);
        ios_base::openmode mode = (ios_base::openmode)NODE_ARG(1, int);

        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const OFStreamType*>(NODE_THIS.type());
        OFStreamType::OFStream* o = new OFStreamType::OFStream(c);
        o->setString(file->c_str());
        o->_ios = o->_ostream = o->_ofstream =
            new std::ofstream(UNICODE_C_STR(file->c_str()), mode);
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(OFStreamType::close, void)
    {
        OFStreamType::OFStream* stream =
            NODE_ARG_OBJECT(0, OFStreamType::OFStream);
        stream->_ofstream->close();

        if (stream->_ofstream != &cout && stream->_ofstream != &cerr)
        {
            delete stream->_ofstream;
            stream->_ofstream = 0;
        }
    }

    NODE_IMPLEMENTATION(OFStreamType::is_open, bool)
    {
        OFStreamType::OFStream* stream =
            NODE_ARG_OBJECT(0, OFStreamType::OFStream);
        NODE_RETURN(stream->_ofstream->is_open());
    }

    NODE_IMPLEMENTATION(OFStreamType::open, void)
    {
        OFStreamType::OFStream* stream =
            NODE_ARG_OBJECT(0, OFStreamType::OFStream);
        const StringType::String* file = NODE_ARG_OBJECT(1, StringType::String);
        int flags = NODE_ARG(2, int);
        stream->_ofstream->open(UNICODE_C_STR(file->c_str()),
                                ios::openmode(flags));
    }

} // namespace Mu
