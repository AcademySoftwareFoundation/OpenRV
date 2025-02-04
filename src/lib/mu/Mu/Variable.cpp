//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ReferenceType.h>
#include <Mu/Context.h>
#include <Mu/Type.h>
#include <Mu/Variable.h>
#include <iostream>

namespace Mu
{

    using namespace std;

    Variable::Variable(Context* context, const char* name,
                       const Type* storageClass, int address,
                       Variable::Attributes attributes)
        : Symbol(context, name)
    {
        _type = storageClass;
        _state = ResolvedState;
        _address = address;
        _initialized = 0;
        init(attributes);
        resolveSymbols();
    }

    Variable::Variable(Context* context, const char* name,
                       const char* storageClass, int address,
                       Variable::Attributes attributes)
        : Symbol(context, name)
    {
        _type = SymbolRef(context->internName(storageClass));
        _state = UntouchedState;
        _address = address;
        _initialized = 0;
        init(attributes);
    }

    Variable::~Variable() {}

    void Variable::init(Attributes attributes)
    {
        _readable = attributes & Readable;
        _writable = attributes & Writable;
        _singleUse = attributes & SingleUse;
        _singleAssign = attributes & SingleAssign;
        _implicitType = attributes & ImplicitType;
    }

    Variable::Attributes Variable::attributes() const
    {
        return (_readable ? Readable : NoVariableAttr)
               & (_writable ? Writable : NoVariableAttr)
               & (_singleAssign ? SingleAssign : NoVariableAttr)
               & (_implicitType ? ImplicitType : NoVariableAttr)
               & (_singleUse ? SingleUse : NoVariableAttr);
    }

    const Type* Variable::storageClass() const
    {
        if (!isResolved())
            resolve();

        if (symbolState() == ResolvedState)
        {
            return static_cast<const Type*>(_type.symbol);
        }
        else
        {
            return 0;
        }
    }

    Name Variable::storageClassName() const
    {
        if (isResolved())
            return storageClass()->fullyQualifiedName();
        else
            return _type.name;
    }

    void Variable::setStorageClass(const Type* t) { _type.symbol = t; }

    void Variable::output(ostream& o) const
    {
        Symbol::output(o);
        o << " (" << storageClass()->fullyQualifiedName() << ")";
    }

    bool Variable::resolveSymbols() const
    {
        if (isResolved())
            return true;
        Name n = _type.name;

        Symbol::ConstSymbolVector symbols =
            globalScope()->findSymbolsOfType<Type>(n);

        if (symbols.size() == 1)
        {
            _type.symbol = symbols.front();
            return true;
        }
        else
        {
            // cerr << "Variable::resolveSymbols() cant find "
            //"type " << n << endl;
            return false;
        }
    }

    const Type* Variable::nodeReturnType(const Node*) const
    {
        if (!isResolved())
            resolve();
        const Type* t = static_cast<const Type*>(_type.symbol);

        if (t && t->referenceType())
            return t->referenceType();
        else
            return t;
    }

    const Function* Variable::referenceFunction() const { return 0; }

    const Function* Variable::extractFunction() const { return 0; }

    void Variable::symbolDependancies(ConstSymbolVector& symbols) const
    {
        if (symbolState() != ResolvedState)
            resolveSymbols();
        symbols.push_back(storageClass());
    }

} // namespace Mu
