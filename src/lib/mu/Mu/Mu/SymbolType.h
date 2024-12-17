#ifndef __MuLang__SymbolType__h__
#define __MuLang__SymbolType__h__
//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/OpaqueType.h>
#include <iosfwd>

namespace Mu
{

    //
    //  class SymbolType
    //
    //  Base type for internal symbols used by runtime
    //

    class SymbolType : public OpaqueType
    {
    public:
        SymbolType(Context*, const char*);
        ~SymbolType();

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void load();
    };

    class TypeSymbolType : public SymbolType
    {
    public:
        TypeSymbolType(Context*, const char*);
        ~TypeSymbolType();
    };

    class FunctionSymbolType : public SymbolType
    {
    public:
        FunctionSymbolType(Context*, const char*);
        ~FunctionSymbolType();
    };

    class VariableSymbolType : public SymbolType
    {
    public:
        VariableSymbolType(Context*, const char*);
        ~VariableSymbolType();
    };

    class ParameterSymbolType : public SymbolType
    {
    public:
        ParameterSymbolType(Context*, const char*);
        ~ParameterSymbolType();
    };

} // namespace Mu

#endif // __MuLang__SymbolType__h__
