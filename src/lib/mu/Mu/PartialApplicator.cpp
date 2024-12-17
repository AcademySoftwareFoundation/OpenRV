//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <assert.h>
// #include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/Exception.h>
#include <Mu/GlobalVariable.h>
#include <Mu/MemberVariable.h>
#include <Mu/MemberFunction.h>
#include <Mu/Namespace.h>
#include <Mu/FreeVariable.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/PartialApplicator.h>
#include <Mu/MuProcess.h>
#include <Mu/Type.h>
#include <Mu/Context.h>
#include <stdio.h>

namespace Mu
{
    using namespace std;

    PartialApplicator::PartialApplicator(const Function* f, Process* p,
                                         Thread* t, const ArgumentVector& args,
                                         const ArgumentMask& mask,
                                         bool dynamicDispatch)
        : _as(p->context(), p, t)
        , _root(0)
        , _f(f)
        , _method(dynamic_cast<const MemberFunction*>(f) != 0
                  && dynamicDispatch)
    {
        _as.allowUnresolvedCalls(false);
        SymbolList newParams = _as.emptySymbolList();
        ParameterVector closureParams;

        //
        //  Create a map from old parameters to new.
        //

        for (int i = 0; i < f->numArgs() + f->numFreeVariables(); i++)
        {
            ParameterVariable* p = (ParameterVariable*)f->parameter(i);

            if (!mask[i])
            {
                ParameterVariable* nv;

                if (p)
                {
                    nv = new ParameterVariable(_as.context(), p->name().c_str(),
                                               p->storageClass());
                }
                else
                {
                    char tname[20];
                    sprintf(tname, "p%d", i);
                    nv = new ParameterVariable(_as.context(), tname,
                                               f->argType(i));
                }

                if (!dynamic_cast<FreeVariable*>(p))
                {
                    newParams.push_back(nv);
                }

                closureParams.push_back(nv);
            }
            else
            {
                closureParams.push_back(0);
            }
        }

        _as.newStackFrame();

        if (newParams.empty())
        {
            _closure = new Function(
                _as.context(),
                _as.context()->uniqueName(_as.scope(), "__lambda").c_str(),
                _f->returnType(), 0, 0, 0,
                Function::ContextDependent | Function::LambdaExpression);
        }
        else
        {
            _closure = new Function(
                _as.context(),
                _as.context()->uniqueName(_as.scope(), "__lambda").c_str(),
                _f->returnType(), newParams.size(),
                (ParameterVariable**)&(newParams.front()), 0,
                Function::ContextDependent | Function::LambdaExpression);
        }

        _as.scope()->addAnonymousSymbol(_closure);
        //_as.scope()->addSymbol(_closure);
        _as.pushScope(_closure);
        _as.declareParameters(newParams);
        _as.removeSymbolList(newParams);

        _root = generate(args, closureParams);

        if (!_root)
            throw InconsistantSignatureException();

        int stackSize = _as.endStackFrame();
        _as.popScope();
        _closure->stackSize(stackSize);

        if (_closure->hasReturn())
        {
            _closure->setBody(_root);
        }
        else if (Node* code = _as.cast(_root, _closure->returnType()))
        {
            _closure->setBody(code);
        }
        else
        {
            throw BadCastException();
        }
    }

    PartialApplicator::~PartialApplicator() {}

    Node* PartialApplicator::generate(const ArgumentVector& args,
                                      ParameterVector& closureParams)
    {
        NodeList nl = _as.emptyNodeList();
        Node* thisArg = 0;

        for (int i = 0, s = closureParams.size(); i < s; i++)
        {
            const Type* t = _f->argType(i);
            Node* n = 0;

            if (ParameterVariable* pv = closureParams[i])
            {
                if (_method && i == 0)
                {
                    thisArg = _as.dereferenceLValue(_as.referenceVariable(pv));
                }
                else
                {
                    n = _as.dereferenceLValue(_as.referenceVariable(pv));
                }
            }
            else
            {
                // Object* o = t->isPrimitiveType() ? 0 :
                // reinterpret_cast<Object*>(args[i]._Pointer);
                // dn = _as.constant(t, o);

                DataNode* dn = _as.constant(t);
                dn->_data = args[i];

                if (_method && i == 0)
                {
                    thisArg = dn;
                }
                else
                {
                    n = dn;
                }
            }

            if (n)
                nl.push_back(n);
        }

        Node* fn = 0;

        if (thisArg)
        {
            fn = _as.callMethod(_f, thisArg, nl);
        }
        else
        {
            fn = _as.callFunction(_f, nl);
        }

        _as.removeNodeList(nl);
        return fn;
    }

} // namespace Mu
