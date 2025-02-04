#ifndef __Mu__Alias__h__
#define __Mu__Alias__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Symbol.h>

namespace Mu
{

    //
    //  class Alias
    //
    //  An alias is a symbol that points to another symbol. An alias has no
    //  particular type and no node should be created by it. The alias is
    //  primarily used by a parser to keep track of typedef and similar. Note
    //  that an alias necessarily represents a whole family of similarily named
    //  symbols. If the alias points to a type, it may also be pointing to
    //  constructors for that type.
    //

    class Alias : public Symbol
    {
    public:
        Alias(Context* context, const char* name, Symbol* symbol);
        Alias(Context* context, const char* name, const char* symbol);
        virtual ~Alias();

        //
        //	Symbol API
        //

        virtual const Type* nodeReturnType(const Node*) const;
        virtual void outputNode(std::ostream&, const Node*) const;
        virtual void symbolDependancies(ConstSymbolVector&);

        //
        //	Output the symbol
        //

        virtual void output(std::ostream&) const;

        //
        //	Alias API
        //

        void set(const Symbol* s);
        const Symbol* alias() const;
        Symbol* alias();

    protected:
        virtual bool resolveSymbols() const;

    private:
        mutable SymbolRef _symbol;
    };

} // namespace Mu

#endif // __Mu__Alias__h__
