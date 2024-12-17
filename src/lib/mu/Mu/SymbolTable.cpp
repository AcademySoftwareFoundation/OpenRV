//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/SymbolTable.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    SymbolTable::RecursiveIterator::RecursiveIterator(SymbolTable* t)
    {
        _itstack.push_back(SymbolTable::Iterator(t));
        fillLeaf();
    }

    void SymbolTable::RecursiveIterator::fillLeaf()
    {
        while (1)
        {
            const Symbol* s = *_itstack.back();

            if (SymbolTable* st = s->symbolTable())
            {
                _itstack.push_back(SymbolTable::Iterator(st));
            }
            else
            {
                break;
            }
        }
    }

    void SymbolTable::RecursiveIterator::operator++()
    {
        if (!_itstack.empty())
        {
            ++_itstack.back();

            if (!_itstack.empty())
            {
                if (!_itstack.back())
                    _itstack.pop_back();
                else
                    fillLeaf();
            }
        }
    }

    static void symboltable_finalizer(void* obj, void* data)
    {
        // cout << "FINALIZER: " << obj << endl;
    }

    SymbolTable::SymbolTable()
    {
        // GC_finalization_proc ofn;
        // void *odata;
        // GC_register_finalizer(this, symboltable_finalizer, 0, &ofn, &odata);
    }

    SymbolTable::~SymbolTable() {}

    void SymbolTable::add(Symbol* s)
    {
        if (const Item* i = _hashTable.find(s))
        {
            i->data()->appendOverload(s);
        }
        else
        {
            _hashTable.add(s);
        }
    }

    bool SymbolTable::exists(Symbol* s) const
    {
        return _hashTable.find(s) ? true : false;
    }

    // Symbol*
    // SymbolTable::find(const String& s) const
    // {
    //     if (Name n = Symbol::namePool().find(s))
    // 	return find(n);
    //     else
    // 	return 0;
    // }

    Symbol* SymbolTable::find(const Name& name) const
    {
        for (const Item* i = _hashTable.firstItemOf(name.hash()); i;
             i = i->next())
            if (i->data()->name() == name)
                return i->data();
        return 0;
    }

} // namespace Mu
