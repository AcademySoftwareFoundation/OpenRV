//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Alias.h>
#include <Mu/Context.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    Alias::Alias(Context* context, const char* name, Symbol* symbol)
        : Symbol(context, name)
    {
        _symbol.symbol = symbol;
        _state = ResolvedState;
    }

    Alias::Alias(Context* context, const char* name, const char* symbol)
        : Symbol(context, name)
    {
        _symbol.name = context->internName(symbol).nameRef();
    }

    Alias::~Alias() {}

    const Type* Alias::nodeReturnType(const Node*) const { return 0; }

    void Alias::outputNode(std::ostream& o, const Node*) const { output(o); }

    void Alias::output(std::ostream& o) const
    {
        o << fullyQualifiedName() << " -> " << alias()->fullyQualifiedName();
    }

    bool Alias::resolveSymbols() const
    {
        if (const Symbol* s =
                globalScope()->findSymbolByQualifiedName(Name(_symbol.name)))
        {
            _symbol.symbol = s;
            return true;
        }
        else
        {
            return false;
        }
    }

    const Symbol* Alias::alias() const
    {
        if (!isResolved())
            resolve();
        return _symbol.symbol;
    }

    Symbol* Alias::alias()
    {
        if (!isResolved())
            resolve();
        return const_cast<Symbol*>(_symbol.symbol);
    }

    void Alias::set(const Symbol* s)
    {
        _symbol.symbol = s;
        _state = ResolvedState;
    }

    void Alias::symbolDependancies(ConstSymbolVector& symbols)
    {
        if (symbolState() != ResolvedState)
            resolveSymbols();
        symbols.push_back(alias());
    }

} // namespace Mu
