#ifndef __Mu__PartialEval__h__
#define __Mu__PartialEval__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Function.h>
#include <Mu/NodeAssembler.h>
#include <Mu/SymbolTable.h>
#include <Mu/ParameterVariable.h>
#include <Mu/TypeVariable.h>
#include <map>
#include <vector>

namespace Mu
{
    class Variable;

    //
    //  Takes a function and an argument list and returns a new function with
    //  some of the parameter replaced by constants.
    //

    class FunctionSpecializer
    {
    public:
        typedef Function::ArgumentVector ArgumentVector;
        typedef std::vector<bool> ArgumentMask;
        typedef NodeAssembler::NodeList NodeList;
        typedef NodeAssembler::SymbolList SymbolList;
        typedef ParameterVariable* Param;
        typedef STLMap<const Param, int>::Type IndexMap;
        typedef STLMap<const Param, Param>::Type ParameterMap;
        typedef STLMap<const Variable*, Variable*>::Type VariableMap;
        typedef STLVector<const Variable*>::Type Variables;
        typedef SymbolTable::SymbolHashTable SymbolHashTable;
        typedef SymbolTable::Item Item;
        typedef TypeVariable::Bindings TypeBindings;

        FunctionSpecializer(const Function*, Process*, Thread*);
        ~FunctionSpecializer();

        Function* partiallyEvaluate(const ArgumentVector&, const ArgumentMask&);

        Function* specialize(const TypeBindings&);

        Function* result() const { return _lambda; }

    private:
        void accumulateVariables(const Symbol*);
        Node* translate(const Node*);
        const Type* translate(const Type*);
        void declareVariables();
        void doit(const char*, SymbolList, bool);
        Node* callDirectly();

    private:
        Node* _root;
        NodeAssembler _as;
        const Function* _f;
        Function* _lambda;
        ArgumentVector _args;
        ArgumentMask _mask;
        IndexMap _originalIndex;
        ParameterMap _map;
        Variables _variables;
        VariableMap _variableMap;
        TypeBindings _typeBindings;
    };

} // namespace Mu

#endif // __Mu__PartialEval__h__
