//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/SymbolicConstant.h>
#include <Mu/Context.h>
#include <Mu/Type.h>

namespace Mu
{
    using namespace std;

    SymbolicConstant::SymbolicConstant(Context* context, const char* name,
                                       const Type* type, const Value& v)
        : Symbol(context, name)
        , _value(v)
    {
        _type.symbol = type;
        _state = ResolvedState;
    }

    SymbolicConstant::SymbolicConstant(Context* context, const char* name,
                                       const char* type, const Value& v)
        : Symbol(context, name)
        , _value(v)
    {
        _type.name = context->internName(type).nameRef();
    }

    SymbolicConstant::~SymbolicConstant() {}

    const Type* SymbolicConstant::nodeReturnType(const Node*) const
    {
        if (!isResolved())
            resolve();
        return type();
    }

    void SymbolicConstant::outputNode(std::ostream& o, const Node*) const
    {
        output(o);
    }

    void SymbolicConstant::output(std::ostream& o) const
    {
        Symbol::output(o);

        o << " = " << type()->fullyQualifiedName() << " ";

        type()->outputValue(o, _value);
    }

    bool SymbolicConstant::resolveSymbols() const
    {
        if (const Type* s =
                globalScope()->findSymbolOfType<Type>(Name(_type.name)))
        {
            _type.symbol = s;
            return true;
        }
        else
        {
            return false;
        }
    }

    const Type* SymbolicConstant::type() const
    {
        if (!isResolved())
            resolve();
        return static_cast<const Type*>(_type.symbol);
    }

} // namespace Mu
