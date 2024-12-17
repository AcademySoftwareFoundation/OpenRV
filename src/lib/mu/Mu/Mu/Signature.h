#ifndef __Mu__Signature__h__
#define __Mu__Signature__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
#include <Mu/HashTable.h>
#include <Mu/Type.h>
#include <vector>

namespace Mu
{
    class Signature;
    class Context;

    //----------------------------------------------------------------------
    //
    //  class Signature
    //
    //  This class is produced after functions are resolved.
    //

    class Signature /*: public Type*/
    {
    public:
        MU_GC_STUBBORN_NEW_DELETE

        //
        //  Types
        //

        typedef Symbol::SymbolRefList Types;
        typedef Symbol::SymbolRef SymbolRef;

        //
        //  Constructor / Destructor
        //

        Signature()
            : _resolved(false)
            , _typesIn(false)
        {
        }

        // Signature(const char*);
        ~Signature();

        bool operator==(const Signature& other) const;

        size_t size() const { return _types.size(); }

        void push_back(const Type* t);
        void push_back(Name);

        const Types& types() const { return _types; }

        SymbolRef& operator[](size_t i) { return _types[i]; }

        const SymbolRef& operator[](size_t i) const { return _types[i]; }

        bool resolved() const { return _resolved; }

        void resolve(const Context*) const;

        String functionTypeName() const;

        const Type* argType(size_t i) const
        {
            return _types[i + 1].symbolOfType<Type>();
        }

        const Type* returnType() const
        {
            return _types[0].symbolOfType<Type>();
        }

    private:
        mutable Types _types;
        mutable bool _resolved : 1;
        bool _typesIn : 1;
    };

    struct SignatureTraits
    {
        static bool equals(const Signature* a, const Signature* b)
        {
            return *a == *b;
        }

        static unsigned long hash(const Signature*);
    };

    typedef HashTable<Signature*, SignatureTraits> SignatureHashTable;

} // namespace Mu

#endif // __Mu__Signature__h__
