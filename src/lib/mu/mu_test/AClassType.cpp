//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <mu_test/AClassType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/DynamicArray.h>
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

    //----------------------------------------------------------------------

    AClassType::AClassType(Context* c, Class* super)
        : Class(c, "a_class", super)
    {
    }

    AClassType::~AClassType() {}

    void AClassType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        String tname = s->name();
        tname += ".";
        tname += "a_class";

        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        s->addSymbols(new ReferenceType(c, "a_class&", this),

                      new Function(c, "a_class", AClassType::construct, None,
                                   Return, tn, Args, "string", End),

                      new Function(c, "a_class", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "print", AClassType::print, None, Return, "void",
                         Args, tn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);

        typedef ParameterVariable Param;

        addSymbols(new MemberVariable(c, "value", "int"),

                   new MemberFunction(c, "foo", AClassType::foo, None, Return,
                                      "string", Args, tn, End),

                   new MemberFunction(c, "bar", AClassType::bar, None, Return,
                                      "string", Args, tn, End),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(AClassType::construct, Pointer)
    {
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        if (!name)
            throw NilArgumentException();

        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const AClassType*>(NODE_THIS.type());
        ClassInstance* o = ClassInstance::allocate(c);

        Layout* x = reinterpret_cast<Layout*>(o->structure());
        x->name = name;

        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(AClassType::print, void)
    {
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);
        i->type()->outputValue(cout, Value(i));
    }

    NODE_IMPLEMENTATION(AClassType::foo, Pointer)
    {
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);
        Layout* x = reinterpret_cast<Layout*>(i->structure());
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        ostringstream str;

        if (x->name)
        {
            str << x->name->c_str();
            str << "->";
        }

        str << "a_class.foo";
        NODE_RETURN(c->stringType()->allocate(str));
    }

    NODE_IMPLEMENTATION(AClassType::bar, Pointer)
    {
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);
        Layout* x = reinterpret_cast<Layout*>(i->structure());
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        ostringstream str;

        if (x->name)
        {
            str << x->name->c_str();
            str << "->";
        }

        str << "a_class.bar";
        NODE_RETURN(c->stringType()->allocate(str));
    }

} // namespace Mu
