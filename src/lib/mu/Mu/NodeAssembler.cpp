//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ASTNode.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Class.h>
#include <Mu/Context.h>
#include <Mu/Exception.h>
#include <Mu/FreeVariable.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/GlobalVariable.h>
#include <Mu/Interface.h>
#include <Mu/Language.h>
#include <Mu/ListType.h>
#include <Mu/MachineRep.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/Namespace.h>
#include <Mu/NodeAssembler.h>
#include <Mu/NodePatch.h>
#include <Mu/NodeSimplifier.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/StackVariable.h>
#include <Mu/StructType.h>
#include <Mu/Symbol.h>
#include <Mu/SymbolTable.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/TupleType.h>
#include <Mu/Type.h>
#include <Mu/TypePattern.h>
#include <Mu/TypeVariable.h>
#include <Mu/Unresolved.h>
#include <Mu/VariantTagType.h>
#include <Mu/VariantType.h>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <vector>

namespace Mu
{
    using namespace std;
    using namespace stl_ext;

    void NodeAssembler::dumpNodeStack()
    {
        NodeVector& n = _nodeStack;
        cout << "stack = (" << n.size() << ")";
        for (int i = 0; i < n.size(); i++)
        {
            cout << " (" << n[i]->symbol()->name() << ")=" << hex << n[i]
                 << dec;
        }
        cout << endl << flush;
    }

    NodeAssembler::NodeAssembler(Context* c, Process* p, Thread* t)
        : _context(c)
        , _process(p)
        , _error(false)
        , _thread(t)
        , _stackOffset(0)
        , _constReduce(true)
        , _prefixScope(0)
        , _allowUnresolved(true)
        , _line(1)
        , _char(1)
        , _initializerType(0)
        , _initializerGlobal(false)
        , _throwOnError(true)
        , _simplify(false)
        , _mythread(t ? false : true)
        , _suffixModule(0)
        , _scope(0)
        , _docmodule(0)
    {
        if (p)
        {
            if (!t)
                _thread = p->newApplicationThread();

            if (p->context())
            {
                assert(p->context() == c);
            }
        }

        pushScope(context()->globalScope());
    }

    NodeAssembler::~NodeAssembler()
    {
        if (_mythread && _process)
        {
            _process->releaseApplicationThread(_thread);
        }
    }

    Process* NodeAssembler::process() const { return _process; }

    Thread* NodeAssembler::thread() const { return _thread; }

    void NodeAssembler::reportError(const char* msg) const
    {
        reportWarning(msg);
        _error = true;
    }

    void NodeAssembler::reportError(Node* n, const char* msg) const
    {
        reportWarning(n, msg);
        _error = true;
    }

    void NodeAssembler::freportError(const char* msg, ...) const
    {
        char temp[256];
        va_list ap;
        va_start(ap, msg);
        vsprintf(temp, msg, ap);
        va_end(ap);

        reportError(temp);
    }

    void NodeAssembler::freportError(Node* n, const char* msg, ...) const
    {
        char temp[256];
        va_list ap;
        va_start(ap, msg);
        vsprintf(temp, msg, ap);
        va_end(ap);

        reportError(n, temp);
    }

    void NodeAssembler::reportWarning(const char* msg) const
    {
        if (lineNum() > 0)
        {
            context()->errorStream()
                << sourceName() << ", line " << lineNum() << ", char "
                << charNum() << ": " << msg << endl;
        }
        else
        {
            context()->errorStream() << sourceName() << ": " << msg;
        }
    }

    void NodeAssembler::reportWarning(Node* n, const char* msg) const
    {
        //
        //  This version tries to find an annotation for n and uses that
        //  if it exists.
        //

        int nline = lineNum();
        int nchar = charNum();
        string file = sourceName().c_str();

        if (context()->debugging()
            || dynamic_cast<const UnresolvedSymbol*>(n->symbol()))
        {
            AnnotatedNode* anode = static_cast<AnnotatedNode*>(n);
            nline = anode->linenum();
            nchar = anode->charnum();
            file = anode->sourceFileName().c_str();
        }

        if (nline > 0)
        {
            context()->errorStream() << file << ", line " << nline << ", char "
                                     << nchar << ": " << msg << endl;
        }
        else
        {
            context()->errorStream() << file << ": " << msg;
        }
    }

    void NodeAssembler::freportWarning(const char* msg, ...) const
    {
        char temp[256];
        va_list ap;
        va_start(ap, msg);
        vsprintf(temp, msg, ap);
        va_end(ap);

        reportWarning(temp);
    }

    void NodeAssembler::freportWarning(Node* n, const char* msg, ...) const
    {
        char temp[256];
        va_list ap;
        va_start(ap, msg);
        vsprintf(temp, msg, ap);
        va_end(ap);

        reportWarning(n, temp);
    }

    void NodeAssembler::pushModuleScope(Name name)
    {
        Module* m = 0;

        if (!(m = scope()->findSymbolOfType<Module>(name)))
        {
            m = new Module(context(), name.c_str());
            scope()->addSymbol(m);

            if (Object* o = retrieveDocumentation(name))
            {
                process()->addDocumentation(m, o);
            }
        }

        pushScope(m);
    }

    //----------------------------------------------------------------------
    //
    //  NodeList
    //

    NodeAssembler::NodeList NodeAssembler::newNodeList(Node* n)
    {
        size_t s = _nodeStack.size();
        _nodeStack.push_back(n);
        NodeList nl;
        nl._c = &_nodeStack;
        nl._start = s;
        nl._stride = 1;
        return nl;
    }

    NodeAssembler::NodeList NodeAssembler::newNodeList(const NodeVector& nodes)
    {
        NodeList nl = newNodeList(nodes.front());
        for (int i = 1; i < nodes.size(); i++)
            nl.push_back(nodes[i]);
        return nl;
    }

    NodeAssembler::NodeList NodeAssembler::emptyNodeList()
    {
        NodeList nl;
        nl._c = &_nodeStack;
        nl._start = _nodeStack.size();
        nl._stride = 1;
        return nl;
    }

    NodeAssembler::NodeList NodeAssembler::newNodeListFromArgs(Node* n)
    {
        NodeList nl = emptyNodeList();

        for (int i = 0, s = n->numArgs(); i < s; i++)
        {
            nl.push_back(n->argNode(i));
        }

        return nl;
    }

    void NodeAssembler::insertNodeAtFront(NodeList nl, Node* n)
    {
        nl.push_back(0);
        for (int i = nl.size() - 1; i > 0; i--)
            nl[i] = nl[i - 1];
        nl[0] = n;
    }

    void NodeAssembler::removeNodeList(NodeList l)
    {
        _nodeStack.resize(l.start());
    }

    bool NodeAssembler::containsNoOps(NodeList l) const
    {
        for (int i = 0; i < l.size(); i++)
        {
            if (!l[i] || l[i]->symbol() == context()->noop())
            {
                return true;
            }
        }

        return false;
    }

    //----------------------------------------------------------------------
    //
    //  SymbolList
    //

    NodeAssembler::SymbolList NodeAssembler::newSymbolList(Symbol* symbol)
    {
        size_t s = _symbolStack.size();
        _symbolStack.push_back(symbol);
        SymbolList sl;
        sl._c = &_symbolStack;
        sl._start = s;
        sl._stride = 1;
        return sl;
    }

    NodeAssembler::SymbolList NodeAssembler::emptySymbolList()
    {
        SymbolList sl;
        sl._c = &_symbolStack;
        sl._start = _symbolStack.size();
        sl._stride = 1;
        return sl;
    }

    void NodeAssembler::removeSymbolList(SymbolList l)
    {
        _symbolStack.resize(l.start());
    }

    void NodeAssembler::insertSymbolAtFront(SymbolList sl, Symbol* symbol)
    {
        sl.push_back(0);
        for (int i = sl.size() - 1; i > 0; i--)
            sl[i] = sl[i - 1];
        sl[0] = symbol;
    }

    NodeAssembler::NameList NodeAssembler::emptyNameList()
    {
        NameList nl;
        nl._c = &_nameStack;
        nl._start = _nameStack.size();
        nl._stride = 1;
        return nl;
    }

    NodeAssembler::NameList NodeAssembler::newNameList(Name n)
    {
        size_t s = _nameStack.size();
        _nameStack.push_back(n);
        NameList nl;
        nl._c = &_nameStack;
        nl._start = s;
        nl._stride = 1;
        return nl;
    }

    void NodeAssembler::removeNameList(NameList l)
    {
        _nameStack.resize(l.start());
    }

    DataNode* NodeAssembler::constant(const Type* type, Object* o) const
    {
        DataNode* dn =
            new DataNode(0, type->machineRep()->constantFunc(), type);
        // if (o) process()->constants().push_back(o);
        return dn;
    }

    DataNode* NodeAssembler::constant(const SymbolicConstant* sym) const
    {
        DataNode* node = constant(sym->type(), 0);
        node->_data = sym->value();
        return node;
    }

    Node* NodeAssembler::newNode(const Function* f, int nargs) const
    {
        Node* n = 0;

        if (f->hasHiddenArgument())
        {
            n = new DataNode(nargs, f->func(), f);
        }
        else if (context()->debugging())
        {
            n = new AnnotatedNode(nargs, f->func(), f, _line, _char,
                                  _sourceName);
        }
        else
        {
            n = new Node(nargs, f->func(), f);
        }

        return n;
    }

    Node* NodeAssembler::functionConstant(const Function* f, bool literal)
    {
        DataNode* n = 0;
        FunctionObject* o = 0;

        if (f->isFunctionOverloaded() && !f->isLambda() && !literal)
        {
            const FunctionType* atype = context()->ambiguousFunctionType();
            o = new FunctionObject(atype);
            // CONSTANT
            o->setFunction(f);
            n = new DataNode(0, atype->machineRep()->constantFunc(), atype);
        }
        else
        {
            o = new FunctionObject(f);
            // CONSTANT
            n = new DataNode(0, f->type()->machineRep()->constantFunc(),
                             f->type());

            if (f->numFreeVariables())
            {
                n->_data._Pointer = (Pointer)o;
                // process()->constants().push_back(o);

                NodeList nl = emptyNodeList();
                size_t nargs = f->numArgs();
                size_t ntotal = nargs + f->numFreeVariables();

                for (int i = 0; i < nargs; i++)
                {
                    nl.push_back(callBestOverloadedFunction(context()->noop(),
                                                            emptyNodeList()));
                }

                for (int i = nargs; i < ntotal; i++)
                {
                    const ParameterVariable* pv = f->parameter(i);

                    if (Variable* v = findTypeInScope<Variable>(pv->name()))
                    {
                        nl.push_back(dereferenceVariable(v));
                    }
                    else
                    {
                        freportError("unable to bind free variable \"%s\" in "
                                     "function \"%s\"",
                                     pv->name().c_str(),
                                     f->fullyQualifiedName().c_str());
                        return 0;
                    }
                }

                Node* rn = dynamicPartialEvalOrApply(n, nl, f->isLambda());
                removeNodeList(nl);
                return rn;
            }
        }

        n->_data._Pointer = (Pointer)o;
        // process()->constants().push_back(o);
        return n;
    }

    void NodeAssembler::prefixScope(const Symbol* symbol)
    {
        Mu::Symbol* s = const_cast<Symbol*>(symbol);

        if (Type* t = dynamic_cast<Type*>(s))
        {
            if (t && t->isReferenceType())
            {
                ReferenceType* rt = static_cast<ReferenceType*>(t);
                t = rt->dereferenceType();
            }

            s = t;
        }

        _prefixScope = s;
    }

    bool NodeAssembler::isConstant(Node* n) const
    {
        if (const Symbol* s = n->symbol())
        {
            if (dynamic_cast<const Type*>(s))
            {
                return true;
            }
        }

        return false;
    }

    Node* NodeAssembler::functionReduce(const Function* f, Node* n)
    {
        if (_constReduce && f->isDynamicActivation())
        {
            //
            //  This is peculiar case where a function object is being
            //  called. If the function object represents a non-anonymous
            //  function then just replace the whole mess with a call to
            //  that function -- in other words remove the indirection.
            //
            //          (f)(a, b, ...) => f(a, b, ...)
            //

            Node* fn = n->argNode(0);
            const FunctionType* t =
                dynamic_cast<const FunctionType*>(fn->type());

            if (fn->symbol() == t)
            {
                //
                //  Its a constant node
                //

                DataNode* dn = static_cast<DataNode*>(fn);
                FunctionObject* fobj =
                    reinterpret_cast<FunctionObject*>(dn->_data._Pointer);

                if (!fobj)
                    return n;

                const Function* ff = fobj->function();

                if (!ff->isLambda())
                {
                    //
                    //  This can be simplified
                    //

                    NodeList nl = emptyNodeList();

                    for (int i = 1; i < n->numArgs(); i++)
                    {
                        nl.push_back(n->argNode(i));
                    }

                    FunctionVector functions(1);
                    functions.front() = ff;
                    Node* nn = callBestFunction(functions, nl);
                    removeNodeList(nl);

                    if (!nn)
                    {
                        freportWarning(
                            "function indirection simplification failed");
                        return n;
                    }
                    else
                    {
                        n->releaseArgv();
                        fn->deleteSelf();
                        n->deleteSelf();
                        return nn;
                    }
                }
                else
                {
                    //
                    //  Is it possible to generate a symbol here?
                    //
                }
            }
        }

        return n;
    }

    Node* NodeAssembler::constReduce(Node* n) const
    {
        if (const Function* f = dynamic_cast<const Function*>(n->symbol()))
        {
            return constReduce(f, n);
        }

        return n;
    }

    Node* NodeAssembler::constReduce(const Function* f, Node* n) const
    {
        //
        //  We don't have enough information here to allow reduction of
        //  constructors that are non-primitive. Or rather: types which are
        //  passed by reference (not referentially transparent) cannot be
        //  reduced without additional information.
        //

        if ((f->isPure() || f->maybePure())
            && (!f->isConstructor()
                || (f->isConstructor() && !f->returnType()->isMutable()))
            && f != context()->noop()
            && n->type() != context()->unresolvedType() && _thread)
        {
            for (int i = 0, s = n->numArgs(); i < s; i++)
            {
                //
                //	If any argument is not a constant then bail
                //

                Node* argNode = n->argNode(i);
                if (!isConstant(argNode))
                    return n;

                if (f->maybePure())
                {
                    //
                    //  A bit more complex. If f is maybe pure then we need to
                    //  check to see if any function arguments are defintely
                    //  not pure.
                    //

                    if (const FunctionType* t =
                            dynamic_cast<const FunctionType*>(argNode->type()))
                    {
                        DataNode* dn = static_cast<DataNode*>(argNode);

                        if (FunctionObject* fobj =
                                reinterpret_cast<FunctionObject*>(
                                    dn->_data._Pointer))
                        {
                            const Function* fn = fobj->function();
                            if (!fn->isPure() && !fn->maybePure())
                                return n;
                        }
                        else
                        {
                            freportWarning(
                                "While trying to reduce function \"%s\", "
                                "argument %d which should have been a function "
                                "object "
                                "of type \"%s\" was found to be nil",
                                f->fullyQualifiedName().c_str(), i,
                                t->fullyQualifiedName().c_str());
                            return n;
                        }
                    }
                }
            }

            //
            //  Replace the node with a constant node holding the result value.
            //

            const Type* t = n->type();
            DataNode* dn = constant(t);

            try
            {
                dn->_data = t->nodeEval(n, *thread());

                //
                //  Make the new object a constant
                //

                if (!t->isReferenceType()
                    && t->machineRep() == PointerRep::rep())
                {
                    Object* o = reinterpret_cast<Object*>(dn->_data._Pointer);
                    // process()->constants().push_back(o);
                }
            }
            catch (Exception& e)
            {
                ostream& err = context()->errorStream();
                context()->error("While reducing \"");
                err << f->name() << "\" during parsing:" << endl;
                context()->error("");

                if (Object* o = thread()->exception())
                {
                    Value v(o);
                    o->type()->outputValue(err, v);
                }
                else
                {
                    err << " an exception was thrown";
                }

                err << "." << endl;
                return 0;
            }

            n->deleteSelf();
            return dn;
        }

        return n;
    }

    SymbolicConstant* NodeAssembler::newSymbolicConstant(Name name, Node* n)
    {
        if (const Type* t = dynamic_cast<const Type*>(n->symbol()))
        {
            try
            {
                Value v = t->nodeEval(n, *thread());

                if (t->machineRep() == PointerRep::rep())
                {
                    // process()->constants().push_back((Object*)(v._Pointer));
                }

                SymbolicConstant* s =
                    new SymbolicConstant(context(), name.c_str(), t, v);

                if (Object* o = retrieveDocumentation(s->name()))
                {
                    process()->addDocumentation(s, o);
                }

                return s;
            }
            catch (ProgramException& e)
            {
                ostream& err = context()->errorStream();
                context()->error("While creating symbolic constant \"");
                err << name << "\" during parsing:" << endl;
                context()->error("");

                if (Object* o = thread()->exception())
                {
                    Value v(o);
                    o->type()->outputValue(err, v);
                }
                else
                {
                    err << " an exception was thrown";
                }

                err << "." << endl;
                return 0;
            }
        }

        return 0;
    }

    Node* NodeAssembler::retainOrRelease(Node* node, bool retain) const
    {
        const Type* type = node->type();
        const char* fname = retain ? "__retain" : "__release";

        if (Name name = context()->lookupName(fname))
        {
            if (const Function* f = type->findSymbolOfType<Function>(name))
            {
                if (f->returnType() == type && f->numArgs() == 1
                    && f->argType(0) == type)
                {
                    Node* n = newNode(f, 1);
                    n->setArg(node, 0);
                    return n;
                }
                else
                {
                    freportError(
                        "Function \"%s\" of type %s does not have proper "
                        "signature",
                        f->name().c_str(), type->name().c_str());
                }
            }
        }

        freportError(
            "Type \"%s\" requires \"%s\" function but does not implement it",
            type->fullyQualifiedName().c_str(), fname);

        return node;
    }

    Node* NodeAssembler::retain(Node* n) const
    {
        return retainOrRelease(n, true);
    }

    Node* NodeAssembler::release(Node* n) const
    {
        return retainOrRelease(n, false);
    }

    Node* NodeAssembler::cast(Node* node, const Type* toType)
    {
        if (!node)
        {
            //
            //  This can happen during partial evaluation if things are
            //  seriously FUBARed. For example, there are native functions
            //  declared yet missing... Anyway, throwing here simply
            //  escapes back to the place where the evaluation was
            //  started which results in an error message.
            //

            throw NilArgumentException();
        }

        const Type* fromType = node->type();

        //
        //	Maybe no cast needed.
        //

        if (fromType == toType)
            return node;

        //
        //  Unresolved?
        //

        if (fromType == context()->unresolvedType())
        {
            NodeList nl = newNodeList(node);
            Node* n = unresolvableCast(toType->fullyQualifiedName(), nl);
            removeNodeList(nl);
            return n;
        }

        //
        //	Check for the case of an object being cast to one of its base
        //	classes. In this case, the cast is a no-op.
        //

        const Class* toClass = dynamic_cast<const Class*>(toType);
        const Class* fromClass = dynamic_cast<const Class*>(fromType);
        const Interface* toInterface = dynamic_cast<const Interface*>(toType);
        const Interface* fromInterface =
            dynamic_cast<const Interface*>(fromType);

        bool match = toType->match(fromType);

        if (match)
        {
            if (!toClass || !fromClass)
            {
                return node;
            }
            else if (toClass && fromClass)
            {
                if (fromClass->substitutable(toClass))
                {
                    return node;
                }
            }
        }

        if ((toInterface && fromClass) || (toClass && fromInterface)
            || (match && toClass && fromClass))
        {
            //
            //  Use the dynamic cast
            //

            DataNode* tnode = constant(toType);
            tnode->_data._Pointer = 0;

            Node* cast = newNode(context()->dynamicCast(), 2);
            cast->setArg(tnode, 0);
            cast->setArg(node, 1);

            if (_constReduce)
                return constReduce(context()->dynamicCast(), cast);
            else
                return cast;
        }

        //
        //	Find an overloaded symbol which casts directly to toType. The
        // while 	loop allows checkin inside the type for constructors in
        // its scope.
        //

        const Function* function = 0;
        const Symbol* startSym = toType;

        while (!function && startSym)
        {
            for (const Symbol* s = startSym->firstOverload(); s;
                 s = s->nextOverload())
            {
                if (const Function* f = dynamic_cast<const Function*>(s))
                {
                    if (f->isCast())
                    {
                        const Type* atype = f->argType(0);

                        if (atype == fromType)
                        {
                            function = f;
                            break;
                        }
                        else if (atype->isTypePattern())
                        {
                            const TypePattern* pt =
                                static_cast<const TypePattern*>(atype);

                            if (pt->match(fromType))
                            {
                                function = f;
                                // don't break since its not exact
                            }
                        }
                    }
                }
            }

            if (startSym == toType)
            {
                startSym = startSym->findSymbol(startSym->name());
            }
            else
            {
                startSym = 0;
            }
        }

        //
        //	If no function was found and the thing being cast to is a
        //	derived class of the thing being cast from, then use a dynamic
        //	cast.
        //

        if (!function && toClass && fromClass)
        {
            if (toClass->isA(fromClass))
            {
                DataNode* tnode = constant(toClass);
                tnode->_data._Pointer = 0;

                Node* cast = newNode(context()->dynamicCast(), 2);
                cast->setArg(tnode, 0);
                cast->setArg(node, 1);

                if (_constReduce)
                    return constReduce(context()->dynamicCast(), cast);
                else
                    return cast;
            }
        }

        if (function)
        {
            Node* cast = newNode(function, function->numArgs());
            cast->setArgs(&node, 1);

            if (_constReduce)
                return constReduce(function, cast);
            else
                return cast;
        }
        else
        {
            return 0;
        }
    }

    Node* NodeAssembler::unaryOperator(const char* op, Node* node1)
    {
        NodeList nl = newNodeList(node1);
        Node* n = callBestFunction(op, nl);
        removeNodeList(nl);
        return n;
    }

    Node* NodeAssembler::binaryOperator(const char* op, Node* node1,
                                        Node* node2)
    {
        NodeList nl = newNodeList(node1);
        nl.push_back(node2);
        Node* n = callBestFunction(op, nl);
        removeNodeList(nl);

        return n;
    }

    Node* NodeAssembler::assignmentOperator(const char* op, Node* node1,
                                            Node* node2)
    {
        Node* n = 0;

        if (node1->type() == context()->unresolvedType()
            || node2->type() == context()->unresolvedType())
        {
            //
            //  Just pass it to binaryOperator() to take care of it
            //

            // n = binaryOperator(op, node1, node2);

            n = new ASTAssign(*this, context()->unresolvedAssignment(), node1,
                              node2);
        }
        else
        {
            if (const ReferenceType* rtype =
                    dynamic_cast<const ReferenceType*>(node1->type()))
            {
                if (Node* n2 = cast(node2, rtype->dereferenceType()))
                {
                    n = binaryOperator(op, node1, n2);
                }
                else
                {
                    freportError(
                        "cannot cast \"%s\" to \"%s\" for assignment.",
                        node2->type()->fullyQualifiedName().c_str(),
                        rtype->dereferenceType()->fullyQualifiedName().c_str());
                }
            }
            else
            {
                //
                //  Can this happen?
                //
                freportError(
                    "illegal assignment from \"%s\" to \"%s\" in this context.",
                    node2->type()->fullyQualifiedName().c_str(),
                    node1->type()->fullyQualifiedName().c_str());
            }
        }

        return n;
    }

    void NodeAssembler::setProcessRoot(Node* node)
    {
        // assert(_process->_rootNode==0);
        if (process()->_rootNode)
        {
            process()->_rootNode->deleteSelf();
        }

        process()->_rootNode = node;

        if (_throwOnError && _error)
        {
            if (node)
                node->deleteSelf();
            throw ParseSyntaxException();
        }
    }

    Node* NodeAssembler::memberOperator(const char* op, Node* n, NodeList nl)
    {
        const Type* type = n->type();
        const Type* rtype = 0;
        Node* rn = 0;
        Name opName = context()->lookupName(op);
        bool unresolved = false;

        if (type == context()->unresolvedType())
        {
            unresolved = true;
        }
        else
        {
            for (int i = 0; i < nl.size(); i++)
            {
                if (nl[i]->type() == context()->unresolvedType())
                {
                    unresolved = true;
                    break;
                }
            }
        }

        if (unresolved)
        {
            insertNodeAtFront(nl, n);
            markCurrentFunctionUnresolved();
            return new ASTIndexMember(*this, nl.size(), &nl.front(),
                                      context()->unresolvedCall());

            // unresolvableCall(opName, nl);
        }

        if (type->isReferenceType())
        {
            rtype = type;
            type = static_cast<const ReferenceType*>(type)->dereferenceType();
        }

        const Class* c = dynamic_cast<const Class*>(type);
        const Interface* i = dynamic_cast<const Interface*>(type);
        Name iname = opName;

        if (c || i)
        {
            if (const Function* f = type->findSymbolOfType<Function>(iname))
            {
                rn = callMethod(f, dereferenceLValue(n), nl);

                if (!rn)
                {
                    freportError("operator%s argument mis-match", op);
                    return 0;
                }
            }
            else
            {
                freportError("operator%s not defined for type %s", op,
                             type->fullyQualifiedName().c_str());
                return 0;
            }
        }
        else
        {
            if (const Function* f = type->findSymbolOfType<Function>(iname))
            {
                insertNodeAtFront(nl, n);

                if (!(rn = callBestOverloadedFunction(f, nl)))
                {
                    freportError("operator%s argument mis-match", op);
                    return 0;
                }
            }
        }

        return rn;
    }

    Node* NodeAssembler::callBestFunction(const char* name, NodeList nodes)
    {
        if (Name n = context()->lookupName(name))
        {
            FunctionVector functions;

            if (findTypesInScope<Function>(n, functions))
            {
                return callBestFunction(functions, nodes);
            }
        }

        return 0;
    }

    Node* NodeAssembler::callBestOverloadedFunction(const Function* func,
                                                    NodeList nodes)
    {
        FunctionVector functions;

        for (const Symbol* s = func->firstOverload(); s; s = s->nextOverload())
        {
            if (const Function* f = dynamic_cast<const Function*>(s))
            {
                functions.push_back(f);
            }
        }

        return callBestFunction(functions, nodes);
    }

    Node* NodeAssembler::callFunction(const Function* F, NodeList nodes)
    {
        int nargs = F->numArgs();
        int nfreevars = F->numFreeVariables();
        bool fretains = F->isRetaining();
        bool native = F->native();

        if (nargs + nfreevars != nodes.size())
        {
            FunctionVector funcs(1);
            funcs[0] = F;
            showOptions(funcs, nodes);
            return 0;
        }

        Context::TypeVector argTypes(nodes.size());
        Context::MatchType matchType = Context::TypeMatch;

        Node* node = newNode(F, nodes.size());

        for (int i = 0; i < nodes.size(); i++)
        {
            Node* argNode = nodes[i];
            const Type* t = argNode ? argNode->type() : 0;

            if (!t || t->isUnresolvedType())
            {
                freportError("error calling NodeAssembler::callFunction(): "
                             "argument %d to function \"%s\" is unresolved",
                             i, F->fullyQualifiedName().c_str());
                return 0;
            }

            argTypes[i] = t;

            if (t != F->argType(i))
            {
                argNode = cast(argNode, F->argType(i));
            }

            //
            //  If the argument type does its own memory management, and this
            //  function retains arguments, then insert a retaining node here.
            //

            if ((fretains || !native) && argNode->type()->isSelfManaged())
            {
                argNode = retain(argNode);
            }

            node->setArg(argNode, i);
        }

        //
        //  Reduce if possible.
        //

        if (F->isDynamicActivation())
        {
            node = functionReduce(F, node);
            F = static_cast<const Function*>(node->symbol());
        }

        if (_constReduce && !F->hasUnresolvedStubs())
        {
            return constReduce(F, node);
        }
        else
        {
            return node;
        }
    }

    Node* NodeAssembler::callExactOverloadedFunction(const Function* Fin,
                                                     NodeList nodes)
    {
        Context::TypeVector argTypes(nodes.size());
        const Function* F = 0;

        for (int i = 0; i < nodes.size(); i++)
        {
            Node* argNode = nodes[i];
            const Type* t = argNode ? argNode->type() : 0;

            if (!t || t->isUnresolvedType())
            {
                freportError("error calling "
                             "NodeAssembler::callExactOverloadedFunction(): "
                             "argument %d to function \"%s\" is unresolved",
                             i, F->fullyQualifiedName().c_str());
                return 0;
            }

            argTypes[i] = t;
        }

        bool found = false;

        for (const Symbol* s = Fin->firstOverload(); s; s = s->nextOverload())
        {
            if (F = dynamic_cast<const Function*>(s))
            {
                size_t ntotal = F->numArgs() + F->numFreeVariables();
                found = true;

                if (ntotal == nodes.size())
                {
                    for (size_t q = 0; q < argTypes.size(); q++)
                    {
                        if (F->argType(q) != argTypes[q])
                        {
                            found = false;
                            break;
                        }
                    }
                }
                else
                {
                    found = false;
                }

                if (found)
                    break;
            }
        }

        if (!found)
            return 0;

        // Node* node = new Node(nodes.size(), NodeFunc(0), F);
        Node* node = newNode(F, nodes.size());
        node->set(F, F->func(node));
        for (size_t i = 0; i < nodes.size(); i++)
            node->setArg(nodes[i], i);
        return node;
    }

    Node* NodeAssembler::callBestFunction(const FunctionVector& functions,
                                          NodeList nodes)
    {
        Context::TypeVector argTypes(nodes.size());
        bool unresolvedArgument = false;

        for (int i = 0; i < nodes.size(); i++)
        {
            Node* argNode = nodes[i];

            if (argNode)
            {
                argTypes[i] = argNode->type();

                if (!argTypes[i] || argTypes[i]->isUnresolvedType())
                {
                    if (_allowUnresolved)
                    {
                        return unresolvableCall(
                            functions.front()->fullyQualifiedName(), nodes);
                    }
                    else
                    {
                        showOptions(functions, nodes);
                        return 0;
                    }
                }
            }
            else
            {
                argTypes[i] = 0;
            }
        }

        Context::MatchType matchType = Context::TypeMatch;

        if (const Function* function = context()->matchSpecializedFunction(
                process(), thread(), functions, argTypes, matchType))
        {
            if (matchType == Context::TypeMatch)
            {
                if (nodes.size() != 2)
                {
                    reportError(
                        "TypeMatch not implemented for function signatures");
                    return 0;
                }
                else
                {
                    swap(nodes[0], nodes[1]);
                    swap(argTypes[0], argTypes[1]);
                }
            }

            int fmax = function->maximumArgs();
            int fmin = function->minimumArgs();
            int freq = function->requiredArgs();
            int freevars = function->numFreeVariables();
            bool fretains = function->isRetaining();
            bool native = function->native();

            int totalArgs = freevars;

            if (matchType == Context::BestPartialMatch
                && function->hasParameters())
            {
                totalArgs += function->isVariadic() ? nodes.size() : fmax;
            }
            else
            {
                totalArgs += nodes.size();
            }

            Node* node = newNode(function, totalArgs);

            Function::TypeVector types;
            function->expandArgTypes(types, nodes.size());

            for (int i = 0; i < nodes.size(); i++)
            {
                Node* arg = nodes[i];

                if (i < fmax)
                {
                    const Type* argType = types[i];

                    if (!argType->isTypePattern())
                    {
                        arg = cast(arg, argType);
                    }
                }

                //
                //  If the argument type does its own memory
                //  management, and this function retains arguments,
                //  then insert a retaining node here.
                //

                if ((fretains || !native) && arg->type()->isSelfManaged())
                {
                    arg = retain(arg);
                }

                node->setArg(arg, i);
            }

            if (matchType == Context::BestPartialMatch
                && function->hasParameters())
            {
                for (int i = nodes.size(); i < fmax; i++)
                {
                    const ParameterVariable* p = function->parameter(i);
                    DataNode* n = constant(p->storageClass());
                    n->_data = p->defaultValue();
                    node->setArg(n, i);
                }
            }

            if (freevars)
            {
                //
                //  Need to add the extra args here
                //

                for (int i = fmax; i < totalArgs; i++)
                {
                    const ParameterVariable* pv = function->parameter(i);

                    if (Variable* v = findTypeInScope<Variable>(pv->name()))
                    {
                        Node* nn = dereferenceVariable(v);
                        node->setArg(nn, i);
                    }
                    else
                    {
                        freportError("unable to bind free variable \"%s\" in "
                                     "function \"%s\"",
                                     pv->name().c_str(),
                                     function->fullyQualifiedName().c_str());
                        return 0;
                    }
                }
            }

            //
            //  If the function is context dependent, then give it a
            //  chance to choose an appropriate function
            //

            if (function->isContextDependent())
            {
                node->_func = function->func(node);
            }

            //
            //  Reduce if possible.
            //

            if (function->isDynamicActivation())
            {
                node = functionReduce(function, node);
                function = static_cast<const Function*>(node->symbol());
            }

            if (_constReduce && !function->hasUnresolvedStubs())
            {

                if (_simplify)
                {
                    NodeSimplifier simplify(this);
                    node = simplify(node);
                }

                return constReduce(function, node);
            }
            else
            {
                return node;
            }
        }
        else
        {
            if (MemberFunction* func = findScopeOfType<MemberFunction>())
            {
                //
                // If there's a member function peer to the current function
                // and it takes one additional argument then try and insert
                // a node.
                //

                FunctionVector mfuncs;
                const Class* fscope = dynamic_cast<const Class*>(func->scope());

                for (int i = 0; i < functions.size(); i++)
                {
                    const Function* g = functions[i];

                    if ((g->minimumArgs() <= nodes.size() + 1
                         && g->maximumArgs() >= nodes.size() + 1)
                        && (g->scope() == func->scope()
                            || (fscope
                                && fscope->isA(
                                    dynamic_cast<const Class*>(g->scope())))))
                    {
                        mfuncs.push_back(g);
                    }
                }

                if (mfuncs.size())
                {
                    insertNodeAtFront(nodes,
                                      dereferenceVariable(func->parameter(0)));
                    return callBestFunction(mfuncs, nodes);
                }
            }
        }

        if (_allowUnresolved)
        {
            //
            //  Create an unresolved call. Assuming this is a first pass,
            //  this will generate a special symbol which should later be
            //  replaced.
            //

            // return unresolvableCall(functions.front()->fullyQualifiedName(),
            // nodes);
            return unresolvableCall(functions.front()->name(), nodes);
        }
        else
        {
            //
            //  Error -- no match -- try to output alternatives so the
            //  user can at least see what the problem might be.
            //

            showOptions(functions, nodes);
            return 0;
        }

        return 0;
    }

    void NodeAssembler::showOptions(const FunctionVector& functions,
                                    NodeList nodes)
    {
        ostream& err = context()->errorStream();
        context()->error("No match found for function \"");
        err << functions.front()->name() << "\" with " << nodes.size()
            << " argument" << (nodes.size() == 1 ? "" : "s") << ": ";

        for (int i = 0; i < nodes.size(); i++)
        {
            if (i)
                err << ", ";

            if (nodes[i])
            {
                if (nodes[i]->type())
                {
                    err << nodes[i]->type()->fullyQualifiedName();
                }
                else
                {
                    err << "unresolved type";
                }
            }
            else
            {
                err << "*unresolved*";
            }
        }

        err << endl;

        int count = 1;

        for (int i = 0; i < functions.size(); i++)
        {
            const Function* f = functions[i];
            err << "  Option #" << count++ << ": ";
            f->output(err);
            err << endl;
        }

        err << flush;
    }

    void NodeAssembler::showArgs(NodeList nodes)
    {
        ostream& err = context()->errorStream();

        for (int i = 0; i < nodes.size(); i++)
        {
            if (i)
                err << ", ";

            if (nodes[i]->type())
            {
                err << nodes[i]->type()->fullyQualifiedName();
            }
            else
            {
                err << "unresolved type";
            }
        }

        err << flush;
    }

    Node* NodeAssembler::unresolvableCall(Name name, NodeList nl,
                                          const Symbol* s)
    {
        ASTCall* astcall = new ASTCall(
            *this, nl.size(), s ? s : context()->unresolvedCall(), name);

        if (nl.size())
            astcall->setArgs(&nl.front(), nl.size());

        markCurrentFunctionUnresolved();
        return astcall;
    }

    Node* NodeAssembler::unresolvableStackDeclaration(Name n)
    {
        Variable::Attributes rw = Variable::ReadWrite;
        Variable::Attributes rwi = rw | Variable::ImplicitType;

        StackVariable* svar =
            new StackVariable(context(), n.c_str(), context()->unresolvedType(),
                              _stackOffset++, rwi);

        _stackVariables.push_back(svar);

        ASTNode* astnode = new ASTStackDeclaration(
            *this, context()->unresolvedDeclaration(), n, svar);

        markCurrentFunctionUnresolved();

        return astnode;
    }

    Node* NodeAssembler::unresolvableCast(Name name, NodeList nl)
    {
        ASTCast* astnode =
            new ASTCast(*this, nl.size(), context()->unresolvedCast(), name);

        if (nl.size())
            astnode->setArgs(&nl.front(), nl.size());

        markCurrentFunctionUnresolved();
        return astnode;
    }

    Node* NodeAssembler::unresolvableMemberCall(Name name, NodeList nl)
    {
        ASTMemberCall* astnode = new ASTMemberCall(
            *this, nl.size(), context()->unresolvedMemberCall(), name);

        if (nl.size())
            astnode->setArgs(&nl.front(), nl.size());
        markCurrentFunctionUnresolved();
        return astnode;
    }

    Node* NodeAssembler::unresolvableConstructor(const Type* t, NodeList nl)
    {
        DataNode* dn = new DataNode(nl.size(), BaseFunctions::unresolved,
                                    context()->unresolvedConstructor());

        dn->_data._Pointer = (Pointer)t;

        /* AJG - bugfix */
        if (nl.size() > 0)
        {
            dn->setArgs(&nl.front(), nl.size());
        }

        markCurrentFunctionUnresolved();
        return dn;
    }

    Node* NodeAssembler::unresolvableStackReference(const StackVariable* sv)
    {
        Symbol* s = nonAnonymousScope();

        if (dynamic_cast<Function*>(s))
        {
            ASTStackReference* astnode = new ASTStackReference(
                *this, 0, context()->unresolvedStackReference(), sv);

            markCurrentFunctionUnresolved();
            return astnode;
        }
        else
        {
            freportError("Unresolved reference to \"%s\"", sv->name().c_str());
            return 0;
        }
    }

    Node* NodeAssembler::unresolvableReference(Name name)
    {
        Symbol* s = nonAnonymousScope();
        markCurrentFunctionUnresolved();
        return new ASTReference(*this, context()->unresolvedStackReference(),
                                name);
    }

    Node* NodeAssembler::callMethod(const Function* method, NodeList nl)
    {
        Node* node = callBestOverloadedFunction(method, nl);

        if (node)
        {
            if (const Interface* i =
                    dynamic_cast<const Interface*>(method->scope()))
            {
                node->_func = node->type()->machineRep()->invokeInterfaceFunc();
                // Node* n = newNode(context()->instanceCache(), 1);
                // n->setArg(node->_argv[0], 0);
                // node->_argv[0] = n;
            }
            else if (const MemberFunction* nf =
                         dynamic_cast<const MemberFunction*>(node->symbol()))
            {
                node->_func = node->type()->machineRep()->callMethodFunc();
                // Node* n = newNode(context()->instanceCache(), 1);
                // n->setArg(node->_argv[0], 0);
                // node->_argv[0] = n;
            }
        }

        removeNodeList(nl);
        return node;
    }

    Node* NodeAssembler::callMethod(const Function* method, Node* self,
                                    NodeList arguments)
    {
        size_t s = arguments.size();
        NodeList nl = newNodeList(self);

        for (int i = 0; i < s; i++)
        {
            nl.push_back(arguments[i]);
        }

        return callMethod(method, nl);
    }

    Node* NodeAssembler::methodThunk(const MemberFunction* f, Node* self)
    {
        NodeList nl = newNodeList(self);
        Node* n = functionConstant(f, true);

        for (size_t i = 1; i < f->numArgs() + f->numFreeVariables(); i++)
        {
            Node* noop =
                callBestOverloadedFunction(context()->noop(), emptyNodeList());
            nl.push_back(noop);
        }

        Node* rn = dynamicPartialEvalOrApply(n, nl, false, true);
        removeNodeList(nl);

        return rn;
    }

    Node* NodeAssembler::call(Node* node, NodeList nl, bool dynamicDispatch)
    {
        Node* n = dereferenceLValue(node);
        const Type* type = n->type();
        const Class* c = dynamic_cast<const Class*>(type);
        const Interface* i = dynamic_cast<const Interface*>(type);
        Node* rn = 0;

        if (c || i)
        {
            Name iname = context()->lookupName("()");
            if (const Function* f = type->findSymbolOfType<Function>(iname))
            {
                if (containsNoOps(nl))
                {
                    rn = dynamicPartialEvalOrApply(n, nl, false,
                                                   dynamicDispatch);
                }
                else
                {
                    rn = callMethod(f, n, nl);
                }

                if (!rn)
                {
                    freportError("Function argument mis-match");
                }
            }
        }
        else
        {
            if (n->type() == context()->unresolvedType())
            {
                if (n->symbol() == context()->unresolvedMemberReference()
                    || (n->symbol() == context()->unresolvedDereference()
                        && n->argNode(0)->symbol()
                               == context()->unresolvedMemberReference()))
                {
                    Name iname = context()->lookupName("()");
                    insertNodeAtFront(nl, n);
                    return unresolvableMemberCall(iname, nl);
                }
                else if (containsNoOps(nl))
                {
                    cout << "implement this" << endl;
                }
                else
                {
                    freportError(
                        "unresolved in this context (needs implementing?)",
                        n->type()->fullyQualifiedName().c_str());
                }
            }
            else if (!(rn = callBestFunction("()", nl)))
            {
                freportError("operator() is not defined for type %s",
                             n->type()->fullyQualifiedName().c_str());
            }
        }

        return rn;
    }

    Node* NodeAssembler::call(const Symbol* sym, NodeList nl,
                              bool dynamicDispatch)
    {
        stl_ext::StaticPointerCast<Symbol, Function> castOp;
        FunctionVector functions;
        Node* rnode = 0;
        bool found;

        if (const Type* type = dynamic_cast<const Type*>(sym))
        {
            //
            //  First try and locate a constructor in the type. This will
            //  require an additional argument for allocation.
            //

            if (type->isTypeVariable())
            {
                //
                //  Make an unresolved call
                //

                return unresolvableConstructor(type, nl);
            }

            if (!type->isPrimitiveType())
            {
                //
                //  Find constructors in the type scope. Constructors
                //  should have a first argument of a "raw" allocated
                //  object which we add here.
                //

                Symbol::SymbolVector symbols =
                    ((Symbol*)sym)->findSymbolsOfType<Function>(sym->name());

                functions.resize(symbols.size());
                transform(symbols.begin(), symbols.end(), functions.begin(),
                          castOp);

                if (!functions.empty())
                {
                    Name allocName = context()->lookupName("__allocate");

                    if (const Function* alloc =
                            sym->findSymbolOfType<Function>(allocName))
                    {
                        Node* a =
                            callBestOverloadedFunction(alloc, emptyNodeList());
                        insertNodeAtFront(nl, a);
                        return callBestFunction(functions, nl);
                    }
                }
            }

            if (functions.empty())
            {
                Symbol::SymbolVector symbols =
                    ((Symbol*)sym)
                        ->scope()
                        ->findSymbolsOfType<Function>(sym->name());
                functions.resize(symbols.size());
                transform(symbols.begin(), symbols.end(), functions.begin(),
                          castOp);
            }

            found = !functions.empty();
        }
        else
        {
            found = findTypesInScope<Function>(sym->name(), functions);
        }

        if (found)
        {
            if (containsNoOps(nl))
            {
                Context::TypeVector types(nl.size());
                Context::MatchType matchType = Context::TypeMatch;

                for (int i = 0; i < nl.size(); i++)
                {
                    if (!nl[i])
                        return 0;
                    const Type* t = nl[i]->type();

                    if (t == context()->voidType())
                    {
                        types[i] = context()->matchAnyType();
                    }
                    else
                    {
                        types[i] = t;
                    }
                }

                if (const Function* f = context()->matchSpecializedFunction(
                        process(), thread(), functions, types, matchType))
                {
                    Node* rn = functionConstant(f, true);
                    const Type* ft = rn->type();
                    return dynamicPartialEvalOrApply(rn, nl, false,
                                                     dynamicDispatch);
                }
            }
            else
            {
                size_t fcount = 0;
                size_t mcount = 0;

                for (size_t i = 0; i < functions.size(); i++)
                {
                    if (dynamic_cast<const MemberFunction*>(functions[i]))
                        mcount++;
                    else
                        fcount++;
                }

                if (fcount && mcount)
                {
                    //
                    //  This ugliness is due to C++ style OO. Its possible
                    //  to have a function in the scope of the class (not
                    //  a method) which could also match. So we need to
                    //  test them all.
                    //

                    Context::TypeVector types(nl.size());
                    Context::MatchType matchType = Context::TypeMatch;

                    for (int i = 0; i < nl.size(); i++)
                    {
                        if (!nl[i])
                            return 0;
                        const Type* t = nl[i]->type();

                        if (t == context()->voidType())
                        {
                            types[i] = context()->matchAnyType();
                        }
                        else
                        {
                            types[i] = t;
                        }
                    }

                    if (const Function* f = context()->matchSpecializedFunction(
                            process(), thread(), functions, types, matchType))
                    {
                        if (const MemberFunction* mf =
                                dynamic_cast<const MemberFunction*>(f))
                        {
                            if (dynamicDispatch)
                            {
                                return callMethod(mf, nl);
                            }
                            else
                            {
                                return callFunction(mf, nl);
                            }
                        }
                        else
                        {
                            return callFunction(f, nl);
                        }
                    }
                }

                if (const MemberFunction* mf =
                        dynamic_cast<const MemberFunction*>(sym))
                {
                    if (dynamicDispatch)
                    {
                        return callMethod(mf, nl);
                    }
                    else
                    {
                        return callFunction(mf, nl);
                    }
                }
                else
                {
                    return callBestFunction(functions, nl);
                }
            }
        }
        else if (Variable* v = findTypeInScope<Variable>(sym->name()))
        {
            Node* rn;

            if (!(rn = referenceVariable(v)))
            {
                freportError("Unable to reference variable \"%s\"",
                             v->fullyQualifiedName().c_str());
                return 0;
            }

            //
            //  At the very least, operator() should be defined
            //

            Name iname = context()->lookupName("()");
            const Type* t = v->storageClass();

            if (const Function* f = t->findSymbolOfType<Function>(iname))
            {
                if (containsNoOps(nl))
                {
                    return dynamicPartialEvalOrApply(rn, nl, false,
                                                     dynamicDispatch);
                }
                else if (dynamicDispatch)
                {
                    return callMethod(f, dereferenceLValue(rn), nl);
                }
                else
                {
                    NodeList nnl = newNodeList(dereferenceLValue(rn));
                    for (size_t i = 0; i < nl.size(); i++)
                        nnl.push_back(nl[i]);
                    Node* rn = callFunction(f, nnl);
                    removeNodeList(nnl);
                    return rn;
                }

                reportError("Function arguments mis-match");
                return 0;
            }
            else if (rn->type() == context()->unresolvedType())
            {
                insertNodeAtFront(nl, rn);
                return unresolvableCall(iname, nl);
            }
        }
        else
        {
            reportError("expecting a function");
            return 0;
        }

        return rnode;
    }

    Node* NodeAssembler::dynamicPartialEvalOrApply(Node* self,
                                                   NodeList arguments,
                                                   bool reduce,
                                                   bool dynamicDispatch)
    {
        self = dereferenceLValue(self);
        const FunctionType* outType =
            dynamic_cast<const FunctionType*>(self->type());
        if (!outType)
            return 0;

        const Signature* outSig = outType->signature();
        if (!outSig->resolved())
            outSig->resolve(context());

#if 0
    const Function* F = reduce ? context()->dynamicPartialEval() :
                                 context()->dynamicPartialApply();
#endif

        const Function* F = context()->curry();

        // Node *fn = new Node(arguments.size() + 3, F->func(), F);
        Node* fn = newNode(F, arguments.size() + 3);
        Signature* s = new Signature;
        s->push_back(outSig->returnType());

        int fmax = outSig->types().size() - 1;
        const Function* selfFunc = 0;

        if (isConstant(self))
        {
            DataNode* dn = static_cast<DataNode*>(self);
            FunctionObject* fobj =
                reinterpret_cast<FunctionObject*>(dn->_data._Pointer);
            selfFunc = fobj->function();
        }

        for (int i = 0; i < arguments.size(); i++)
        {
            Node* n = arguments[i];

            if (i < fmax)
            {
                const Type* atype = outSig->argType(i);

                if (n->symbol() != context()->noop())
                {
                    n = cast(n, atype);
                }
                else
                {
                    s->push_back(atype);
                }
            }
            else
            {
                assert(selfFunc);
                const Type* atype = selfFunc->parameter(i)->storageClass();
                n = cast(n, atype);
            }

            fn->setArg(n, i + 3);
        }

        //
        //  Make the result type node
        //

        const Type* rtype = context()->functionType(s);
        DataNode* dn =
            new DataNode(0, rtype->machineRep()->constantFunc(), rtype);
        fn->setArg(dn, 0);
        fn->setArg(self, 1);

        DataNode* dd = constant(context()->boolType());
        dd->_data._bool = dynamicDispatch;
        fn->setArg(dd, 2);

        if (_constReduce)
            return constReduce(F, fn);
        return fn;
    }

    void NodeAssembler::newStackFrame()
    {
        _stackVariableStack.push_back(_stackVariables);
        _stackVariables.clear();
        _offsetStack.push_back(_stackOffset);
        _stackOffset = 0;
    }

    void NodeAssembler::declareParameters(SymbolList parameters)
    {
        //
        //	The ParameterVariable should have already been added to the
        //	function scope. This function will not do that. (This is
        //	currently taken care of when you define the function using the
        //	parameters).
        //

        for (int i = 0; i < parameters.size(); i++)
        {
            ParameterVariable* p =
                static_cast<ParameterVariable*>(parameters[i]);
            p->setAddress(_stackOffset++);
            _stackVariables.push_back(p);

            if (Object* o = retrieveDocumentation(p->name()))
            {
                process()->addDocumentation(p, o);
            }
        }
    }

    FreeVariable* NodeAssembler::declareFreeVariable(const Type* t, Name n)
    {
        //
        //  The called is responsible for adding the variable to proper
        //  scope!
        //

        FreeVariable* v = new FreeVariable(context(), n.c_str(), t);
        v->setAddress(_stackOffset++);
        _stackVariables.push_back(v);
        return v;
    }

    bool NodeAssembler::declareMemberVariables(const Type* type)
    {
        for (int i = 0; i < _initializerList.size(); i++)
        {
            Initializer& in = _initializerList[i];

            if (in.node)
            {
                //
                //  Can't do this in a class declaration
                //

                freportError("initializer not allowed for member \"%s\"",
                             in.name.c_str());

                clearInitializerList();
                return false;
            }
            else
            {
                MemberVariable* m =
                    new MemberVariable(context(), in.name.c_str(), type);
                scope()->addSymbol(m);

                if (Object* o = retrieveDocumentation(m->name()))
                {
                    process()->addDocumentation(m, o);
                }
            }
        }

        clearInitializerList();
        return true;
    }

    Node* NodeAssembler::declareVariables(const Type* declarationType,
                                          const char* asOp,
                                          NodeAssembler::VariableType kind)
    {
        NodeList nl = emptyNodeList();
        bool used = false;

        for (int i = 0; i < _initializerList.size(); i++)
        {
            Initializer& in = _initializerList[i];
            const Type* type = declarationType;

            if (!type)
            {
                if (in.node)
                {
                    type = in.node->type();
                }
                else
                {
                    return 0;
                }
            }

            if (type == context()->nilType())
            {
                freportError("cannot declare variable \"%s\" as type nil",
                             in.name.c_str());
                return 0;
            }

            if (!in.node)
            {
                //
                //	Look for default constructor -- if it exists use it
                //

                for (const Symbol* s = type->firstOverload(); s;
                     s = s->nextOverload())
                {
                    if (const Function* f = dynamic_cast<const Function*>(s))
                    {
                        if (!f->numArgs() && f->returnType() == type)
                        {
                            in.node =
                                callBestOverloadedFunction(f, emptyNodeList());
                            break;
                        }
                    }
                }

                if (!in.node)
                {
                    if (const Class* ctype = dynamic_cast<const Class*>(type))
                    {
                        DataNode* dn = constant(type);
                        in.node = dn;
                    }
                }
            }

            Variable* var = 0;

            if (kind == Stack)
            {
                Variable::Attributes rw = Variable::ReadWrite;
                Variable::Attributes rwi = rw | Variable::ImplicitType;
                var = declareStackVariable(type, in.name,
                                           declarationType ? rw : rwi);
            }
            else if (kind == Global)
            {
                String temp(in.name);

                var = new GlobalVariable(context(), temp.c_str(), type,
                                         process()->globals().size(),
                                         Variable::ReadWrite, 0);

                scope()->addSymbol(var);
                process()->globals().push_back(Value());
            }

            if (var)
            {
                if (Object* o = retrieveDocumentation(var->name()))
                {
                    process()->addDocumentation(var, o);
                }
            }

            if (in.node)
            {
                // if (Node *cnode = cast(in.node,type))
                // {
                //     // Node *anode = binaryOperator(asOp,
                //     // 			     referenceVariable(var),
                //     // 			     cnode);

                //     Node* anode = assignmentOperator(asOp,
                //                                      referenceVariable(var),
                //                                      cnode);

                //     if (!anode)
                //     {
                //         freportError("No assignment from \"%s\" to \"%s\"",
                //                      in.node->type()->fullyQualifiedName().c_str(),
                //                      type->fullyQualifiedName().c_str());

                //         clearInitializerList();
                //         return 0;
                //     }

                //     nl.push_back(anode);
                // }

                if (Node* r = referenceVariable(var))
                {
                    if (Node* anode = assignmentOperator(asOp, r, in.node))
                    {
                        nl.push_back(anode);
                    }
                    else
                    {
                        // memory leak?
                        freportError(
                            "Unable to cast \"%s\" to \"%s\"",
                            in.node->type()->fullyQualifiedName().c_str(),
                            type->fullyQualifiedName().c_str());

                        clearInitializerList();
                        return 0;
                    }
                }
                else
                {
                    freportError("Unable to reference \"%s\"",
                                 var->name().c_str());
                    clearInitializerList();
                    return 0;
                }
            }
            else
            {
                //
                //	The variable will be initialized to 0 (or nil)
                //

                nl.push_back(referenceVariable(var));
            }
        }

        Node* n = 0;

        if (nl.size() == 1)
        {
            n = nl.front();
        }
        else if (nl.size() > 1)
        {
            n = callBestOverloadedFunction(context()->simpleBlock(), nl);
        }
        else
        {
            n = callBestOverloadedFunction(context()->noop(), emptyNodeList());
        }

        clearInitializerList();
        removeNodeList(nl);

        return n;
    }

    StackVariable*
    NodeAssembler::declareStackVariable(const Type* t, Name n,
                                        Variable::Attributes attrs)
    {
        StackVariable* svar =
            new StackVariable(context(), n.c_str(), t, _stackOffset++, attrs);
        _stackVariables.push_back(svar);
        scope()->addSymbol(svar);
        return svar;
    }

    GlobalVariable* NodeAssembler::declareGlobalVariable(const Type* t, Name n)
    {
        GlobalVariable* var = new GlobalVariable(context(), n.c_str(), t,
                                                 process()->globals().size(),
                                                 Variable::ReadWrite, 0);

        if (Object* o = retrieveDocumentation(var->name()))
        {
            process()->addDocumentation(var, o);
        }

        scope()->addSymbol(var);
        process()->globals().push_back(Value());
        return var;
    }

    Namespace* NodeAssembler::declareNamespace(Name n)
    {
        Namespace* ns = new Namespace(context(), n.c_str());
        scope()->addSymbol(ns);
        return ns;
    }

    int NodeAssembler::endStackFrame()
    {
        //
        //  Partition the stack variables into parameters and
        //  non-parameters. If free variables have been created, these
        //  need to be part of the parameter list.
        //

        int s = _stackVariables.size();
        stable_partition(_stackVariables.begin(), _stackVariables.end(),
                         stl_ext::IsA_p<StackVariable, ParameterVariable>());

        int base = _stackOffset - _stackVariables.size();

        for (int i = 0; i < _stackVariables.size(); i++)
        {
            //
            //  Renumber the partioned variables
            //

            _stackVariables[i]->setAddress(base + i);
        }

        if (_stackVariableStack.size())
        {
            //
            //  Restore the old stack to the in-progress stack
            //

            _stackVariables = _stackVariableStack.back();
            _stackVariableStack.pop_back();
            _stackOffset = _offsetStack.back();
            _offsetStack.pop_back();
        }
        else
        {
            _stackVariables.clear();
            _stackOffset = 0;
        }

        return s;
    }

    Node* NodeAssembler::endStackFrame(NodeList nl)
    {
        Node* n = 0;
        Function* I = 0;

        if (nl.size() != 0)
        {
#if 0
	for (int i=0; i < _stackVariables.size(); i++)
	{
	    StackVariable *v = _stackVariables[i];

	    if (v->storageClass()->isSelfManaged())
	    {
		nl.push_back(release(referenceVariable(v)));
	    }
	}
#endif
            // n = callBestOverloadedFunction(context()->fixedFrameBlock(), nl);
            DataNode* dn = new DataNode(nl.size(), NodeFunc(0),
                                        context()->fixedFrameBlock());
            if (nl.size())
                dn->setArgs(&nl.front(), nl.size());
            dn->_func = context()->fixedFrameBlock()->func(dn);
            dn->_data._int = _stackVariables.size();
            n = dn;

            if (scope() != context()->globalScope())
            {
                I = new Function(context(), uniqueNameInScope("__init").c_str(),
                                 context()->voidType(), 0, 0, n,
                                 Function::None);
            }
        }

        size_t ss = endStackFrame();

        if (I && scope() != context()->globalScope())
        {
            I->stackSize(ss);
            scope()->addSymbol(I);
            return callFunction(I, emptyNodeList());
        }
        else
        {
            return n;
        }
    }

    Name NodeAssembler::uniqueNameInScope(const char* name) const
    {
        return context()->uniqueName(scope(), name);

#if 0
    for (int i=0; true; i++)
    {
	char temp[100];
	sprintf(temp,"%s%d",name,i);

	if (context()->namePool().exists(temp))
	{
	    Name symName = context()->lookupName(temp);
            if (scope()->findSymbol(symName)) continue;
            return symName;
	}
	else
	{
            return context()->internName(temp);
	}
    }
#endif
    }

    void NodeAssembler::pushAnonymousScope(const char* name)
    {
        if (Symbol* anon =
                new Namespace(context(), uniqueNameInScope(name).c_str()))
        {
            scope()->addSymbol(anon);
            pushScope(anon);
        }
        else
        {
            freportError("Unable to create anonymous scope");
        }
    }

    Node* NodeAssembler::dereferenceLValue(Node* n)
    {
        if (!n)
        {
            return 0;
        }

        const Type* t = n->type();
        if (!t)
            return n;

        if (t == context()->unresolvedType())
        {
            if (n->symbol() == context()->unresolvedMemberReference())
            {
                //
                //  This is not good, but it works for the moment. See
                //  also ASTMemberReference and ASTMemberCall for the full
                //  hack.
                //

                return n;
            }
            else
            {
                return new ASTDereference(
                    *this, context()->unresolvedDereference(), n);
            }
        }
        else if (t->isReferenceType())
        {
            const ReferenceType* r = static_cast<const ReferenceType*>(t);
            const MachineRep* rep = r->dereferenceType()->machineRep();

            if (n->func() == rep->referenceStackFunc())
            {
                //
                //	Transform the node into a dereference node
                //	(AccessStack) instead of casting it back.
                //

                n->_func = rep->dereferenceStackFunc();
                return n;
            }
            else if (n->func() == rep->referenceGlobalFunc())
            {
                //
                //	Transform the node into a dereference node
                //	instead of casting it back.
                //

                n->_func = rep->dereferenceGlobalFunc();
                return n;
            }
            else if (const MemberVariable* mv =
                         dynamic_cast<const MemberVariable*>(n->symbol()))
            {
                //
                //	If its a member variable and its a reference we can
                //	substitute in a dereference instead.
                //

                t = dynamic_cast<const Type*>(mv->scope());
                assert(t);
                rep = t->machineRep();

                if (const Class* c = dynamic_cast<const Class*>(t))
                {
                    rep = mv->storageClass()->machineRep();

                    if (n->func() == rep->referenceClassMemberFunc())
                    {
                        n->_func = rep->dereferenceClassMemberFunc();

                        if (!n->_func)
                        {
                            String temp(rep->name());
                            freportError(
                                "MachineRep \"%s\" does not "
                                "implement class member dereference function",
                                temp.c_str());
                            return 0;
                        }

                        return n;
                    }
                }
                else
                {
                    if (n->func() == rep->referenceMemberFunc())
                    {
                        n->_func = rep->dereferenceMemberFunc();

                        if (!n->_func)
                        {
                            String temp(rep->name());
                            freportError(
                                "MachineRep \"%s\" does not "
                                "implement class member dereference function",
                                temp.c_str());
                            return 0;
                        }

                        return n;
                    }
                }
            }

            //
            //  Have to cast it.
            //

            n = cast(n, r->dereferenceType());
        }
        else if (const VariantTagType* vt =
                     dynamic_cast<const VariantTagType*>(t))
        {
            n = cast(n, vt->variantType());
        }

        return n;
    }

    StackVariable* NodeAssembler::findStackVariable(StackVariable* var) const
    {
        for (int i = _stackVariables.size(); i--;)
        {
            if (_stackVariables[i] == var)
                return var;
        }

        return 0;
    }

    const StackVariable*
    NodeAssembler::findStackVariable(const StackVariable* var) const
    {
        for (int i = _stackVariables.size(); i--;)
        {
            if (_stackVariables[i] == var)
                return var;
        }

        return 0;
    }

    Node* NodeAssembler::referenceVariable(const Variable* var)
    {
        // const_cast<Variable*>(var)->lvalueReferenceCount++;

        if (const MemberVariable* v = dynamic_cast<const MemberVariable*>(var))
        {
            if (Function* F = currentFunction())
            {
                Name tname = context()->internName("this");

                if (ParameterVariable* thisp =
                        F->findSymbolOfType<ParameterVariable>(tname))
                {
                    Node* prefix = dereferenceVariable(thisp);
                    return prefix ? referenceMemberVariable(v, prefix) : 0;
                }
            }
        }
        else
        {
            Node* n = 0;
            n = new Node;
            n->_symbol = var;
            const MachineRep* rep = var->storageClass()->machineRep();

            if (const StackVariable* sv =
                    dynamic_cast<const StackVariable*>(var))
            {
                if (sv->storageClass() == context()->unresolvedType()
                    || sv->storageClass()->isTypeVariable())
                {
                    n->deleteSelf();
                    return unresolvableStackReference(sv);
                }
                else
                {
                    n->_func = rep->referenceStackFunc();
                }
            }
            else if (const GlobalVariable* gv =
                         dynamic_cast<const GlobalVariable*>(var))
            {
                n->_func = rep->referenceGlobalFunc();
            }

            return n;
        }

        return 0;
    }

    Node* NodeAssembler::dereferenceVariable(const char* name)
    {
        if (const Variable* var =
                findTypeInScope<Variable>(context()->internName(name)))
        {
            return dereferenceVariable(var);
        }
        else
        {
            freportError("Cannot dereference variable of name \"%s\"", name);
            return 0;
        }
    }

    Node* NodeAssembler::dereferenceVariable(const Variable* v)
    {
        return dereferenceLValue(referenceVariable(v));
    }

    Node* NodeAssembler::unresolvableMemberReference(Name name, Node* n)
    {
        ASTMemberReference* astnode = new ASTMemberReference(
            *this, 1, context()->unresolvedMemberReference(), name);

        astnode->setArg(n, 0);
        markCurrentFunctionUnresolved();
        return astnode;
    }

    Node* NodeAssembler::referenceMemberVariable(const MemberVariable* var,
                                                 Node* a)
    {
        const Type* t = dynamic_cast<const Type*>(var->scope());

        if (!t)
        {
            freportError("Member variable \"%s\" is a member of \"%s\" which "
                         "is not a type",
                         var->fullyQualifiedName().c_str(),
                         var->scope()->fullyQualifiedName().c_str());

            return 0;
        }

        if (const Class* c = dynamic_cast<const Class*>(t))
        {
            const MachineRep* rep = var->storageClass()->machineRep();

            if (const Function* f = var->referenceFunction())
            {
                NodeList nl = newNodeList(dereferenceLValue(a));
                Node* n = callBestOverloadedFunction(f, nl);
                removeNodeList(nl);
                return n;
            }
            else if (NodeFunc nf = rep->referenceClassMemberFunc())
            {
                Node* n = new Node(1, nf, var);
                n->setArg(dereferenceLValue(a), 0);
                return n;
            }
        }
        else
        {
            const MachineRep* rep = t->machineRep();
            const Type* nt = a->type();
            const ReferenceType* rt = dynamic_cast<const ReferenceType*>(nt);

            if (rt)
            {
                const Type* dt = rt->dereferenceType();

                if (const Function* f = var->referenceFunction())
                {
                    NodeList nl = newNodeList(a);
                    Node* n = callBestOverloadedFunction(f, nl);
                    removeNodeList(nl);
                    return n;
                }
                else if (NodeFunc nf = rep->referenceMemberFunc())
                {
                    Node* n = new Node(1, nf, var);
                    n->setArg(a, 0);
                    return n;
                }
                else
                {
                    String temp(rep->name());
                    freportError("MachineRep \"%s\" does not "
                                 "implement class member reference function",
                                 temp.c_str());
                }
            }
            else
            {
                if (const Function* f = var->extractFunction())
                {
                    NodeList nl = newNodeList(a);
                    Node* n = callBestOverloadedFunction(f, nl);
                    removeNodeList(nl);
                    return n;
                }
                else if (NodeFunc nf = rep->extractMemberFunc())
                {
                    Node* n = new Node(1, nf, var);
                    n->setArg(a, 0);
                    return n;
                }
                else
                {
                    String temp(rep->name());
                    freportError("MachineRep \"%s\" does not "
                                 "implement class member extract function",
                                 temp.c_str());
                }
            }
        }

        return 0;
    }

    Node* NodeAssembler::replaceNode(Node* n, const Symbol* s, NodeFunc f) const
    {
        n->_symbol = s;
        n->_func = f;
        return n;
    }

    struct Patch
    {
        Patch(NodeAssembler* as)
            : _as(as)
        {
        }

        void operator()(Function* f) { _as->patchFunction(f); }

        NodeAssembler* _as;
        MU_GC_NEW_DELETE
    };

    void NodeAssembler::patchUnresolved()
    {
        for_each(_unresolvedFunctions.begin(), _unresolvedFunctions.end(),
                 Patch(this));

        _unresolvedFunctions.clear();
    }

    void NodeAssembler::patchFunction(Function* f)
    {
        if (f->_unresolvedStubs == true && !f->isPolymorphic())
        {
            f->_unresolvedStubs = false;
            NodePatch(this, f).patch();

            if (f->_unresolvedStubs)
            {
                freportError(
                    "While back patching unresolved symbols in function \"%s\""
                    " some symbols not resolvable",
                    f->fullyQualifiedName().c_str());
            }
            else
            {
                f->body()->markChangeEnd();
            }
        }
    }

    void NodeAssembler::pushScope(Symbol* s, bool declarative)
    {
        _scope = new ScopeState(s, _scope, declarative);
    }

    void NodeAssembler::setScope(const ScopeState* ss) { _scope = ss; }

    void NodeAssembler::popScope()
    {
        //
        //  Destructively pop the top of the scope. NOTE: we do NOT delete
        //  these objects as they are popped because ASTNodes may be
        //  refering to them. Just let the GC clean up the mess
        //

        while (!_scope->declaration && _scope->parent)
            _scope = _scope->parent;
        if (_scope->parent)
            _scope = _scope->parent;
    }

    void NodeAssembler::popScopeToRoot()
    {
        while (_scope->parent)
            _scope = _scope->parent;
    }

    Symbol* NodeAssembler::scope() const
    {
        for (const ScopeState* ss = _scope; ss; ss = ss->parent)
        {
            if (ss->declaration)
                return ss->symbol;
        }

        return 0;
    }

    Symbol* NodeAssembler::nonAnonymousScope()
    {
        for (const ScopeState* ss = _scope; ss; ss = ss->parent)
        {
            if (ss->declaration && !dynamic_cast<Namespace*>(ss->symbol))
            {
                return ss->symbol;
            }
        }

        return 0;
    }

    const Type* NodeAssembler::popType()
    {
        if (_typeStack.size())
        {
            const Type* t = _typeStack.back();
            _typeStack.pop_back();
            return t;
        }
        else
        {
            return 0;
        }
    }

    const Type* NodeAssembler::topType()
    {
        if (_typeStack.size())
        {
            return _typeStack.back();
        }
        else
        {
            return 0;
        }
    }

    void NodeAssembler::pushType(const Type* t) { _typeStack.push_back(t); }

    Interface* NodeAssembler::declareInterface(const char* name,
                                               SymbolList inheritance)
    {
        Interface::Interfaces ilist;

        for (int q = 0; q < inheritance.size(); q++)
        {
            Interface* i = dynamic_cast<Interface*>(inheritance[q]);

            if (!i)
            {
                freportError("Interface \"%s\" may not inherit "
                             "from non-interface \"%s\"",
                             name, inheritance[q]->name().c_str());
                return 0;
            }
            else
            {
                ilist.push_back(i);
            }
        }

        Interface* i = new Interface(context(), name, ilist);
        String rtname = name;
        rtname += "&";
        scope()->addSymbol(i);

        if (Object* o = retrieveDocumentation(i->name()))
        {
            process()->addDocumentation(i, o);
        }

        ReferenceType* rt = new ReferenceType(context(), rtname.c_str(), i);
        i->globalScope()->addSymbol(rt);
        pushScope(i, true);
        return i;
    }

    void NodeAssembler::generateDefaults(Interface* i)
    {
        //
        //  Create the default constructor
        //

        i->freeze();
        const ReferenceType* rt = i->referenceType();

        //
        //  Declare = functions
        //

        Function* OpAs =
            new Function(context(), "=", BaseFunctions::assign,
                         Function::MemberOperator | Function::Operator,
                         Function::Return, rt->fullyQualifiedName().c_str(),
                         Function::Args, rt->fullyQualifiedName().c_str(),
                         i->fullyQualifiedName().c_str(), Function::End);

        i->globalScope()->addSymbol(OpAs);
    }

    VariantType* NodeAssembler::declareVariantType(const char* name)
    {
        VariantType* t = new VariantType(context(), name);
        scope()->addSymbol(t);
        String rtname = t->name().c_str();
        rtname += "&";
        ReferenceType* rt = new ReferenceType(context(), rtname.c_str(), t);
        scope()->addSymbol(rt);

        Function* dr = new Function(
            context(), t->name().c_str(), BaseFunctions::dereference,
            Function::Cast, Function::Return, t->fullyQualifiedName().c_str(),
            Function::Args, t->fullyQualifiedName().c_str(), Function::End);

        Function* OpAs =
            new Function(context(), "=", BaseFunctions::assign,
                         Function::MemberOperator | Function::Operator,
                         Function::Return, rt->fullyQualifiedName().c_str(),
                         Function::Args, rt->fullyQualifiedName().c_str(),
                         t->fullyQualifiedName().c_str(), Function::End);

        t->globalScope()->addSymbol(dr);
        t->globalScope()->addSymbol(OpAs);

        pushScope(t, true);
        return t;
    }

    VariantTagType* NodeAssembler::declareVariantTagType(const char* name,
                                                         const Type* type)
    {
        if (VariantType* v = dynamic_cast<VariantType*>(scope()))
        {
            VariantTagType* t = new VariantTagType(context(), name, type);
            scope()->addSymbol(t);
            const Type* r = t->representationType();

            if (r != context()->voidType())
            {
                //
                //  This is the "default" variant tag type constructor which
                //  just takes the reptype as an argument. We'll be calling
                //  this function from the "copied" constructors.
                //

                const Function* FT = t->findSymbolOfType<Function>(t->name());

                //
                //  Add all of the type constructors
                //

                STLVector<const Function*>::Type constructors;

                if (r->scope()->symbolTable())
                {
                    for (SymbolTable::Iterator i(r->scope()->symbolTable()); i;
                         ++i)
                    {
                        for (const Symbol* s = *i; s; s = s->nextOverload())
                        {
                            if (const Function* F =
                                    dynamic_cast<const Function*>(s))
                            {
                                if (F->name() == r->name()
                                    && F->isConstructor())
                                {
                                    constructors.push_back(F);
                                }
                            }
                        }
                    }
                }

                if (r->symbolTable())
                {
                    for (SymbolTable::Iterator i(r->symbolTable()); i; ++i)
                    {
                        for (const Symbol* s = *i; s; s = s->nextOverload())
                        {
                            if (const Function* F =
                                    dynamic_cast<const Function*>(s))
                            {
                                if (F->name() == r->name())
                                    constructors.push_back(F);
                            }
                        }
                    }
                }

                //
                //  Iterator over them and copy them so that the
                //  variant type has the same constructors.
                //

                for (int i = 0; i < constructors.size(); i++)
                {
                    const Function* F = constructors[i];

                    //
                    //  Don't bother with copy and reference constructors
                    //

                    if (F->numArgs() == 1)
                    {
                        const Type* a = F->argType(0);
                        if (a == r || a == r->referenceType())
                            continue;
                    }

                    //
                    //  Don't bother with type pattern constructors
                    //

                    bool hasPatternArg = false;

                    for (int q = 0; q < F->numArgs(); q++)
                    {
                        if (F->argType(q)->isTypePattern())
                        {
                            hasPatternArg = true;
                            break;
                        }
                    }

                    if (hasPatternArg)
                        continue;

                    //
                    //  build this constructor
                    //

                    // cout << "F = ";
                    // F->output(cout);
                    // cout << endl;

                    //
                    //  Requires allocator call?
                    //

                    bool needsAlloc = F->scope() == r && F->numArgs() > 1
                                      && F->argType(0) == r;

                    SymbolList sl = emptySymbolList();

                    for (int q = needsAlloc ? 1 : 0; q < F->numArgs(); q++)
                    {
                        char temp[80];
                        sprintf(temp, "_%d", q);
                        sl.push_back(new ParameterVariable(context(), temp,
                                                           F->argType(q)));
                    }

                    newStackFrame();

                    //
                    //  This function is "generated" so it will not be
                    //  output to a muc file
                    //

                    Function* FC = new Function(
                        context(), t->name().c_str(), v, sl.size(),
                        (ParameterVariable**)(sl.size() ? &sl[0] : NULL),
                        (Node*)0,
                        Function::Cast | Function::Pure | Function::Generated);
                    t->addSymbol(FC);
                    pushScope(FC);
                    declareParameters(sl);
                    removeSymbolList(sl);

                    //
                    //  call the data constructor dereferencing
                    //  the parameters
                    //

                    NodeList nl = emptyNodeList();

                    if (needsAlloc)
                    {
                        Name allocName = context()->lookupName("__allocate");

                        if (const Function* alloc =
                                r->findSymbolOfType<Function>(allocName))
                        {
                            Node* a = callBestOverloadedFunction(
                                alloc, emptyNodeList());
                            nl.push_back(a);
                        }
                        else
                        {
                            freportError(
                                "Internal failure generating union constructor "
                                "%s: __allocate() not found in type %s",
                                FC->fullyQualifiedName().c_str(),
                                F->fullyQualifiedName().c_str());
                            abort();
                        }
                    }

                    for (int j = 0; j < FC->numArgs(); j++)
                    {
                        nl.push_back(dereferenceVariable(FC->parameter(j)));
                    }

                    Node* n = callFunction(F, nl);
                    removeNodeList(nl);

                    nl = newNodeList(n);
                    n = callFunction(FT, nl);
                    removeNodeList(nl);

                    FC->setBody(n);
                    FC->stackSize(endStackFrame());
                    popScope();
                }
            }

            return t;
        }
        else
        {
            freportError("Variant Tag \"%s\" of type \"%s\" cannot "
                         "be declared in this scope (\"%s\")",
                         name, type->fullyQualifiedName().c_str(),
                         scope()->fullyQualifiedName().c_str());
            return 0;
        }
    }

    Class* NodeAssembler::declareClass(const char* name, SymbolList inheritance,
                                       bool globalScope)
    {
        Class::ClassVector supers;

        for (int i = 0; i < inheritance.size(); i++)
        {
            Class* c = dynamic_cast<Class*>(inheritance[i]);

            if (c)
                supers.push_back(c);
        }

        Class* c = new Class(context(), name, supers);
        String rtname = name;
        rtname += "&";

        if (Object* o = retrieveDocumentation(c->name()))
        {
            process()->addDocumentation(c, o);
        }

        if (globalScope)
        {
            context()->globalScope()->addSymbol(c);
        }
        else
        {
            scope()->addSymbol(c);
        }

        ReferenceType* rt = new ReferenceType(context(), rtname.c_str(), c);
        // c->globalScope()->addSymbol(rt);
        c->scope()->addSymbol(rt);

        Function* dr = new Function(
            context(), c->name().c_str(), BaseFunctions::dereference,
            Function::Cast, Function::Return, c->fullyQualifiedName().c_str(),
            Function::Args, rt->fullyQualifiedName().c_str(), Function::End);

        // c->globalScope()->addSymbol(dr);
        c->scope()->addSymbol(dr);

        Function* A =
            new Function(context(), "__allocate", BaseFunctions::classAllocate,
                         Function::None, Function::Return,
                         c->fullyQualifiedName().c_str(), Function::End);

        c->addSymbol(A);

        pushScope(c, true);
        return c;
    }

    void NodeAssembler::generateDefaults(Class* c)
    {
        //
        //  Create the default constructor
        //

        c->freeze();
        const Class::MemberVariableVector& mvars0 = c->memberVariables();
        Class::MemberVariableVector mvars;
        Class::MemberVariableVector svars;

        //
        //  Copy over a clean mvars without the object bondary members
        //

        for (size_t i = 0; i < mvars0.size(); i++)
        {
            if (mvars0[i]->storageClass() != context()->typeSymbolType())
            {
                mvars.push_back(mvars0[i]);
            }
        }

        const ReferenceType* rt = c->referenceType();

        //
        //  Determine if a constructor for all the public members allready
        //  exists or not
        //

        stl_ext::StaticPointerCast<Symbol, Function> castOp;
        Context::TypeVector types(mvars.size() + 1);
        Context::MatchType matchType = Context::ExactMatch;
        types[0] = c;
        for (int i = 0; i < mvars.size(); i++)
            types[i + 1] = mvars[i]->storageClass();
        Symbol::SymbolVector symbols =
            c->findSymbolsOfType<Function>(c->name());
        Context::FunctionVector functions;
        functions.resize(symbols.size());
        transform(symbols.begin(), symbols.end(), functions.begin(), castOp);

        bool hasConstructor = functions.size() != 0;
        bool hasMemberInit = false;
        bool hasDefaultInit = false;

        if (const Function* f = context()->matchSpecializedFunction(
                process(), thread(), functions, types, matchType))
        {
            hasMemberInit = true;
        }

        types.resize(1);

        if (const Function* f = context()->matchSpecializedFunction(
                process(), thread(), functions, types, matchType))
        {
            hasDefaultInit = true;
        }

        // if (!hasMemberInit)
        if (!hasConstructor)
        {
            //
            //  Make a aggregate constructor
            //  Create the parameters (named _0 ... _N)
            //

            ParameterVariable* thisv =
                new ParameterVariable(context(), "this", c);
            SymbolList params = newSymbolList(thisv);

            for (int i = 0; i < mvars.size(); i++)
            {
                char temp[40];
                sprintf(temp, "_%d", i);
                const Type* t = mvars[i]->storageClass();

                //
                //  By adding default arguments, it can cover both the
                //  default constructor and the aggregate constructor
                //

                params.push_back(
                    new ParameterVariable(context(), temp, t, Value()));
            }

            newStackFrame();

            //
            //  Consider this function as "user defined". I.e. don't make
            //  it Function::Generated -- we want it output into muc files
            //

            Function* F =
                new Function(context(), c->name().c_str(), c, params.size(),
                             (ParameterVariable**)&params.front(), 0,
                             Function::ContextDependent);
            c->addSymbol(F);
            pushScope(F);
            declareParameters(params);
            removeSymbolList(params);

            //
            //  For each parameter, assign to member
            //

            NodeList nl = emptyNodeList();

            // foreach mvar -> parameter = i: this.mvar = _i

            for (int i = 0; i < mvars.size(); i++)
            {
                const ParameterVariable* pv = F->parameter(i + 1);
                Node* lval = referenceMemberVariable(
                    mvars[i], dereferenceVariable(thisv));
                Node* rval = dereferenceVariable(pv);
                Node* asop = binaryOperator("=", lval, rval);
                nl.push_back(asop);
            }

            // return this
            nl.push_back(dereferenceVariable(thisv));

            Node* body =
                callBestOverloadedFunction(context()->simpleBlock(), nl);
            removeNodeList(nl);

            F->stackSize(endStackFrame());
            F->setBody(body);
            popScope();
        }
    }

    bool NodeAssembler::checkRedeclaration(const char* name,
                                           const Type* returnType,
                                           SymbolList parameters)
    {
        if (!name)
            return true;

        if (const Function* G = scope()->findSymbolOfType<Function>(
                context()->internName(name)))
        {
            for (const Symbol* s = G->firstOverload(); s; s = s->nextOverload())
            {
                if (const Function* G0 = dynamic_cast<const Function*>(s))
                {
                    if (G0->numArgs() == parameters.size())
                    {
                        bool differ = false;

                        for (size_t i = 0; i < parameters.size(); i++)
                        {
                            const ParameterVariable* p =
                                static_cast<const ParameterVariable*>(
                                    parameters[i]);

                            if (p->storageClass() != G0->argType(i))
                            {
                                differ = true;
                                break;
                            }
                        }

                        if (!differ)
                        {
                            ostringstream str;

                            if (G0->body() && context()->debugging())
                            {
                                AnnotatedNode* anode =
                                    static_cast<AnnotatedNode*>(G0->body());
                                str << "declared at " << anode->sourceFileName()
                                    << ", line " << anode->linenum()
                                    << ", char " << anode->charnum();
                            }
                            else if (!G0->body())
                            {
                                str << " which is a native function";
                            }

                            string temp(str.str());
                            freportError("Redeclaration of \"%s\" %s",
                                         G0->fullyQualifiedName().c_str(),
                                         temp.c_str());

                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    Function* NodeAssembler::declareFunction(const char* name,
                                             const Type* returnType,
                                             SymbolList parameters,
                                             unsigned int attrs,
                                             bool addToContext)
    {
        if (!checkRedeclaration(name, returnType, parameters))
            return 0;

        newStackFrame();
        const char* fname =
            name ? name : context()->uniqueName(scope(), "__lambda").c_str();

        Function* f;

        /* AJG - bugfix */
        if (parameters.empty())
        {
            f = new Function(context(), fname, returnType, 0, 0, 0, attrs);
        }
        else
        {
            f = new Function(context(), fname, returnType, parameters.size(),
                             (ParameterVariable**)&(parameters.front()), 0,
                             attrs);
        }

        if (Object* o = retrieveDocumentation(f->name()))
        {
            process()->addDocumentation(f, o);
        }

        if (addToContext)
        {
            if (name)
            {
                scope()->addSymbol(f);
            }
            else
            {
                scope()->addAnonymousSymbol(f);
            }
        }

        pushScope(f);
        declareParameters(parameters);
        return f;
    }

    MemberFunction* NodeAssembler::declareMemberFunction(const char* name,
                                                         const Type* returnType,
                                                         SymbolList parameters,
                                                         unsigned int attrs)
    {
        Object* odoc = retrieveDocumentation(context()->internName(name));

        ParameterVariable* self =
            new ParameterVariable(context(), "this", classScope());
        insertSymbolAtFront(parameters, self);

        if (!checkRedeclaration(name, returnType, parameters))
            return 0;

        // is this not correct? It was missing before.
        newStackFrame();
        MemberFunction* f = 0;

        if (parameters.empty())
        {
            f = new MemberFunction(context(), name, returnType, 0, 0, 0, attrs);
        }
        else
        {
            f = new MemberFunction(
                context(), name, returnType, parameters.size(),
                (ParameterVariable**)&(parameters.front()), 0, attrs);
        }

        if (odoc)
            process()->addDocumentation(f, odoc);

        scope()->addSymbol(f);
        pushScope(f);
        declareParameters(parameters);

        return f;
    }

    Function* NodeAssembler::declareFunctionBody(Function* F, Node* n)
    {
        if (MemberFunction* M = dynamic_cast<MemberFunction*>(F))
        {
            if (M->isConstructor())
            {
                NodeList nl = newNodeList(n);
                nl.push_back(dereferenceVariable(M->parameter(0)));
                n = callBestOverloadedFunction(context()->simpleBlock(), nl);
                removeNodeList(nl);
            }
        }

        int stackSize = endStackFrame();
        popScope();
        F->stackSize(stackSize);

        if (F->hasReturn())
        {
            F->setBody(n);
        }
        else if (!F->returnType())
        {
            F->setBody(n);
        }
        else if (n)
        {
            if (Node* code = cast(n, F->returnType()))
            {
                F->setBody(code);
                code->markChangeEnd();
            }
            else if (n->type()->isUnresolvedType())
            {
                //
                //  Unresolved case will be handled by NodePatch
                //

                F->setBody(n);
            }
            else
            {
                freportError("Function body returns %s; cannot cast to %s.",
                             n->type()->fullyQualifiedName().c_str(),
                             F->returnTypeName().c_str());

                return 0;
            }
        }
        else
        {
            //
            //  This is an "abstract" function (no body) so generate a
            //  throw statement to that effect.
            //
        }

        return F;
    }

    void NodeAssembler::declarationType(const Type* t, bool global)
    {
        _initializerGlobal = global;
        _initializerType = t;
    }

    Node* NodeAssembler::declareInitializer(Name n, Node* node)
    {
        _initializerList.push_back(Initializer(n, node));

        if (_initializerGlobal)
        {
            Node* rn = declareGlobalVariables(_initializerType, "=");

            if (!rn)
            {
                if (_initializerType)
                {
                    freportError("Illegal assignment to %s.", n.c_str());
                }
                else
                {
                    freportError("Cannot use default constructor with implicit "
                                 "type declaration (what type is it?).");
                }
            }

            return rn;
        }
        else
        {
            if (!_initializerType)
            {
                if (classScope() || interfaceScope())
                {
                    freportError("let may not be used in this context.");
                    return 0;
                }
                else
                {
                    Node* rn = declareStackVariables(_initializerType, "=");

                    if (!rn)
                    {
                        freportError(
                            "Cannot use default constructor with implicit "
                            "type declaration (what type is it?).");
                    }

                    return rn;
                }
            }
            else if (classScope())
            {
                declareMemberVariables(_initializerType);
                return 0;
            }
            else if (interfaceScope())
            {
                freportError("An interface may not have member variables");
                return 0;
            }
            else
            {
                Node* rn = declareStackVariables(_initializerType, "=");
                if (!rn)
                    freportError("Illegal assignment to %s.", n.c_str());
                return rn;
            }
        }
    }

    Class* NodeAssembler::declareTupleType(SymbolList types)
    {
        Context::TypeVector typeVector(types.size());

        for (int i = 0; i < types.size(); i++)
        {
            typeVector[i] = static_cast<const Type*>(types[i]);
        }

        return context()->tupleType(typeVector);
    }

    Node* NodeAssembler::tupleNode(NodeList nl)
    {
        SymbolList sl = emptySymbolList();
        bool ok = true;

        for (int i = 0; i < nl.size(); i++)
        {
            const Type* t = nl[i]->type();

            if (nl[i]->type()->isUnresolvedType())
            {
                // Node* n = unresolvableCall(context()->internName("(type*)"),
                // nl);
                Node* n = new ASTTupleConstructor(*this, nl.size(), &nl.front(),
                                                  context()->unresolvedCall());
                markCurrentFunctionUnresolved();
                removeSymbolList(sl);
                return n;
            }

            if (t == context()->nilType())
            {
                freportError("nil is not allowed in this context");
                removeSymbolList(sl);
                return 0;
            }

            sl.push_back((Symbol*)nl[i]->type());
        }

        Class* type = declareTupleType(sl);
        removeSymbolList(sl);

        //
        //  Call the default constructor
        //

        return call(type, nl);
    }

    Node* NodeAssembler::listNode(NodeList nl)
    {
        for (size_t i = 0; i < nl.size(); i++)
        {
            if (nl[i]->type()->isUnresolvedType())
            {
                markCurrentFunctionUnresolved();

                return new ASTListConstructor(*this, nl.size(), &nl.front(),
                                              context()->unresolvedCall());
            }
        }

        const ListType* t = context()->listType(nl[0]->type());

        for (int i = 1, s = nl.size(); i < s; i++)
        {
            const Type* ntype = nl[i]->type();
            const Type* etype = t->elementType();

            if (!etype->match(ntype))
            {
                freportError(
                    "cannot construct \"%s\" list "
                    "because of inconsistant element types: "
                    "at element %d: \"%s\" does not match expected \"%s\"",
                    t->fullyQualifiedName().c_str(), i + 1,
                    ntype->fullyQualifiedName().c_str(),
                    etype->fullyQualifiedName().c_str());

                return 0;
            }
        }

        return call(t, nl);
    }

    TypeVariable* NodeAssembler::declareTypeVariable(const char* bname)
    {
        String n = "'";
        n += bname;
        Name name = context()->internName(n);

        if (const TypeVariable* t = findTypeInScope<TypeVariable>(name))
        {
            return const_cast<TypeVariable*>(t);
        }
        else
        {
            TypeVariable* tt = new TypeVariable(context(), n.c_str());
            scope()->addSymbol(tt);
            return tt;
        }
    }

    Node* NodeAssembler::suffix(Node* n, Name suffixName)
    {
        if (!_suffixModule)
        {
            Name moduleName = context()->internName("suffix");
            _suffixModule =
                _context->globalScope()->findSymbolOfType<Module>(moduleName);
        }

        if (_suffixModule)
        {
            if (Function* f =
                    _suffixModule->findSymbolOfType<Function>(suffixName))
            {
                NodeList nl = newNodeList(n);

                if (Node* r = callBestOverloadedFunction(f, nl))
                {
                    removeNodeList(nl);
                    return r;
                }

                removeNodeList(nl);
            }
        }

        freportError("Unknown suffix \"%s\".", suffixName.c_str());
        return 0;
    }

    void NodeAssembler::addDocumentation(Name n, Object* o, const Type* t)
    {
        if (_docmodule)
        {
            if (Symbol* symbol =
                    _docmodule->findSymbolByQualifiedName(n, false))
            {
                bool add = false;

                for (Symbol* sym = symbol->firstOverload(); sym;
                     sym = sym->nextOverload())
                {
                    add = false;

                    if (t)
                    {
                        if (sym == t)
                        {
                            add = true;
                        }
                        else if (Function* F = dynamic_cast<Function*>(sym))
                        {
                            add = F->type() == t;
                        }
                        else if (Variable* v = dynamic_cast<Variable*>(sym))
                        {
                            add = v->storageClass() == t;
                        }
                    }
                    else
                    {
                        add = true;
                    }

                    if (add)
                        process()->addDocumentation(sym, o);
                }
            }
        }
        else
        {
            if (_scope)
                _scope->docMap[n] = o;
        }
    }

    void NodeAssembler::setDocumentationModule(Symbol* s) { _docmodule = s; }

    Object* NodeAssembler::retrieveDocumentation(Name n)
    {
        for (const ScopeState* ss = _scope; ss; ss = ss->parent)
        {
            DocMap::iterator i = ss->docMap.find(n);

            if (i != ss->docMap.end())
            {
                Object* o = i->second;
                ss->docMap.erase(i);
                return o;
            }
        }

        if (n != "")
        {
            return retrieveDocumentation(context()->internName(""));
        }
        else
        {
            return 0;
        }
    }

    NodeAssembler::Pattern* NodeAssembler::newPattern(Name n)
    {
        return new Pattern(n);
    }

    NodeAssembler::Pattern* NodeAssembler::newPattern(Node* n)
    {
        // return newPattern(context()->namePool().intern("_").nameRef());
        return new Pattern(n);
    }

    NodeAssembler::Pattern* NodeAssembler::newPattern(Pattern* p,
                                                      const char* name)
    {
        Name n = context()->lookupName(name);

        if (const TypePattern* tp = context()->findSymbolOfType<TypePattern>(n))
        {
            return new Pattern(p, tp);
        }
        else
        {
            freportError("Bad type pattern in newPattern() (%s)", name);
            return 0;
        }
    }

    void NodeAssembler::appendPattern(Pattern* pattern, Pattern* appendee)
    {
        Pattern* p = pattern;
        for (; p->next; p = p->next)
            ;
        p->next = appendee;
    }

    Node* NodeAssembler::resolvePatterns(Pattern* pattern, Node* rhs)
    {
        declarationType(0, _initializerGlobal);

        if (pattern->constructor)
        {
            rhs = cast(rhs, pattern->constructor);
            if (!rhs)
                return 0;
            const Function* F =
                pattern->constructor->findSymbolOfType<Function>(
                    context()->lookupName("__unpack"));
            NodeList nl = newNodeList(rhs);
            rhs = callFunction(F, nl);
            removeNodeList(nl);

            if (const VariantTagType* tt =
                    dynamic_cast<const VariantTagType*>(pattern->constructor))
            {
                if (tt->representationType() == context()->voidType())
                    return rhs;
            }
        }

        if (pattern->children)
        {
            if (!pattern->typePattern->match(rhs->type()))
            {
                freportError(
                    "Pattern will not match against type \"%s\", "
                    "requires match of type pattern \"%s\"",
                    rhs->type()->fullyQualifiedName().c_str(),
                    pattern->typePattern->fullyQualifiedName().c_str());
                return 0;
            }

            Name name = uniqueNameInScope("__var");
            Node* n = declareInitializer(name, rhs);
            NodeList nl = resolvePatternList(pattern->children, n,
                                             findTypeInScope<Variable>(name));
            if (nl.empty())
                return 0;
            insertNodeAtFront(nl, n);

            Node* block =
                callBestOverloadedFunction(context()->simpleBlock(), nl);
            removeNodeList(nl);
            return block;
        }
        else if (pattern->expression)
        {
            Node* n = 0;

            n = binaryOperator(
                rhs->type() == context()->nilType() ? "eq" : "==", rhs,
                pattern->expression);

            NodeList nl = newNodeList(n);
            n = callBestFunction("__bool_pattern_test", nl);
            removeNodeList(nl);
            return n;
        }
        else
        {
            if (pattern->name == "_")
            {
                return declareInitializer(uniqueNameInScope("__any"), rhs);
            }
            else
            {
                return declareInitializer(pattern->name, rhs);
            }
        }
    }

    NodeAssembler::NodeList NodeAssembler::resolvePatternList(Pattern* pattern,
                                                              Node* rhs,
                                                              Variable* var)
    {
        NodeList nl = emptyNodeList();
        const Type* t = var->storageClass();

        if (const Class* c = dynamic_cast<const Class*>(t))
        {
            size_t i = 0;
            int count = 0;
            const Class::MemberVariableVector& mvars = c->memberVariables();
            for (Pattern* q = pattern; q; q = q->next)
                count++;

            if (count != mvars.size())
            {
                freportError("Number of patterns (%d) does not match "
                             "number of fields (%d) in type \"%s\"",
                             count, mvars.size(),
                             t->fullyQualifiedName().c_str());

                removeNodeList(nl);
                return emptyNodeList();
            }

            for (Pattern* p = pattern; p; p = p->next)
            {
                Node* d = dereferenceVariable(var);
                Node* r = referenceMemberVariable(mvars[i++], d);

                if (Node* n = resolvePatterns(p, dereferenceLValue(r)))
                {
                    nl.push_back(n);
                }
                else
                {
                    removeNodeList(nl);
                    return emptyNodeList();
                }
            }
        }
        else if (const VariantType* v = dynamic_cast<const VariantType*>(t))
        {
            // variant constructors only take one argument
            // (this won't happen)
        }
        else
        {
            freportError("pattern cannot match type \"%s\"",
                         t->fullyQualifiedName().c_str());
        }

        return nl;
    }

    void NodeAssembler::unflattenPattern(Pattern* pattern, const char* tname,
                                         bool keepLeaf)
    {
        if (pattern->next)
        {
            if (keepLeaf && !pattern->next->next)
                return;
            pattern->next = newPattern(pattern->next, tname);
            unflattenPattern(pattern->next->children, tname, keepLeaf);
        }
        else
        {
            Name matchAny = context()->internName("_");
            appendPattern(pattern, newPattern(matchAny));
        }
    }

    //----------------------------------------------------------------------
    //
    //  Case statements
    //

    Node* NodeAssembler::beginCase(Node* n, const Function* predicate)
    {
        pushAnonymousScope("__");
        pushScope((Symbol*)(n->type()), false);
        declarationType(0);
        CaseState state;
        state.exprType = n->type();
        state.predicate = predicate;
        _caseStack.push_back(state);
        return declareInitializer(context()->internName("__case_result"), n);
    }

    struct VTypeComp
    {
        bool operator()(const Type* a, const Type* b) const
        {
            const VariantTagType* va = static_cast<const VariantTagType*>(a);
            const VariantTagType* vb = static_cast<const VariantTagType*>(b);
            return va->index() < vb->index();
        }
    };

    Node* NodeAssembler::finishCase(Node* n, NodeList nl)
    {
        insertNodeAtFront(nl, n);
        Node* r = callBestFunction("__case_test", nl);
        removeNodeList(nl);
        popScope();

        _caseStack.pop_back();
        return r;
    }

    Node* NodeAssembler::casePatternStatement(Node* n, NodeList nl)
    {
        if (n)
            insertNodeAtFront(nl, n);
        Node* r = callBestFunction("__pattern_test", nl);
        removeNodeList(nl);
        popScope();
        return r;
    }

    Node* NodeAssembler::casePattern(Pattern* p)
    {
        if (p->constructor)
        {
            if (p->constructor->scope() != _caseStack.back().exprType)
            {
                freportError(
                    "case pattern constructor \"%s\" does not match case expr "
                    "type \"%s\"",
                    p->constructor->fullyQualifiedName().c_str(),
                    _caseStack.back().exprType->fullyQualifiedName().c_str());
            }
            else if (Node* n = dereferenceVariable("__case_result"))
            {
                if (n = cast(n, p->constructor))
                {
                    if (Node* r = resolvePatterns(p, n))
                    {
                        _caseStack.back().types.push_back(p->constructor);
                        return r;
                    }
                }
            }
        }
        else
        {
            if (Node* n = dereferenceVariable("__case_result"))
            {
                if (Node* r = resolvePatterns(p, n))
                    return r;
            }
        }

        _caseStack.pop_back();
        return 0;
    }

    bool NodeAssembler::casePattern(const Type* constructor)
    {
        if (constructor->scope() != _caseStack.back().exprType)
        {
            freportError(
                "case pattern constructor \"%s\" does not match case expr type "
                "\"%s\"",
                constructor->fullyQualifiedName().c_str(),
                _caseStack.back().exprType->fullyQualifiedName().c_str());
            _caseStack.pop_back();
            return false;
        }
        else
        {
            //_caseStack.back().types.push_back(constructor);
            return true;
        }
    }

    void NodeAssembler::setSourceName(const std::string& name)
    {
        _sourceName = context()->internName(name.c_str());
        context()->setSourceFileName(_sourceName);
    }

    void NodeAssembler::markCurrentFunctionUnresolved()
    {
        if (currentFunction())
        {
            if (!currentFunction()->_unresolvedStubs)
            {
                currentFunction()->_unresolvedStubs = true;
                _unresolvedFunctions.push_back(currentFunction());
            }
        }
    }

    Node* NodeAssembler::foreachStatement(Node* dnode, Node* cnode, Node* bnode)
    {
        const Mu::Type* stype = cnode->type();

        if (!bnode)
        {
            bnode =
                callBestOverloadedFunction(context()->noop(), emptyNodeList());
        }

        if (_allowUnresolved
            && (stype->isUnresolvedType() || bnode->type()->isUnresolvedType()))
        {
            Node* args[3];
            args[0] = cnode;
            args[1] = dnode;
            args[2] = bnode;

            Node* astforeach =
                new ASTForEach(*this, args, context()->unresolvedCall());

            markCurrentFunctionUnresolved();
            return astforeach;
        }

        NodeList nl = newNodeList(dnode);
        nl.push_back(cnode);
        nl.push_back(bnode);

        Node* n = callBestFunction("__for_each", nl);
        removeNodeList(nl);

        return n;
    }

    void NodeAssembler::ScopeState::show()
    {
        cout << "scope:" << endl;
        for (const ScopeState* ss = this; ss; ss = ss->parent)
        {
            cout << "    " << ss->symbol->fullyQualifiedName().c_str()
                 << (ss->declaration ? "  declaration" : "") << endl;
        }
    }

    void NodeAssembler::setLine(int i)
    {
        _line = i;
        context()->setSourceLine(_line);
    }

    void NodeAssembler::addLine(int i)
    {
        _line += i;
        context()->setSourceLine(_line);
    }

    void NodeAssembler::setChar(int i)
    {
        _char = i;
        context()->setSourceChar(_char);
    }

    void NodeAssembler::addChar(int i)
    {
        _char += i;
        context()->setSourceChar(_char);
    }

} // namespace Mu
