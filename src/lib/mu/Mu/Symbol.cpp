//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Symbol.h>
#include <Mu/Context.h>
#include <Mu/Node.h>
#include <Mu/SymbolTable.h>
#include <Mu/Module.h>
#include <Mu/UTF8.h>
#include <iostream>
#include <typeinfo>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace Mu
{
    using namespace std;

    static void symbol_finalizer(void* obj, void* data)
    {
        Symbol* s = (Symbol*)obj;
    }

    Symbol::Symbol(Context* context, const char* name)
        : _context(context)
    {
        init(context->internName(name));
        context->symbolConstructed(this);
    }

    void Symbol::init(Name name)
    {
        _name = name;
        _state = UntouchedState;
        _searchable = false;
        _scope = 0;
        _symbolTable = 0;
        _resolving = false;
        _overload = 0;
        _primary = context()->primaryState();
        _datanode = false;
    }

    Symbol::~Symbol()
    {
        _context->symbolDestroyed(this);
        delete _symbolTable;
    }

    bool Symbol::interned() const { return _scope ? true : false; }

    void Symbol::load()
    {
        // nothing
    }

    QualifiedName Symbol::fullyQualifiedName() const
    {
        if (!isResolved())
            resolve();
        if (!scope())
            return name();
        String qname;

        const Symbol* g = globalScope();

        for (const Symbol* s = this; s && s->scope(); s = s->scope())
        {
            qname.insert(0, s->name().c_str());
            if (s->scope() != g)
                qname.insert(0, ".");
        }

        return context()->internName(qname);
    }

    String Symbol::mangledName() const
    {
        if (!scope())
            return Context::encodeName(name());
        String qname;

        const Symbol* g = globalScope();

        if (g == this)
            return "";

        if (scope())
        {
            if (scope() != g)
            {
                qname = scope()->mangledName();
                qname += "_";
            }

            qname += context()->encodeName(name());
        }

        return qname;
    }

    String Symbol::mangledId() const
    {
        char temp[80];
        snprintf(temp, 80, "s%zx", size_t(this) >> 4);
        return String(temp);
    }

    void Symbol::appendOverload(Symbol* symbol)
    {
        bool resolved = isResolved();
        if (resolved)
            MU_GC_BEGIN_CHANGE_STUBBORN(this);
        if (_overload)
            symbol->_overload = _overload;
        _overload = symbol;
        if (resolved)
            MU_GC_END_CHANGE_STUBBORN(this);
    }

    void Symbol::output(std::ostream& o) const { o << fullyQualifiedName(); }

    void Symbol::outputNode(std::ostream& o, const Node* n) const { output(o); }

    std::ostream& operator<<(std::ostream& o, const Symbol& symbol)
    {
        symbol.output(o);
        return o;
    }

    bool Symbol::resolveSymbols() const { return true; }

    const Type* Symbol::nodeReturnType(const Node*) const { return 0; }

    void Symbol::addSymbol(Symbol* symbol)
    {
        bool resolved = isResolved();
        if (resolved)
            MU_GC_BEGIN_CHANGE_STUBBORN(this);
        if (!_symbolTable)
            _symbolTable = new SymbolTable();
        _symbolTable->add(symbol);

        assert(!symbol->_scope);
        symbol->_scope = this;
        symbol->load();
        if (resolved)
            MU_GC_END_CHANGE_STUBBORN(this);
    }

    void Symbol::addSymbols(Symbol* symbol, ...)
    {
        if (!symbol)
            return;

        va_list ap;
        va_start(ap, symbol);
        Symbol* s;

        addSymbol(symbol);

        while (s = va_arg(ap, Symbol*))
            addSymbol(s);
        va_end(ap);
    }

    void Symbol::addAnonymousSymbol(Symbol* symbol)
    {
#if 0
    bool resolved = isResolved();
    if (resolved) MU_GC_BEGIN_CHANGE_STUBBORN(this);
    assert(!symbol->_scope);
    symbol->_scope = this;
    symbol->load();
    if (resolved) MU_GC_END_CHANGE_STUBBORN(this);
#endif
        addSymbol(symbol);
    }

    Symbol* Symbol::findSymbolByQualifiedName(Name name, bool restricted)
    {
        const Symbol* s =
            const_cast<const Symbol*>(this)->findSymbolByQualifiedName(
                name, restricted);
        return const_cast<Symbol*>(s);
    }

    static size_t findSeparator(const String& s)
    {
        for (size_t i = 0, p = 0; i < s.size(); i++)
        {
            switch (s[i])
            {
            case '(':
                p++;
                break;
            case ')':
                p--;
                break;
            case '[':
                p++;
                break;
            case ']':
                p--;
                break;
            case '.':
                if (p == 0)
                    return i;
            }
        }

        return String::npos;
    }

    const Symbol* Symbol::findSymbolByQualifiedName(Name name,
                                                    bool restricted) const
    {
        size_t p;
        const String& sname = name;

        //     const Symbol* s = context()->globalScope()->findSymbol(name);

        //     if (s && s->scope() == context()->globalScope())
        //     {
        //         return s;
        //     }

        if ((p = findSeparator(sname)) != String::npos)
        {
            Mu::String first = sname.substr(0, p);
            Mu::String rest = sname.substr(p + 1);

            if (Name name0 = context()->lookupName(first.c_str()))
            {
                if (const Symbol* sym = findSymbol(name0))
                {
                    if (rest != "")
                    {
                        Name n = context()->internName(rest.c_str());

                        for (const Symbol* s = sym->firstOverload(); s;
                             s = s->nextOverload())
                        {
                            if (!restricted || s->_searchable)
                            {
                                if (const Symbol* child =
                                        s->findSymbolByQualifiedName(
                                            n, restricted))
                                {
                                    return child;
                                }
                            }
                        }
                    }
                    else
                    {
                        return sym;
                    }
                }
            }
        }
        else
        {
            return findSymbol(name);
        }

        return 0;
    }

    const Symbol* Symbol::findSymbol(Name name) const
    {
        if (symbolTable())
        {
            if (Symbol* symbol = symbolTable()->find(name))
            {
#if 0
            if (symbol->symbolState() != Symbol::ResolvedState)
            {
                symbol->resolve();
            }
#endif

                return symbol;
            }
        }

        return 0;
    }

    Symbol* Symbol::findSymbol(Name name)
    {
        const Symbol* s = const_cast<const Symbol*>(this)->findSymbol(name);
        return const_cast<Symbol*>(s);
    }

    static void qualifiedNameLookup(const Symbol::NameVector& name,
                                    Symbol* base, Symbol::SymbolVector& symbols)
    {
        if (Symbol* s = base->findSymbol(name[0]))
        {
            if (name.size() == 1)
            {
                for (Symbol* n = s->firstOverload(); n; n = n->nextOverload())
                {
                    symbols.push_back(n);
                }
            }
            else
            {
                Symbol::NameVector newName = name;
                newName.erase(newName.begin());

                for (Symbol* n = s->firstOverload(); n; n = n->nextOverload())
                {
                    qualifiedNameLookup(newName, n, symbols);
                }
            }
        }
    }

    static void qualifiedNameLookup(const Symbol::NameVector& name,
                                    const Symbol* base,
                                    Symbol::ConstSymbolVector& symbols)
    {
        if (const Symbol* s = base->findSymbol(name[0]))
        {
            if (name.size() == 1)
            {
                for (const Symbol* n = s->firstOverload(); n;
                     n = n->nextOverload())
                {
                    symbols.push_back(n);
                }
            }
            else
            {
                Symbol::NameVector newName = name;
                newName.erase(newName.begin());

                for (const Symbol* n = s->firstOverload(); n;
                     n = n->nextOverload())
                {
                    qualifiedNameLookup(newName, n, symbols);
                }
            }
        }
    }

    void Symbol::findSymbols(QualifiedName name, SymbolVector& symbols)
    {
        NameVector names;
        context()->separateName(name, names);

        //
        //  NOTE: this is all very fishy. If a tuple symbol is used here
        //  that looks like this: (int,foo.bar), it will separate into:
        //  "(int,foo" and "bar)" which is clearly wrong. The parens
        //  should prevent the dot separation. Similarly, these names will
        //  separate incorrectly:
        //
        //      int[foo.bar]
        //      [foo.bar]
        //      vector foo.bar[3]
        //
        //  This function assumes that these symbols will live in the
        //  global namespace and therefore will be found if no tokenizing
        //  of the name occurs.
        //

        qualifiedNameLookup(names, this, symbols);

        if (symbols.empty())
        {
            names.clear();
            names.push_back(name);
            qualifiedNameLookup(names, this, symbols);
        }
    }

    void Symbol::findSymbols(QualifiedName name,
                             ConstSymbolVector& symbols) const
    {
        NameVector names;
        context()->separateName(name, names);
        qualifiedNameLookup(names, this, symbols);

        //
        //  See comments in above function
        //

        if (symbols.empty())
        {
            names.clear();
            names.push_back(name);
            qualifiedNameLookup(names, this, symbols);
        }
    }

    Module* Symbol::globalModule()
    {
        return static_cast<Module*>(globalScope());
    }

    const Module* Symbol::globalModule() const
    {
        return static_cast<const Module*>(globalScope());
    }

    Symbol* Symbol::globalScope()
    {
        Symbol* s = this;

        if (scope())
        {
            while (s->scope())
                s = s->scope();
            return s;
        }
        else
        {
            return this;
        }
    }

    const Symbol* Symbol::globalScope() const
    {
        const Symbol* s = this;

        if (scope())
        {
            while (s->scope())
                s = s->scope();
            return s;
        }
        else
        {
            return this;
        }
    }

    Symbol* Symbol::firstOverload()
    {
        if (Symbol* s = scope())
        {
            if (Symbol* next = scope()->findSymbol(name()))
            {
                return next;
            }
            else
            {
                return this;
            }
        }
        else
        {
            return this;
        }
    }

    const Symbol* Symbol::firstOverload() const
    {
        if (const Symbol* s = scope())
        {
            if (const Symbol* next = scope()->findSymbol(name()))
            {
                return next;
            }
            else
            {
                return this;
            }
        }
        else
        {
            return this;
        }
    }

    void Symbol::resolve() const
    {
        if (symbolState() != ResolvedState)
        {
            const_cast<Symbol*>(this)->_resolving = true;

            if (_scope && resolveSymbols())
            {
                const_cast<Symbol*>(this)->_state = ResolvedState;
                MU_GC_END_CHANGE_STUBBORN(this);
            }
            else
            {
                //
                //	Symbol is not resolved -- it still contains
                //	names which are unresolved. It will be ignored
                //	by other table functions
                //

                const_cast<Symbol*>(this)->_state = Symbol::UnresolvedState;
            }

            const_cast<Symbol*>(this)->_resolving = false;
        }
    }

    void Symbol::symbolDependancies(ConstSymbolVector&) const {}

} // namespace Mu
