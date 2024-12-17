//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <mu_test/BarInterface.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/Node.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <MuLang/StringType.h>

namespace Mu
{

    using namespace std;

    BarInterface::BarInterface(Context* c)
        : Interface(c, "bar_interface")
    {
    }

    BarInterface::~BarInterface() {}

    void BarInterface::load()
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

        Symbol* s = scope();
        Context* c = context();

        const char* tn = "test.bar_interface";
        const char* rn = "test.bar_interface&";

        s->addSymbols(new ReferenceType(c, "bar_interface&", this),

                      new Function(c, "bar_interface",
                                   BaseFunctions::dereference, Cast, Return, tn,
                                   Args, rn, End),

                      EndArguments);

        addSymbols(new MemberFunction(c, "bar", BarInterface::bar, None, Return,
                                      "string", Args, tn, End),

                   new MemberFunction(c, "foo", BarInterface::foo, None, Return,
                                      "string", Args, tn, End),

                   EndArguments);

        globalScope()->addSymbols(

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(BarInterface::bar, Pointer)
    {
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);
        Process* p = NODE_THREAD.process();
        const StringType* c = static_cast<const StringType*>(NODE_THIS.type());
        NODE_RETURN(c->allocate("bar_interface.bar (default implementation)"));
    }

    NODE_IMPLEMENTATION(BarInterface::foo, Pointer)
    {
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);
        Process* p = NODE_THREAD.process();
        const StringType* c = static_cast<const StringType*>(NODE_THIS.type());
        NODE_RETURN(c->allocate("bar_interface.foo (default implementation)"));
    }

} // namespace Mu
