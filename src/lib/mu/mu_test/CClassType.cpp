//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <mu_test/CClassType.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <Mu/BaseFunctions.h>
#include <Mu/MemberVariable.h>
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

    CClassType::CClassType(Context* c, Class* super)
        : Class(c, "c_class", super)
    {
    }

    CClassType::~CClassType() {}

    void CClassType::load()
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
        tname += "c_class";

        String aname = tname + "[]";
        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();
        const char* an = aname.c_str();

        s->addSymbols(new ReferenceType(c, "c_class&", this),

                      new Function(c, "c_class", CClassType::construct, None,
                                   Return, tn, Args, "string", End),

                      new Function(c, "c_class", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "print", CClassType::print, None, Return, "void",
                         Args, tn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);

        typedef ParameterVariable Param;

        context->arrayType(this, 1, 0);

        addSymbols(new MemberVariable(c, "parent", tn),
                   new MemberVariable(c, "children", an),

                   new MemberFunction(c, "bar", CClassType::bar, None, Return,
                                      "string", Args, tn, End),

                   new MemberFunction(c, "baz", CClassType::baz, None, Return,
                                      "string", Args, tn, End),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(CClassType::construct, Pointer)
    {
        StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
        if (!name)
            throw NilArgumentException();

        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const Class* type = static_cast<const CClassType*>(NODE_THIS.type());
        ClassInstance* o = ClassInstance::allocate(type);

        Layout* x = reinterpret_cast<Layout*>(o->structure());
        x->name = name;
        x->children =
            new DynamicArray((DynamicArrayType*)c->arrayType(type, 1, 0), 1);

        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(CClassType::print, void)
    {
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);
        i->type()->outputValue(cout, Value(i));
    }

    NODE_IMPLEMENTATION(CClassType::baz, Pointer)
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

        str << "c_class.baz";
        NODE_RETURN(c->stringType()->allocate(str));
    }

    NODE_IMPLEMENTATION(CClassType::bar, Pointer)
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

        str << "c_class.bar";
        NODE_RETURN(c->stringType()->allocate(str));
    }

} // namespace Mu
