//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Context.h>
#include <Mu/Context.h>
#include <Mu/FreeVariable.h>
#include <Mu/Function.h>
#include <Mu/FunctionType.h>
#include <Mu/GlobalVariable.h>
#include <Mu/MachineRep.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/TypePattern.h>
#include <Mu/Type.h>
#include <Mu/config.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#if defined _MSC_VER
#define snprintf _snprintf
#endif

namespace Mu
{

    using namespace std;

    Function::Function(Context* context, const char* name)
        : Symbol(context, name)
    {
    }

    Function::Function(Context* context, const char* name, NodeFunc func,
                       Attributes attrs, ...)
        : Symbol(context, name)
    {
        va_list ap;
        va_start(ap, attrs);
        init(func, attrs, ap);
        va_end(ap);
    }

    Function::Function(Context* context, const char* name,
                       const Type* returnType, int nparams,
                       ParameterVariable** params, Node* code, Attributes attrs)
        : Symbol(context, name)
    {
        init(code, returnType, nparams, params, attrs);
    }

    Function::Function(Context* context, const char* name,
                       const Type* returnType, int nparams,
                       ParameterVariable** params, NodeFunc func,
                       Attributes attrs)
        : Symbol(context, name)
    {
        init(0, returnType, nparams, params, attrs | Native);
        _func = func;
    }

    void Function::init(Node* code, const Type* returnType, int nparams,
                        ParameterVariable** params, Attributes attrs)
    {
        Signature* sig = new Signature();

        assert((nparams == 0 && params == 0) || (nparams != 0 && params != 0));

        _compiledFunction = (CompiledFunction)0;
        _signature = sig;
        _stackSize = nparams;
        _interfaceIndex = 0;
        _code = code;
        _func = NodeFunc(0);
        _mapped = attrs & Mapped ? true : false;
        _operator = attrs & Operator ? true : false;
        _memberOp = attrs & MemberOperator ? true : false;
        _cast = attrs & Cast ? true : false;
        _lossy = attrs & Lossy ? true : false;
        _commutative = attrs & Commutative ? true : false;
        _contextDependent = attrs & ContextDependent ? true : false;
        _native = attrs & Native ? true : false;
        _retaining = attrs & Retaining ? true : false;
        _lambda = attrs & LambdaExpression ? true : false;
        _sideEffects = !(attrs & ~NoSideEffects) ? true : false;
        _dependentSideEffects = attrs & DependentSideEffects ? true : false;
        _dynamicActivation = attrs & DynamicActivation ? true : false;
        _hiddenArgument = attrs & HiddenArgument ? true : false;
        _generated = attrs & Generated ? true : false;
        _noAttributes = false;
        _multiSignature = false;
        _method = false;
        _hasParameters = true;
        _returns = false;
        _polymorphic = false;
        _maximumArgs = nparams;
        _minimumArgs = 0;
        _requiredArgs = nparams;
        _state = UntouchedState;
        _type = 0;
        _variadic = false;
        _unresolvedStubs = false;
        _datanode = _hiddenArgument;

        sig->push_back(returnType ? returnType->fullyQualifiedName()
                                  : context()->internName(""));

        for (size_t i = 0; i < nparams; i++)
        {
            addSymbol(params[i]);

            if (!dynamic_cast<FreeVariable*>(params[i]))
            {
                _minimumArgs += params[i]->hasDefaultValue() ? 0 : 1;
                sig->push_back(params[i]->storageClassName());
            }
        }
    }

    void Function::init(NodeFunc func, Attributes attrs, va_list& ap)
    {
        //
        //	Use of this init implies a native function. This is overridden
        //	if NonNative is passed in. NonNative has no meaning for other
        //	constructors.
        //

        if (!(attrs & NonNative))
            attrs |= Native;

        Signature* sig = new Signature;

        _signature = sig;
        _compiledFunction = (CompiledFunction)0;
        _stackSize = 0;
        _code = 0;
        _interfaceIndex = 0;
        _func = func;
        _noAttributes = !attrs;
        _mapped = attrs & Mapped ? true : false;
        _operator = attrs & Operator ? true : false;
        _memberOp = attrs & MemberOperator ? true : false;
        _cast = attrs & Cast ? true : false;
        _lossy = attrs & Lossy ? true : false;
        _commutative = attrs & Commutative ? true : false;
        _contextDependent = attrs & ContextDependent ? true : false;
        _native = attrs & Native ? true : false;
        _retaining = attrs & Retaining ? true : false;
        _lambda = attrs & LambdaExpression ? true : false;
        _sideEffects = !(attrs & ~NoSideEffects) ? true : false;
        _dependentSideEffects = attrs & DependentSideEffects ? true : false;
        _dynamicActivation = attrs & DynamicActivation ? true : false;
        _hiddenArgument = attrs & HiddenArgument ? true : false;
        _generated = attrs & Generated ? true : false;
        _hasParameters = false;
        _multiSignature = false;
        _method = false;
        _returns = false;
        _polymorphic = false;
        _minimumArgs = 0;
        _maximumArgs = 0;
        _requiredArgs = 0;
        _type = 0;
        _variadic = false;
        _datanode = _hiddenArgument;
        _unresolvedStubs = false;

        int aMode = -1;

        QualifiedName returnTypeName;
        STLVector<QualifiedName>::Type typeNames;

        while (char* arg = va_arg(ap, char*))
        {
            switch (reinterpret_cast<ArgKeyword>(arg))
            {
            case Return:
                aMode = Return;
                break;

            case Compiled:
                aMode = Compiled;
                break;

            case Args:
                aMode = Args;
                break;

            case ArgVector:
                aMode = ArgVector;
                break;

            case Parameters:
                aMode = Parameters;
                _hasParameters = true;
                break;

            case Optional:
                aMode = Optional;
                break;

            case Maximum:
                _maximumArgs = va_arg(ap, int);
                break;

            default:
                switch (aMode)
                {
                case Return:
                    returnTypeName = context()->internName(arg);
                    break;

                case ArgVector:
                    if (!arg)
                        break;

                    {
                        const StringVector* names =
                            reinterpret_cast<StringVector*>(arg);

                        for (int i = 0; i < names->size(); i++)
                        {
                            typeNames.push_back(
                                context()->internName((*names)[i]));
                            _minimumArgs++;
                            if (_maximumArgs < typeNames.size())
                                _maximumArgs = typeNames.size();
                        }
                    }

                    break;

                case Args:
                case Parameters:
                    if (!arg)
                        break;
                    _requiredArgs++;

                    if (_hasParameters)
                    {
                        ParameterVariable* p =
                            reinterpret_cast<ParameterVariable*>(arg);
                        if (!p->hasDefaultValue())
                        {
                            _minimumArgs++;
                        }
                    }
                    else
                    {
                        _minimumArgs++;
                    }

                    // fall through

                case Optional:
                    if (_hasParameters)
                    {
                        ParameterVariable* p =
                            reinterpret_cast<ParameterVariable*>(arg);
                        p->setAddress(_parameters.size());
                        addSymbol(p);
                        typeNames.push_back(p->storageClassName());
                    }
                    else
                    {
                        typeNames.push_back(context()->internName(arg));
                    }
                    if (_maximumArgs < typeNames.size())
                        _maximumArgs = typeNames.size();
                    break;

                case Compiled:
                    _compiledFunction = reinterpret_cast<CompiledFunction>(arg);
                    break;

                default:
                    assert("bad argument in Function::init" == 0);
                }
            }
        }

        sig->push_back(returnTypeName);
        for (int i = 0; i < typeNames.size(); i++)
            sig->push_back(typeNames[i]);
    }

    Function::~Function() { _code->deleteSelf(); }

    String Function::mangledName() const
    {
        String n;

        if (isLambda())
        {
            char temp[80];
            snprintf(temp, 80, "%p", this);
            n = temp;
        }
        else
        {

            if (scope() != globalScope())
            {
                n += scope()->mangledName();
                n += "_";
            }

            n += context()->encodeName(name());
            n += "_";
            n += returnType()->mangledName();

            for (int i = 0; i < numArgs() + numFreeVariables(); i++)
            {
                n += "_";
                if (i >= numArgs())
                    n += "F";
                n += argType(i)->mangledName();
            }
        }

        return n;
    }

    Function::Attributes Function::baseAttributes() const
    {
        Attributes a = 0;

        if (isOperator())
            a |= Operator;
        if (isMemberOperator())
            a |= MemberOperator;
        if (isCast())
            a |= Cast;
        if (isCommutative())
            a |= Commutative;
        if (isMapped())
            a |= Mapped;
        if (isLossy())
            a |= Lossy;
        if (!hasSideEffects())
            a |= NoSideEffects;
        if (isRetaining())
            a |= Retaining;
        if (isDynamicActivation())
            a |= DynamicActivation;
        if (isLambda())
            a |= LambdaExpression;
        if (hasHiddenArgument())
            a |= HiddenArgument;
        if (hasDependentSideEffects())
            a |= DependentSideEffects;

        return a;
    }

    NodeFunc Function::func(Node*) const
    {
        if (isDynamicActivation())
        {
            return returnType()->machineRep()->dynamicActivationFunc();
        }
        else if (_code || !_native)
        {
            //
            //  This will pick up the recursive case in which _code is not yet
            //  set. This happens in a non-native recursive function. The
            //  return type should be known ahead of time, so this should work.
            //

            return returnType()->machineRep()->functionActivationFunc();
        }
        else
        {
            return _func;
        }
    }

    const FunctionType* Function::type() const
    {
        if (!_type)
        {
            if (symbolState() != ResolvedState)
                resolve();

            if (symbolState() != ResolvedState)
            {
                cerr << "WARNING: unable to resolve function "
                     << fullyQualifiedName() << endl;
                return 0;
            }

            Context* c = (Context*)globalModule()->context();
            _type = c->functionType(_signature);
        }

        return _type;
    }

    const Signature* Function::signature() const
    {
        if (symbolState() != ResolvedState)
            resolve();
        if (symbolState() != ResolvedState)
            return 0;
        return _signature;
    }

    void Function::symbolDependancies(ConstSymbolVector& symbols) const
    {
        if (symbolState() != ResolvedState)
            resolve();
        symbols.push_back(returnType());
        for (int i = 0, s = numArgs(); i < numArgs(); i++)
            symbols.push_back(argType(i));
    }

    void Function::addSymbol(Symbol* s)
    {
        //
        //  Collect parameters and free variables in the parameter list
        //

        if (ParameterVariable* pv = dynamic_cast<ParameterVariable*>(s))
        {
            //
            //  The pv will already be resolved if its from parsing
            //  in that case find out if we're polytypic right now.
            //

            if (pv->storageClass() && pv->storageClass()->isTypeVariable()
                && !native())
            {
                _polymorphic = true;
            }

            _parameters.push_back(pv);
        }

        Symbol::addSymbol(s);
    }

    void Function::output(std::ostream& o) const
    {
        if (symbolState() != ResolvedState)
            resolve();

        //
        //	Note: this function does not directly deal with the
        //	_returnType and _argTypes because it is unclear what state the
        //	symbol is in (resolved or not) -- even after calling resolve()
        //	above; it could have failed.
        //

        //
        //	Operators automatically have the name operator prefixed
        //

        if (isOperator())
            o << "operator ";

        if (symbolState() != ResolvedState)
        {
            o << name() << "(" << "*unresolved*)";
            return;
        }

        //
        //	Arguments -- if unresolved, output the names instead (this is
        //	handled by the argName() function.
        //

        o << fullyQualifiedName() << " (" << returnType()->fullyQualifiedName()
          << ";";

        if (hasParameters())
        {
            for (int i = 0; i < numArgs(); i++)
            {
                const ParameterVariable* p = parameter(i);
                const Type* pc = p->storageClass();

                o << (i ? ", " : " ")
                  << (pc ? pc->fullyQualifiedName().c_str() : "*unresolved*")
                  << " " << p->name();

                if (p->hasDefaultValue())
                {
                    o << " = ";
                    p->storageClass()->outputValue(o, p->defaultValue());
                }
            }
        }
        else
        {

            for (int i = 0; i < numArgs(); i++)
            {
                o << (i ? ", " : " ") << argType(i)->fullyQualifiedName();
            }
        }

        o << ")";

#if 0
    if (isPure())
    {
        o << " pure";
    }
    else if (maybePure())
    {
        o << " maybe pure";
    }
    else
    {
        o << " impure";
    }
#endif
    }

    bool Function::isPolymorphic() const
    {
        if (symbolState() != ResolvedState)
            resolve();
        // assert(symbolState() == ResolvedState);
        return _polymorphic;
    }

    bool Function::resolveSymbols() const
    {
        const Module* global = globalModule();
        if (!global)
            return false;

        const Context* context = global->context();

        if (!context)
            return false;
        _signature->resolve(context);

        if (!_signature->resolved())
        {
            // cerr << "ERROR: attempting to resolve symbols in function \"";
            // cerr << name() << "\"" << endl;
            return false;
        }
        else
        {
            for (int i = 0; i < _signature->size(); i++)
            {
                const Type* t =
                    static_cast<const Type*>((*_signature)[i].symbol);

                if (t->isTypePattern())
                {
                    _multiSignature = true;
                    const TypePattern* ptype =
                        static_cast<const TypePattern*>(t);
                    if (ptype->variadic())
                        _variadic = true;
                }

                if (t->isTypeVariable() && !native())
                {
                    _polymorphic = true;
                }
            }

            _signature = context->internSignature((Signature*)_signature);
            return true;
        }
    }

    int Function::numArgs() const { return _signature->size() - 1; }

    QualifiedName Function::argTypeName(int i) const
    {
        if (symbolState() == ResolvedState)
        {
            return (*_signature)[i + 1].symbol->fullyQualifiedName();
        }
        else
        {
            return (*_signature)[i + 1].name;
        }
    }

    QualifiedName Function::returnTypeName() const { return argTypeName(-1); }

    const Type* Function::argType(int i) const
    {
        if (symbolState() != ResolvedState)
            resolve();
        if (symbolState() != ResolvedState)
            return 0;

        if (i >= 0 && _hasParameters)
        {
            assert(_parameters.size() > i);
            const ParameterVariable* pv = _parameters[i];
            const Type* ptype = pv->storageClass();
            return ptype;
        }
        else
        {
            assert(i + 1 <= _maximumArgs);
            return static_cast<const Type*>((*_signature)[i + 1].symbol);
        }
    }

    bool Function::expandArgTypes(TypeVector& types, size_t size) const
    {
        if (symbolState() != ResolvedState)
            resolve();
        if (symbolState() != ResolvedState)
            return false;

        types.resize(size);

        if (_multiSignature)
        {
            for (int i = 0, fi = 0; i < size; i++, fi++)
            {
                const Type* type = argType(fi);
                types[i] = type;

                if (type->isTypePattern())
                {
                    const TypePattern* ptype =
                        static_cast<const TypePattern*>(type);
                    ptype->argumentAdjust(i, fi);
                }
            }
        }
        else
        {
            for (int i = 0; i < size; i++)
            {
                types[i] = argType(i);
            }
        }

        return true;
    }

    int Function::numFreeVariables() const
    {
        return _hasParameters ? _parameters.size() - numArgs() : 0;
    }

    const ParameterVariable* Function::parameter(int i) const
    {
        if (_hasParameters && i < _parameters.size())
        {
            return _parameters[i];
        }
        else
        {
            return 0;
        }
    }

    ParameterVariable* Function::parameter(int i)
    {
        if (_hasParameters && i < _parameters.size())
        {
            return _parameters[i];
        }
        else
        {
            return 0;
        }
    }

    const Type* Function::returnType() const { return argType(-1); }

    const Type* Function::nodeReturnType(const Node*) const
    {
        if (const Type* t = returnType())
        {
            if (t->isTypePattern())
            {
                cerr << "Function: ";
                output(cerr);
                cerr << endl;
                cerr << "\tneeds to implement Function::nodeReturnType()\n";
            }

            return t;
        }

        return 0;
    }

    bool Function::matches(const Function* f) const
    {
        if (name() == f->name())
        {
            int n = numArgs();
            if (n == f->numArgs())
            {
                if (returnTypeName() == f->returnTypeName())
                {
                    for (int i = 0; i < n; i++)
                    {
                        if (argTypeName(i) != f->argTypeName(i))
                            return false;
                    }

                    return true;
                }
            }
        }

        return false;
    }

    Function* Function::firstFunctionOverload()
    {
        if (Symbol* s = scope())
        {
            return scope()->findSymbolOfType<Function>(name());
        }
        else
        {
            return this;
        }
    }

    const Function* Function::firstFunctionOverload() const
    {
        if (const Symbol* s = scope())
        {
            return scope()->findSymbolOfType<Function>(name());
        }
        else
        {
            return this;
        }
    }

    Function* Function::nextFunctionOverload()
    {
        for (Symbol* s = nextOverload(); s; s = s->nextOverload())
        {
            if (Function* f = dynamic_cast<Function*>(s))
            {
                return f;
            }
        }

        return 0;
    }

    const Function* Function::nextFunctionOverload() const
    {
        for (const Symbol* s = nextOverload(); s; s = s->nextOverload())
        {
            if (const Function* f = dynamic_cast<const Function*>(s))
            {
                return f;
            }
        }

        return 0;
    }

    bool Function::isFunctionOverloaded() const
    {
        const Function* f = firstFunctionOverload();

        if (f != this)
        {
            return true;
        }
        else
        {
            return f->nextFunctionOverload() == 0 ? false : true;
        }
    }

    static Function::Attribute purity(const Function* self, const Node* n)
    {
        if (!n)
            return Function::None;

        if (const Function* f = dynamic_cast<const Function*>(n->symbol()))
        {
            //
            //  If its recursive assume its pure. If you find out its not
            //  -- no harm done.
            //

            if (f == self)
                return Function::Pure;

            //
            //  Check if the node function is pure
            //

            if (f->isPure() || f->maybePure())
            {
                bool maybe = false;

                for (int i = 0, s = n->numArgs(); i < s; i++)
                {
                    switch (purity(self, n->argNode(i)))
                    {
                    case Function::Pure:
                        break;
                    case Function::MaybePure:
                        maybe = true;
                        break;
                    default:
                    case Function::None:
                        return Function::None;
                    }
                }

                if (maybe)
                    return Function::MaybePure;
                return f->isPure() ? Function::Pure : Function::MaybePure;
            }
            else
            {
                return Function::None;
            }
        }
        else if (const GlobalVariable* v =
                     dynamic_cast<const GlobalVariable*>(n->symbol()))
        {
            //
            //  If its either taking the address of or value of a global
            //  variable its not pure.
            //

            return Function::None;
        }
        else if (const MemberVariable* v =
                     dynamic_cast<const MemberVariable*>(n->symbol()))
        {
            //
            //  If its taking the address of a MemberVariable its not
            //  pure.
            //

            if (n->func() == n->type()->machineRep()->referenceMemberFunc())
            {
                return Function::None;
            }
            else
            {
                return Function::Pure;
            }
        }
        else
        {
            return Function::Pure;
        }
    }

    void Function::setReturnType(const Type* t)
    {
        //
        //  Check for missing return type
        //

        Signature& sig = *(Signature*)_signature;

        if (sig.resolved() && !sig[0].symbol)
        {
            sig[0].symbol = t;
        }
        else
        {
            sig[0].name = t->fullyQualifiedName().nameRef();
            // abort();
        }
    }

    void Function::setBody(Node* n)
    {
        _code = n;

        Signature& sig = *(Signature*)_signature;

        if (sig.resolved())
        {
            if (!sig[0].symbol)
                setReturnType(_code ? _code->type() : 0);
        }
        else
        {
            sig[0].name = _code ? _code->type()->fullyQualifiedName().nameRef()
                                : context()->internName("").nameRef();
        }

        //
        //  Determine function attributes
        //

        _noAttributes = false;

        switch (purity(this, _code))
        {
        case Pure:
            _mapped = true;
            _sideEffects = false;
            _dependentSideEffects = false;
            break;
        case MaybePure:
            _mapped = true;
            _sideEffects = true;
            _dependentSideEffects = true;
            break;
        default:
            _mapped = false;
            _sideEffects = true;
            _dependentSideEffects = false;
            break;
        }
    }

} // namespace Mu
