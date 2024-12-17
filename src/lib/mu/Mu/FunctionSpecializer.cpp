//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/BaseFunctions.h>
#include <Mu/Context.h>
#include <Mu/Exception.h>
#include <Mu/FreeVariable.h>
#include <Mu/Function.h>
#include <Mu/FunctionSpecializer.h>
#include <Mu/GlobalVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/MemberVariable.h>
#include <Mu/Namespace.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/Type.h>
#include <Mu/Unresolved.h>
#include <assert.h>
#include <stdio.h>

namespace Mu
{
    using namespace std;

    FunctionSpecializer::FunctionSpecializer(const Function* f, Process* p,
                                             Thread* t)
        : _as(p->context(), p, t)
        , _root(0)
        , _f(f)
    {
        _as.allowUnresolvedCalls(false);
    }

    FunctionSpecializer::~FunctionSpecializer() {}

    void FunctionSpecializer::doit(const char* name, SymbolList newParams,
                                   bool lambda)
    {
        accumulateVariables(_f);

        _as.newStackFrame();

        if (newParams.empty())
        {
            _lambda = new Function(
                _as.context(), name, translate(_f->returnType()), 0, 0, 0,
                _f->baseAttributes() | Function::ContextDependent
                    | Function::LambdaExpression);
        }
        else
        {
            _lambda = new Function(
                _as.context(), name, translate(_f->returnType()),
                newParams.size(), (ParameterVariable**)&(newParams.front()), 0,
                _f->baseAttributes() | Function::ContextDependent
                    | Function::LambdaExpression);
        }

        if (lambda)
        {
            _as.scope()->addAnonymousSymbol(_lambda);
        }
        else
        {
            ((Symbol*)(_f->scope()))->addSymbol(_lambda);
        }

        _as.pushScope(_lambda);
        _as.declareParameters(newParams);
        _as.removeSymbolList(newParams);

        declareVariables();

        if (_f->body())
        {
            _root = translate(_f->body());
        }
        else if (_f->compiledFunction())
        {
            _root = callDirectly();
        }

        if (!_root)
            throw InconsistantSignatureException();

        int stackSize = _as.endStackFrame();
        _as.popScope();
        _lambda->stackSize(stackSize);

        if (_lambda->hasReturn())
        {
            _lambda->setBody(_root);
        }
        else if (Node* code = _as.cast(_root, _lambda->returnType()))
        {
            _lambda->setBody(code);
        }
        else
        {
            throw BadCastException();
        }
    }

    Function* FunctionSpecializer::partiallyEvaluate(const ArgumentVector& args,
                                                     const ArgumentMask& mask)
    {
        _args = args;
        _mask = mask;

        SymbolList newParams = _as.emptySymbolList();

        for (int i = 0; i < _f->numArgs() + _f->numFreeVariables(); i++)
        {
            Param p = (Param)_f->parameter(i);
            _originalIndex[p] = i;

            if (!_mask[i])
            {
                Param nv = new ParameterVariable(
                    _as.context(), p->name().c_str(), p->storageClass());

                if (!dynamic_cast<FreeVariable*>(p))
                {
                    newParams.push_back(nv);
                }

                _map[p] = nv;
            }
        }

        doit(_as.uniqueNameInScope("__lambda").c_str(), newParams, true);
        return _lambda;
    }

    Function* FunctionSpecializer::specialize(const TypeBindings& bindings)
    {
        _typeBindings = bindings;
        SymbolList newParams = _as.emptySymbolList();

        for (int i = 0; i < _f->numArgs() + _f->numFreeVariables(); i++)
        {
            Param p = (Param)_f->parameter(i);
            _originalIndex[p] = i;

            Param nv = new ParameterVariable(_as.context(), p->name().c_str(),
                                             translate(p->storageClass()));

            if (!dynamic_cast<FreeVariable*>(p))
            {
                newParams.push_back(nv);
            }

            _map[p] = nv;
        }

        doit(_f->name().c_str(), newParams, false);
        return _lambda;
    }

    Node* FunctionSpecializer::callDirectly()
    {
        NodeList nl = _as.emptyNodeList();

        for (int i = 0; i < _args.size(); i++)
        {
            if (_mask[i])
            {
                DataNode* dn = _as.constant(_f->parameter(i)->storageClass());
                dn->_data = _args[i];
                nl.push_back(dn);
            }
            else
            {
                nl.push_back(_as.dereferenceVariable(_f->parameter(i)));
            }
        }

        Node* n = _as.callFunction(_f, nl);
        _as.removeNodeList(nl);
        return n;
    }

    const Type* FunctionSpecializer::translate(const Type* t)
    {
        if (t->isTypeVariable())
        {
            const TypeVariable* tp = static_cast<const TypeVariable*>(t);
            TypeBindings::const_iterator i = _typeBindings.find(tp);
            if (i != _typeBindings.end())
                return i->second;
        }

        return t;
    }

    void FunctionSpecializer::declareVariables()
    {
        char temp[80];

        for (int i = 0; i < _variables.size(); i++)
        {
            const Variable* v = _variables[i];

            if (const ParameterVariable* pv =
                    dynamic_cast<const ParameterVariable*>(v))
            {
                Variable* np = _map[(ParameterVariable*)pv];
                _variableMap[v] = np;
                continue;
            }

            sprintf(temp, "v%d", i);

            _as.clearInitializerList();
            const Type* t = translate(v->storageClass());

            //
            //  A bit wasteful currently. Just discards the constant nodes that
            //  are supplied by the declareVariables function. It will copy the
            //  initializer expression if it finds one later
            //

            //
            //  TODO: change this to use the alternate
            //  declareStackVariables function in NodeAssembler.
            //

            if (dynamic_cast<const StackVariable*>(v))
            {
                _as.declarationType(t);
                Variable* nv = _as.declareStackVariable(
                    t, _as.context()->internName(temp), Variable::ReadWrite);
                _variableMap[v] = nv;
            }
            else if (dynamic_cast<const GlobalVariable*>(v))
            {
                _as.declarationType(t, true);
                Variable* nv = new GlobalVariable(
                    _as.context(), temp, t, _as.process()->globals().size(),
                    Variable::ReadWrite, 0);
                _as.scope()->addSymbol(nv);
                _as.process()->globals().push_back(Value());
                _variableMap[v] = nv;
            }
        }
    }

    void FunctionSpecializer::accumulateVariables(const Symbol* sym)
    {
        if (!sym->symbolTable())
            return;

        SymbolHashTable& ht = sym->symbolTable()->hashTable();

        for (SymbolHashTable::Iterator i(ht); i; ++i)
        {
            Symbol* is = *i;

            for (Symbol* s = is->firstOverload(); s; s = s->nextOverload())
            {
                if (const ParameterVariable* pv =
                        dynamic_cast<const ParameterVariable*>(s))
                {
                    int index = _originalIndex[(ParameterVariable*)pv];
                    assert(index != -1);

                    //
                    //  Skip parameters that have values
                    //

                    if (!_mask.empty() && _mask[index])
                        continue;
                }

                if (const Variable* v = dynamic_cast<const Variable*>(s))
                {
                    _variables.push_back(v);
                }
                else if (dynamic_cast<Namespace*>(s))
                {
                    accumulateVariables(s);
                }
            }
        }
    }

    Node* FunctionSpecializer::translate(const Node* n)
    {
        const Symbol* sym = n->symbol();

        if (const Function* f = dynamic_cast<const Function*>(sym))
        {
            NodeList nl = _as.emptyNodeList();

            for (size_t i = 0, s = n->numArgs(); i < s; i++)
            {
                nl.push_back(translate(n->argNode(i)));
            }

            Node* rn = _as.callBestOverloadedFunction(f, nl);
            _as.removeNodeList(nl);

            if (f->hasHiddenArgument())
            {
                const DataNode* dn = static_cast<const DataNode*>(n);
                DataNode* drn = static_cast<DataNode*>(rn);
                drn->_data = dn->_data;
            }

            if (f == _as.context()->returnFromFunction()
                || f == _as.context()->returnFromVoidFunction())
            {
                _lambda->hasReturn(true);
            }

            return rn;
        }

        else if (dynamic_cast<const UnresolvedConstructor*>(sym))
        {
            const DataNode* dn = static_cast<const DataNode*>(n);
            const Type* t = reinterpret_cast<const Type*>(dn->_data._Pointer);

            NodeList nl = _as.emptyNodeList();

            for (size_t i = 0, s = n->numArgs(); i < s; i++)
            {
                nl.push_back(translate(n->argNode(i)));
            }

            //
            //  NOTE: probably need to decide what SCOPE to create here!
            //

            Node* rn = _as.call(translate(t), nl);
            _as.removeNodeList(nl);
            return rn;
        }

        else if (dynamic_cast<const UnresolvedCast*>(sym))
        {
            const DataNode* dn = static_cast<const DataNode*>(n);
            Name uname = dn->_data._name;

            if (const Type* t =
                    _as.context()->findSymbolOfTypeByQualifiedName<Type>(uname))
            {
                Node* rn = _as.cast(translate(n->argNode(0)), t);
                assert(rn);
                return rn;
            }
            else
            {
                abort();
            }
        }

        else if (dynamic_cast<const UnresolvedCall*>(sym))
        {
            const DataNode* dn = static_cast<const DataNode*>(n);
            Name uname = dn->_data._name;

            if (uname == "[]")
            {
                NodeList nl = _as.emptyNodeList();

                for (int i = 1; i < n->numArgs(); i++)
                {
                    nl.push_back(translate(n->argNode(i)));
                }

                Node* rn =
                    _as.memberOperator("[]", translate(n->argNode(0)), nl);
                _as.removeNodeList(nl);

                assert(rn);
                return rn;
            }

            if (uname == "()")
            {
                Node* ln = _as.dereferenceLValue(translate(n->argNode(0)));
                NodeList nl = _as.emptyNodeList();
                for (int i = 1; i < n->numArgs(); i++)
                {
                    nl.push_back(translate(n->argNode(i)));
                }

                Node* rn = _as.call(ln, nl);
                _as.removeNodeList(nl);
                assert(rn);
                return rn;
            }

            if (uname == "=")
            {
                const Node* lhs = n->argNode(0);

                if (lhs->type() == _as.context()->unresolvedType()
                    && lhs->symbol()
                           == _as.context()->unresolvedStackReference())
                {
                    const DataNode* dn = static_cast<const DataNode*>(lhs);
                    StackVariable* sv =
                        reinterpret_cast<StackVariable*>(dn->_data._Pointer);

                    Variable* nv = _variableMap[sv];

                    if (sv->isImplicitlyTyped())
                    {
                        if (nv->storageClass()
                            == _as.context()->unresolvedType())
                        {
                            Node* rhs = translate(n->argNode(1));
                            if (rhs->type()->isReferenceType())
                            {
                                const ReferenceType* rt =
                                    static_cast<const ReferenceType*>(
                                        rhs->type());
                                nv->setStorageClass(rt->dereferenceType());
                            }
                            else
                            {
                                nv->setStorageClass(rhs->type());
                            }

                            NodeList nl = _as.emptyNodeList();
                            nl.push_back(translate(lhs));
                            nl.push_back(rhs);
                            Node* rn = _as.callBestFunction("=", nl);
                            _as.removeNodeList(nl);
                            return rn;
                        }
                    }
                }
            }

            NodeList nl = _as.emptyNodeList();

            for (size_t i = 0, s = n->numArgs(); i < s; i++)
            {
                nl.push_back(translate(n->argNode(i)));
            }

            //
            //  NOTE: probably need to decide what SCOPE to create here!
            //

            Node* rn = _as.callBestFunction(uname.c_str(), nl);
            _as.removeNodeList(nl);
            return rn;
        }

        else if (dynamic_cast<const UnresolvedMemberCall*>(sym))
        {
            //
            //  Check for member reference above this
            //

            const DataNode* mr = (const DataNode*)n->argNode(0);
            const UnresolvedMemberReference* mrsym =
                dynamic_cast<const UnresolvedMemberReference*>(mr->symbol());
            Name uname = mr->_data._name;

            if (mrsym)
            {
            }

            abort();
        }

        else if (dynamic_cast<const UnresolvedMemberReference*>(sym))
        {
            //
            //  Check for member reference above this
            //

            const DataNode* mr = (const DataNode*)n->argNode(0);
            const UnresolvedMemberReference* mrsym =
                dynamic_cast<const UnresolvedMemberReference*>(mr->symbol());
            Name uname = mr->_data._name;

            if (mrsym)
            {
            }

            abort();
        }

        else if (dynamic_cast<const UnresolvedStackReference*>(sym))
        {
            const DataNode* dn = static_cast<const DataNode*>(n);
            StackVariable* sv =
                reinterpret_cast<StackVariable*>(dn->_data._Pointer);
            Variable* v = _variableMap[sv];
            return _as.referenceVariable(v);
        }

        else if (dynamic_cast<const UnresolvedStackDereference*>(sym))
        {
            const DataNode* dn = static_cast<const DataNode*>(n);
            StackVariable* sv =
                reinterpret_cast<StackVariable*>(dn->_data._Pointer);
            Variable* v = _variableMap[sv];
            return _as.dereferenceVariable(v);
        }

        else if (const Type* t = dynamic_cast<const Type*>(sym))
        {
            //
            //  Cannot be specialized (its a constant)
            //

            DataNode* dn = _as.constant(t);
            const DataNode* idn = static_cast<const DataNode*>(n);
            dn->_data = idn->_data;

            if (!t->isPrimitiveType())
            {
                //_as.process()->constants().push_back((Object*)dn->_data._Pointer);
            }

            return dn;
        }

        else if (const ParameterVariable* v =
                     dynamic_cast<const ParameterVariable*>(sym))
        {
            int n = _originalIndex[(ParameterVariable*)v];
            assert(n != -1);

            if (!_mask.empty() && _mask[n])
            {
                const Type* t = v->storageClass();
                DataNode* dn;

                if (t->isPrimitiveType())
                {
                    dn = _as.constant(t);
                    dn->_data = _args[n];
                }
                else
                {
                    Object* o = (Object*)(_args[n]._Pointer);
                    dn = _as.constant(t, o);
                    dn->_data._Pointer = (Pointer)o;
                }

                return dn;
            }
        }

        //
        //  Fall-through case for parameter variables is to treat them
        //  like variables. Note that the a check is done to find out if
        //  the variable node is referencing or dereferencing the
        //  value. This "optimization" makes the node tree faster, but it
        //  also makes it a lot more confusing that two nodes could have
        //  identical arguments return types and symbols yet still have a
        //  different node function. Maybe that should be changed.
        //

        if (const Variable* v = dynamic_cast<const Variable*>(sym))
        {
            const StackVariable* sv = dynamic_cast<const StackVariable*>(sym);
            const GlobalVariable* gv = dynamic_cast<const GlobalVariable*>(sym);

            if (sv || gv)
            {
                const Variable* nv = _variableMap[v];

                if (gv && !nv)
                {
                    //
                    //  Not in the map and its a global variable, just
                    //  reference it directly. (Its a global variable that
                    //  is not in the scope of the function).
                    //

                    nv = v;
                }

                Node* nn = _as.referenceVariable(nv);
                if (n->func() != nn->func())
                    nn = _as.dereferenceLValue(nn);
                return nn;
            }
            else if (const MemberVariable* v =
                         dynamic_cast<const MemberVariable*>(sym))
            {
                Node* a = translate(n->argNode(0));
                Node* nn = _as.referenceMemberVariable(v, a);
                if (n->func() != nn->func())
                    nn = _as.dereferenceLValue(nn);
                return nn;
            }
        }

        abort();
        return 0;
    }

} // namespace Mu
