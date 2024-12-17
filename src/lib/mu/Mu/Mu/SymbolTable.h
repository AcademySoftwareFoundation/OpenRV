#ifndef __Mu__SymbolTable__h__
#define __Mu__SymbolTable__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/config.h>
#include <Mu/StringHashTable.h>
#include <Mu/Symbol.h>

namespace Mu
{

    //
    //  class SymbolTable
    //
    //  Manages symbols for a scope. Uses HashTable<> to store them.
    //

    class SymbolTable
    {
        struct SymbolTraits
        {
            static bool equals(const Symbol* a, const Symbol* b)
            {
                return a->name() == b->name();
            }

            static unsigned long hash(const Symbol* s)
            {
                return (s->name().hash());
            }
        };

    public:
        MU_GC_NEW_DELETE
        typedef HashTable<Symbol*, SymbolTraits> SymbolHashTable;
        typedef SymbolHashTable::Item Item;

        class Iterator
        {
        public:
            Iterator(SymbolTable* t)
                : _i(t->hashTable())
            {
            }

            const Symbol* operator*() const { return *_i; }

            operator bool() const { return _i; }

            void operator++() { ++_i; }

        private:
            SymbolTable::SymbolHashTable::Iterator _i;
        };

        class RecursiveIterator
        {
        public:
            typedef STLVector<SymbolTable::Iterator>::Type ITVector;

            RecursiveIterator(SymbolTable* t);

            const Symbol* operator*() const { return *_itstack.back(); }

            operator bool() const
            {
                return _itstack.empty() ? false : bool(_itstack.back());
            }

            void operator++();

        private:
            void fillLeaf();

        private:
            ITVector _itstack;
        };

        SymbolTable();
        ~SymbolTable();

        //
        //	Adds a symbol to the symbol table.  If there is already an
        //	existing symbol with the same name, then the symbol is added
        //	as an overloaded occurance.
        //

        void add(Symbol*);

        //
        //	These functions let you search by symbol name.
        //

        // Symbol*		    find(const String&) const;
        Symbol* find(const Name&) const;
        bool exists(Symbol*) const;

        SymbolHashTable& hashTable() { return _hashTable; }

    private:
    private:
        SymbolHashTable _hashTable;
    };

} // namespace Mu

#endif // __Mu__SymbolTable__h__
