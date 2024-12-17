//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Context.h>
#include <Mu/Signature.h>
#include <Mu/Type.h>
#include <Mu/Exception.h>
#include <algorithm>
#include <iostream>

namespace Mu
{
    using namespace std;
    using namespace stl_ext;

    unsigned long SignatureTraits::hash(const Signature* sig)
    {
        unsigned long l = 0;

        for (int i = 0, s = sig->size(); i < s; i++)
        {
            l ^= (*sig)[i].symbol->name().hash();
        }

        return l;
    }

    //----------------------------------------------------------------------

    // Signature::Signature(const char* s) : _resolved(false), _typesIn(false)
    // {
    //     vector<string> tokens;
    //     tokenize(tokens, s, "(,;) \t\r\n");

    //     for (int i=0; i < tokens.size(); i++)
    //     {
    //         if (Name n = context()->lookupName(tokens[i].c_str()))
    //         {
    //             push_back(n);
    //         }
    //         else
    //         {
    //             throw InconsistantSignatureException();
    //         }
    //     }
    // }

    Signature::~Signature()
    {
        _types.clear();
        _resolved = false;
        _typesIn = true;
    }

    bool Signature::operator==(const Signature& other) const
    {
        if (_types.size() != other._types.size())
            return false;
        for (int i = 0, s = _types.size(); i < s; i++)
        {
            if (_types[i].symbol != other._types[i].symbol)
                return false;
        }

        return true;
    }

    void Signature::push_back(const Type* t)
    {
        if (_types.size() && !_typesIn)
        {
            _types.push_back(t->fullyQualifiedName());
        }
        else
        {
            _typesIn = true;
            _resolved = true;
            _types.push_back(t);
        }
    }

    void Signature::push_back(Name n)
    {
        if (_types.size() && _typesIn)
        {
            throw InconsistantSignatureException();
        }
        else
        {
            _typesIn = false;
            _types.push_back(n);
        }
    }

    void Signature::resolve(const Context* context) const
    {
        if (resolved())
            return;

        const Symbol* global = context->globalScope();
        STLVector<const Symbol*>::Type types(_types.size());

        for (int i = 0; i < _types.size(); i++)
        {
            if (Name(_types[i].name) == "")
                return;

            Symbol::ConstSymbolVector symbols =
                global->findSymbolsOfType<Type>(_types[i].name);

            if (symbols.size() == 1)
            {
                types[i] = symbols.front();
            }
            else if (symbols.size() > 1)
            {
                throw AmbiguousSymbolNameException();
            }
            else
            {
#if 0
            cerr << "ERROR: unable to resolve "
                 << Name(_types[i].name) 
                 << " for arg " << i
                 << endl;
#endif

                // cout << "signature = (" << Name(_types[i].name) << "; ";

                // for (int i=1; i < _types.size(); i++)
                // {
                //     if (i > 1) cout << ", ";
                //     cout << Name(_types[i].name);
                // }

                // cout << ")" << endl;

                return;
            }
        }

        for (int i = 0; i < types.size(); i++)
            _types[i].symbol = types[i];
        const_cast<Signature*>(this)->_resolved = true;
        MU_GC_END_CHANGE_STUBBORN(this);
    }

    String Signature::functionTypeName() const
    {
        if (!resolved())
            return Name();

        String name = "(";
        name += types().front().symbol->fullyQualifiedName().c_str();
        name += ";";

        for (int i = 1; i < types().size(); i++)
        {
            if (i > 1)
                name += ",";
            name += types()[i].symbol->fullyQualifiedName().c_str();
        }

        name += ")";

        return name;
    }

    //----------------------------------------------------------------------

} // namespace Mu
