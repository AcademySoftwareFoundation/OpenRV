//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/StructType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Function.h>
#include <Mu/MemberVariable.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <stdio.h>

namespace Mu
{
    using namespace std;

    StructType::StructType(Context* context, const char* name,
                           const NameValuePairs& fields)
        : Class(context, name)
        , _fields(fields)
    {
    }

    StructType::~StructType() {}

    void StructType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;
        STLVector<ParameterVariable*>::Type params;
        Context* c = context();
        params.push_back(new ParameterVariable(c, "this", this));

        for (int i = 0; i < _fields.size(); i++)
        {
            const char* name = _fields[i].first.c_str();
            const Type* type = _fields[i].second;

            addSymbol(new MemberVariable(c, name, type));
            params.push_back(new ParameterVariable(c, name, type, Value()));
        }

        String rname = name();
        rname += "&";
        const char* n = name().c_str();
        const char* fn = fullyQualifiedName().c_str();
        const char* rn = rname.c_str();

        Symbol* g = scope();

        string qrname = g->fullyQualifiedName();
        qrname += ".";
        qrname += rn;
        const char* frn = qrname.c_str();

        g->addSymbol(new ReferenceType(c, rn, this));

        g->addSymbols(new Function(c, n, BaseFunctions::dereference, Cast,
                                   Return, fn, Args, frn, End),

                      new Function(c, "=", BaseFunctions::assign, AsOp, Return,
                                   frn, Args, frn, fn, End),

                      EndArguments);

        addSymbols(new Function(c, "__allocate", defaultConstructor, None,
                                Return, fn, End),

                   new Function(c, n, this, params.size(), &params.front(),
                                aggregateConstructor, Mapped),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(StructType::defaultConstructor, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const StructType* c = static_cast<const StructType*>(NODE_THIS.type());
        ClassInstance* o = ClassInstance::allocate(c);
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(StructType::aggregateConstructor, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const StructType* c = static_cast<const StructType*>(NODE_THIS.type());
        ClassInstance* o = NODE_ARG_OBJECT(0, ClassInstance);
        const Node* n = 0;

        for (size_t i = 1; n = NODE_THIS.argNode(i); i++)
        {
            o->fieldType(i - 1)->nodeEval(o->field(i - 1), n, NODE_THREAD);
        }

        NODE_RETURN(Pointer(o));
    }

} // namespace Mu
