//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <mu_test/TestModule.h>
#include <mu_test/SequenceInterface.h>
#include <mu_test/BaseType.h>
#include <mu_test/AClassType.h>
#include <mu_test/BClassType.h>
#include <mu_test/CClassType.h>
#include <mu_test/BarInterface.h>
#include <Mu/Function.h>
#include <Mu/Exception.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/ParameterVariable.h>
#include <MuLang/StringType.h>
#include <MuLang/MuLangContext.h>
#include <sstream>
#include <iostream>
#include <stdlib.h>

namespace Mu
{
    using namespace std;

    TestModule::TestModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    TestModule::~TestModule() {}

    void TestModule::load()
    {
        Function::ArgKeyword Return = Function::Return;
        Function::ArgKeyword Args = Function::Args;
        Function::ArgKeyword Optional = Function::Optional;
        Function::ArgKeyword End = Function::End;
        Function::ArgKeyword Parameters = Function::Parameters;

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

        MuLangContext* context = (MuLangContext*)globalModule()->context();

        typedef ParameterVariable Param;

        Class* base = new BaseType(context);
        Class* a = new AClassType(context, base);
        Class* b = new BClassType(context, base);
        Class* c = new CClassType(context, base);

        addSymbols(new SequenceInterface(context), new BarInterface(context),
                   base, a, b, c,

                   EndArguments);
    }

} // namespace Mu
