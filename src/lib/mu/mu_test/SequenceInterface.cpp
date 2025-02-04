//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <mu_test/SequenceInterface.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/ReferenceType.h>

namespace Mu
{

    using namespace std;

    SequenceInterface::SequenceInterface(Context* c)
        : Interface(c, "sequence")
    {
    }

    SequenceInterface::~SequenceInterface() {}

    void SequenceInterface::load()
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

        const char* tn = "test.sequence";
        const char* rn = "test.sequence&";

        s->addSymbols(new ReferenceType(c, "sequence&", this),

                      new Function(c, "sequence", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        addSymbols(new MemberFunction(c, "clear", NodeFunc(0), None, Return,
                                      "void", Args, tn, End),

                   new MemberFunction(c, "push_back", NodeFunc(0), None, Return,
                                      "int", Args, tn, "int", End),

                   new MemberFunction(c, "pop_back", NodeFunc(0), None, Return,
                                      "int", Args, tn, End),

                   EndArguments);

        globalScope()->addSymbols(

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);
    }

} // namespace Mu
