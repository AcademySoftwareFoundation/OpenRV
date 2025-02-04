//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <mu_test/BClassType.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <Mu/MemberVariable.h>
#include <Mu/BaseFunctions.h>
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

    BClassType::BClassType(Context* c, Class* super)
        : Class(c, "b_class", super)
    {
    }

    BClassType::~BClassType() {}

    void BClassType::load()
    {
        Function::ArgKeyword Return = Function::Return;
        Function::ArgKeyword Args = Function::Args;
        Function::ArgKeyword Optional = Function::Optional;
        Function::ArgKeyword End = Function::End;

        Function::Attributes None = Function::None;
        Function::Attributes CommOp = Function::Mapped | Function::Commutative
                                      | Function::Operator
                                      | Function::NoSideEffects;
        Function::Attributes Op =
            Function::Mapped | Function::Operator | Function::NoSideEffects;
        Function::Attributes Mapped =
            Function::Mapped | Function::NoSideEffects;
        Function::Attributes Cast = Mapped | Function::Cast;
        Function::Attributes Lossy = Cast | Function::Lossy;
        Function::Attributes AsOp =
            Function::MemberOperator | Function::Operator;
        Function::ArgKeyword Parameters = Function::Parameters;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        String tname = s->name();
        tname += ".";
        tname += "b_class";

        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        s->addSymbols(new ReferenceType(c, "b_class&", this),

                      new Function(c, "b_class", BClassType::construct, None,
                                   Return, tn, Args, "string", End),

                      new Function(c, "b_class", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "print", BClassType::print, None, Return, "void",
                         Args, tn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);

        typedef ParameterVariable Param;

        addSymbols(new MemberVariable(c, "value", "float"),
                   new MemberVariable(c, "ch", "char"),

                   new MemberFunction(c, "foo", BClassType::foo, None, Return,
                                      "string", Args, tn, End),

                   new MemberFunction(c, "bar", BClassType::bar, None, Return,
                                      "string", Args, tn, End),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(BClassType::construct, Pointer)
    {
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        if (!name)
            throw NilArgumentException();

        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const BClassType*>(NODE_THIS.type());
        ClassInstance* o = ClassInstance::allocate(c);

        Layout* x = reinterpret_cast<Layout*>(o->structure());
        x->name = name;

        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(BClassType::print, void)
    {
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);
        i->type()->outputValue(cout, Value(i));
    }

    NODE_IMPLEMENTATION(BClassType::foo, Pointer)
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

        str << "b_class.foo";
        NODE_RETURN(c->stringType()->allocate(str));
    }

    NODE_IMPLEMENTATION(BClassType::bar, Pointer)
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

        str << "b_class.bar";
        NODE_RETURN(c->stringType()->allocate(str));
    }

} // namespace Mu
