//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/MemberFunction.h>
#include <Mu/Class.h>

namespace Mu
{
    using namespace std;

    MemberFunction::MemberFunction(Context* context, const char* name)
        : Function(context, name)
        , _offset(0)
        , _destructor(false)
    {
        _method = true;
    }

    MemberFunction::MemberFunction(Context* context, const char* name,
                                   NodeFunc func,
                                   Function::Attributes attributes, ...)
        : Function(context, name)
        , _offset(0)
    {
        _method = true;
        _destructor = attributes & Destructor;
        va_list ap;
        va_start(ap, attributes);
        init(func, attributes, ap);
        va_end(ap);
    }

    MemberFunction::MemberFunction(Context* context, const char* name,
                                   const Type* returnType, int nparams,
                                   ParameterVariable** v, Node* n,
                                   Attributes attributes)
        : Function(context, name, returnType, nparams, v, n, attributes)
        , _offset(0)
    {
        _method = true;
        _destructor = attributes & Destructor;
    }

    MemberFunction::MemberFunction(Context* context, const char* name,
                                   const Type* returnType, int nparams,
                                   ParameterVariable** v, NodeFunc f,
                                   Attributes attributes)
        : Function(context, name, returnType, nparams, v, f, attributes)
        , _offset(0)
    {
        _method = true;
        _destructor = attributes & Destructor;
    }

    MemberFunction::~MemberFunction() {}

    void MemberFunction::output(std::ostream& o) const
    {
        Function::output(o);
        o << " (member)";
    }

    bool MemberFunction::isConstructor() const
    {
        return name() == scope()->name();
    }

    Class* MemberFunction::memberClass()
    {
        return dynamic_cast<Class*>(scope());
    }

    const Class* MemberFunction::memberClass() const
    {
        return dynamic_cast<const Class*>(scope());
    }

    bool MemberFunction::matches(const Function* f) const
    {
        if (name() == f->name())
        {
            int n = numArgs();
            if (n == f->numArgs())
            {
                if (returnTypeName() == f->returnTypeName())
                {
                    for (int i = 1; i < n; i++)
                    {
                        if (argTypeName(i) != f->argTypeName(i))
                            return false;
                    }

                    return true;
                }
            }
        }

        return false;
    }

    void
    MemberFunction::findOverridingFunctions(MemberFunctionVector& funcs) const
    {
        const Class* c = memberClass();
        c->findOverridingFunctions(this, funcs);
    }

} // namespace Mu
