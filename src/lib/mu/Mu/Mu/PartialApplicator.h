#ifndef __Mu__PartialApplicator__h__
#define __Mu__PartialApplicator__h__
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
#include <map>
#include <vector>

namespace Mu
{
    class Variable;

    //
    //  class PartialApplicator
    //
    //  Takes a function and an argument list and returns a new function
    //  with some of the parameters replaced by constants (already
    //  applied). The function object returned is similar in many ways to
    //  the result returned by ParatialEvaluator. Since the API is the
    //  same, the classes are nearly interchangeable.
    //

    class PartialApplicator
    {
    public:
        typedef Function::ArgumentVector ArgumentVector;
        typedef std::vector<bool> ArgumentMask;
        typedef NodeAssembler::NodeList NodeList;
        typedef NodeAssembler::SymbolList SymbolList;
        typedef ParameterVariable* Param;
        typedef STLVector<Param>::Type ParameterVector;

        PartialApplicator(const Function*, Process*, Thread*,
                          const ArgumentVector&, const ArgumentMask&,
                          bool dynamicDispatch = false);

        ~PartialApplicator();

        Function* result() const { return _closure; }

    private:
        Node* generate(const ArgumentVector&, ParameterVector&);

    private:
        Node* _root;
        Node* _this;
        NodeAssembler _as;
        const Function* _f;
        Function* _closure;
        bool _method;
    };

} // namespace Mu

#endif // __Mu__PartialApplicator__h__
