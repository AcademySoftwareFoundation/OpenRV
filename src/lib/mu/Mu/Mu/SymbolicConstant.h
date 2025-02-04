#ifndef __Mu__SymbolicConstant__h__
#define __Mu__SymbolicConstant__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Symbol.h>
#include <Mu/Value.h>

namespace Mu
{
    class Type;

    namespace Archive
    {
        class Reader;
        class Writer;
    } // namespace Archive

    class SymbolicConstant : public Symbol
    {
    public:
        SymbolicConstant(Context* context, const char* name, const Type*,
                         const Value&);
        SymbolicConstant(Context* context, const char* name, const char* type,
                         const Value&);
        ~SymbolicConstant();

        //
        //	Symbol API
        //

        virtual const Type* nodeReturnType(const Node*) const;
        virtual void outputNode(std::ostream&, const Node*) const;

        //
        //	Output the symbol
        //

        virtual void output(std::ostream&) const;

        //
        //	SymbolicConstant API
        //

        const Type* type() const;

        //
        //	Value
        //

        Value value() const { return _value; }

        void setValue(Value v) { _value = v; }

    protected:
        virtual bool resolveSymbols() const;

    private:
        mutable SymbolRef _type;
        Value _value;

        friend class Archive::Reader;
        friend class Archive::Writer;
    };

} // namespace Mu

#endif // __Mu__SymbolicConstant__h__
