//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/ObjectInterface.h>
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

    ObjectInterface::ObjectInterface(Context* c)
        : Interface(c, "object")
    {
    }

    ObjectInterface::~ObjectInterface() {}

    void ObjectInterface::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        const char* tn = "object";
        const char* rn = "object&";

        s->addSymbols(new ReferenceType(c, "object&", this),

                      new Function(c, "object", BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        addSymbols(new MemberFunction(c, "identity", ObjectInterface::identity,
                                      None, Return, tn, Args, tn, End),

                   EndArguments);

        globalScope()->addSymbols(

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

#if 0
                   new Function(c, "eq", BaseFunctions::eq, CommOp,
                                Return, "bool",
                                Args, tn, tn, End),
#endif

            EndArguments);
    }

    NODE_IMPLEMENTATION(ObjectInterface::identity, Pointer)
    {
        NODE_RETURN((Pointer)NODE_ARG_OBJECT(0, Object));
    }

} // namespace Mu
