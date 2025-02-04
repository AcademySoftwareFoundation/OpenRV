//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuIO/OSStreamType.h>

#ifndef TWK_STUB_IT_OUT
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <ctype.h>
#include <iostream>
#include <sstream>

/* AJG - TIFF?  JIM JIM JIM? */
// #include <tiffio.h>

namespace Mu
{
    using namespace std;
    using namespace Mu;

    OSStreamType::OSStream::OSStream(const Class* c)
        : OStreamType::OStream(c)
    {
    }

    OSStreamType::OSStream::~OSStream() {}

    //----------------------------------------------------------------------

    OSStreamType::OSStreamType(Context* c, const char* name, Class* super)
        : OStreamType(c, name, super)
    {
    }

    OSStreamType::~OSStreamType() {}

    Object* OSStreamType::newObject() const { return new OSStream(this); }

    void OSStreamType::deleteObject(Object* obj) const
    {
        delete static_cast<OSStreamType::OSStream*>(obj);
    }

    void OSStreamType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        String tname = fullyQualifiedName();
        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        s->addSymbols(new ReferenceType(c, "osstream&", this),

                      new Function(c, "osstream", OSStreamType::construct, None,
                                   Return, tn, End),

                      new Function(c, "osstream", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "print", StreamType::print, None, Return, "void",
                         Args, tn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            new Function(c, "string", OSStreamType::tostring, None, Return,
                         "string", Args, tn, End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(OSStreamType::construct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const OSStreamType*>(NODE_THIS.type());
        OSStreamType::OSStream* o = new OSStreamType::OSStream(c);
        o->_ios = o->_ostream = o->_osstream = new std::ostringstream();
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(OSStreamType::tostring, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        OSStream* stream = NODE_ARG_OBJECT(0, OSStream);
        if (stream)
            NODE_RETURN(c->stringType()->allocate(stream->_osstream->str()));
        else
            NODE_RETURN(0);
    }

} // namespace Mu

#endif
