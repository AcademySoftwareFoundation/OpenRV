#ifndef __Mu__NodeAssembler__h__
#define __Mu__NodeAssembler__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/Interface.h>
#include <Mu/Function.h>
#include <Mu/Module.h>
#include <Mu/Name.h>
#include <Mu/NodeFunc.h>
#include <algorithm>
#include <iostream>
#include <set>
#include <stl_ext/slice.h>
#include <string>
#include <map>

namespace Mu
{

    class Class;
    class Context;
    class DataNode;
    class FreeVariable;
    class Function;
    class GlobalVariable;
    class MemberFunction;
    class MemberVariable;
    class Module;
    class Namespace;
    class Node;
    class Process;
    class ParameterVariable;
    class StackVariable;
    class SymbolicConstant;
    class Thread;
    class Type;
    class TypePattern;
    class UnresolvedSymbol;
    class Variable;
    class VariantTagType;
    class VariantType;

    //
    //  class NodeAssembler
    //
    //  A NodeAssembler is most useful when parsing. The NodeAssembler
    //  provides convenient functions for assembling node trees. It relies
    //  heavily on the implementation of the Language object to get the
    //  desired language specific behavior for function prototype matching
    //  and casting.
    //
    //  Typically, you create one of these and destory it when done with
    //  it. Its state is not useful once the Process object has been completely
    //  built.
    //

    class NodeAssembler
    {
    public:
        MU_GC_NEW_DELETE

        //
        //	Typedefs / Constants
        //

        typedef STLMap<Name, Object*>::Type DocMap;

        struct Initializer
        {
            MU_GC_NEW_DELETE
            Initializer()
                : name()
                , node(0)
            {
            }

            Initializer(Name n, Node* nn)
                : name(n)
                , node(nn)
            {
            }

            Name name;
            Node* node;
        };

        struct ScopeState
        {
            MU_GC_NEW_DELETE
            ScopeState(Symbol* s, const ScopeState* p, bool d = true)
                : symbol(s)
                , parent(p)
                , declaration(d)
            {
            }

            Symbol* symbol;
            bool declaration;
            mutable DocMap docMap;
            const ScopeState* parent;

            void show();
        };

        struct Pattern
        {
            MU_GC_NEW_DELETE
            Pattern(Name n)
                : children(0)
                , name(n)
                , next(0)
                , typePattern(0)
                , constructor(0)
                , expression(0)
            {
            }

            Pattern(Node* n)
                : children(0)
                , name()
                , next(0)
                , typePattern(0)
                , constructor(0)
                , expression(n)
            {
            }

            explicit Pattern(Pattern* list, const TypePattern* tp)
                : children(list)
                , name()
                , next(0)
                , typePattern(tp)
                , constructor(0)
                , expression(0)
            {
            }

            Name name;
            Pattern* children;
            const Type* constructor;
            const TypePattern* typePattern;
            Node* expression;
            Pattern* next;
        };

        typedef STLVector<Node*>::Type NodeVector;
        typedef STLVector<Symbol*>::Type SymbolVector;
        typedef STLVector<Name>::Type NameVector;
        typedef STLVector<FreeVariable*>::Type FreeVariableVector;
        typedef STLVector<StackVariable*>::Type StackVarVector;
        typedef STLVector<StackVarVector>::Type StackVarStack;
        typedef stl_ext::slice_struct<NodeVector> NodeList;
        typedef stl_ext::slice_struct<SymbolVector> SymbolList;
        typedef stl_ext::slice_struct<NameVector> NameList;
        typedef STLVector<Initializer>::Type InitializerList;
        typedef STLSet<Class*>::Type ClassSet;
        typedef STLVector<int>::Type OffsetStack;
        typedef STLVector<size_t>::Type IntList;
        typedef STLVector<const TypePattern*>::Type TypePatternVector;
        typedef STLVector<const Type*>::Type TypeStack;
        typedef STLVector<const Function*>::Type FunctionVector;
        typedef STLVector<Function*>::Type UnresolvedFunctions;
        typedef ParameterVariable* Param;
        typedef STLVector<Param>::Type ParameterVector;

        struct CaseState
        {
            const Type* exprType;
            TypeStack types;
            const Function* predicate;
            MU_GC_NEW_DELETE
        };

        typedef STLList<CaseState>::Type CaseStateStack;

        void dumpNodeStack();

        //
        //	NodeAssembler requires a Context and Language to build a
        //	Process object. You pass in the three objects.
        //
        //  If you don't pass in a process, then there are a limited
        //  number of things you can do with the NodeAssembler. Namely,
        //  you can declare a class or tuple.
        //

        NodeAssembler(Context*, Process* p = 0, Thread* t = 0);
        ~NodeAssembler();

        //
        //	Returns the context/language passed into the constructor
        //

        Context* context() const { return _context; }

        Process* process() const;
        Thread* thread() const;

        //
        //  Parsing state
        //

        void setLine(int i);
        void addLine(int i);
        void setChar(int i);
        void addChar(int i);
        void setSourceName(const std::string& name);

        int lineNum() const { return _line; }

        int charNum() const { return _char; }

        Name sourceName() const { return _sourceName; }

        //
        //  Error reporting. The versions that take a Node* argument
        //  attempt to find annotation for that node.
        //

        void reportError(const char*) const;
        void reportWarning(const char*) const;
        void reportError(Node*, const char*) const;
        void reportWarning(Node*, const char*) const;

        void freportError(const char*, ...) const;
        void freportWarning(const char*, ...) const;
        void freportError(Node*, const char*, ...) const;
        void freportWarning(Node*, const char*, ...) const;

        void showOptions(const FunctionVector&, NodeList);
        void showArgs(NodeList);

        bool errorOccured() const { return _error; };

        //
        //	Mode Settings. The most obvious here is the constant
        //	expression reduction.
        //

        void reduceConstantExpressions(bool b) { _constReduce = b; }

        void allowUnresolvedCalls(bool b) { _allowUnresolved = b; }

        void simplifyExpressions(bool b) { _simplify = b; }

        void throwOnError(bool b) { _throwOnError = b; }

        //
        //	Lexical Scope. You can push and pop elements off the scope
        //	stack. The stack is used for both look up and declaration. You
        //	can push an anonyous scope which uses the input name as a base
        //	name for a mangled unque name. The anonymous symbol is
        //	retained in the scope in which it is defined.
        //

        Symbol* scope() const;
        Symbol* nonAnonymousScope();

        const ScopeState* scopeState() const { return _scope; }

        Class* classScope() { return dynamic_cast<Class*>(scope()); }

        Interface* interfaceScope()
        {
            return dynamic_cast<Interface*>(scope());
        }

        void pushScope(Symbol* s, bool declarative = true);
        void setScope(const ScopeState*);

        void pushModuleScope(Name);    // creates a module
        void pushModuleScope(Module*); // uses a module

        void popScope();
        void popScopeToRoot();

        void pushAnonymousScope(const char* name);

        template <class T> T* findTypeInScope(Name) const;

        template <class T>
        bool findTypesInScope(Name, typename STLVector<const T*>::Type&) const;

        template <class T> T* findScopeOfType() const;

        Function* currentFunction() const
        {
            return findScopeOfType<Function>();
        }

        Name uniqueNameInScope(const char*) const;

        //
        //	Prefix scope overrides the scope stack. This is for languages
        // that 	require explicit scope operators: foo.bar.baz. Set this
        // to 0, to 	return to use of the scope stack.
        //

        void prefixScope(const Symbol* s);

        Symbol* prefixScope() const { return _prefixScope; }

        //
        //  Function declaration
        //  These functions push the result as the scope.
        //

        Function* declareFunction(const char* name, const Type* returnType,
                                  SymbolList parameters, unsigned int attrs,
                                  bool addToContext = true);

        MemberFunction* declareMemberFunction(const char* name,
                                              const Type* returnType,
                                              SymbolList parameters,
                                              unsigned int attrs);

        Function* declareFunctionBody(Function*, Node*);

        void markCurrentFunctionUnresolved();

        bool checkRedeclaration(const char* name, const Type* returnType,
                                SymbolList paramters);

        //
        //	type stack
        //

        void pushType(const Type* t);
        const Type* popType();
        const Type* topType();

        //
        //	Build a constant (leaf) node -- you have to stuff the data in
        //	the node after getting the pointer. if the object* is non-null
        //	then the data is put there for you and the object is entered
        //	as a constant in the process.
        //

        DataNode* constant(const Type*, Object* o = 0) const;
        DataNode* constant(const SymbolicConstant*) const;

        Node* newNode(const Function*, int nargs = 0) const;

        //
        //	Reduce a constant expression (if the input is one).
        //

        Node* constReduce(Node*) const;
        Node* constReduce(const Function*, Node*) const;
        bool isConstant(Node*) const;

        Node* functionReduce(const Function*, Node*);

        //
        //  Symbolic constant creation
        //

        SymbolicConstant* newSymbolicConstant(Name, Node*);

        //
        //	Returns a node which casts the passed in node from the toType
        //	to the fromType.
        //

        Node* cast(Node*, const Type* toType);

        //
        //	Add a retaining or releasing node for types that do their own
        //	memory management.
        //

        Node* retain(Node*) const;
        Node* release(Node*) const;

        //
        //	Build a tree from one or two branches. Uses the default rules
        //	of operator function matching from Langaugae::matchFunction()
        //

        Node* unaryOperator(const char* op, Node* node1);
        Node* binaryOperator(const char* op, Node* n1, Node* n2);

        //
        //  Like binaryOperator("=", ...) but does some extra type
        //  checking when using catch all type patterns
        //

        Node* assignmentOperator(const char* op, Node* n1, Node* n2);

        //
        //  Member operator has some freaky semantics
        //

        Node* memberOperator(const char* op, Node*, NodeList);

        //
        //  Call a specific function. This function does not test for
        //  overloaded alternatives, nor does it automatically locate free
        //  variable bindings. If the function has free variables which need to
        //  be bound, then these must be passed in as arguments. This will not
        //  generate default arguments.
        //
        //  This function will not produce an unresolved call -- it will fail
        //  instead.
        //

        Node* callFunction(const Function*, NodeList);

        //
        //  Find and call a specific function amoung overloaded functions.
        //  If the function has free variables which need to be bound,
        //  then these must be passed in as arguments. This will not
        //  generate default arguments or do any automatic casting of any
        //  sort.
        //
        //  This function will not produce an unresolved call -- it will fail
        //  instead.
        //

        Node* callExactOverloadedFunction(const Function*, NodeList);

        //
        //  Generates a NodeList of all overloaded functions of the passed
        //  in function then calls callBestFunction().
        //

        Node* callBestOverloadedFunction(const Function*, NodeList);

        //
        //  From the set of functions passed in, choose the best match.
        //  May require some scope information. Argument expressions in
        //  the NodeList may be modified via cast or memory management
        //  nodes to make the call correct.
        //
        //  If the current scope is inside a method, it may automatically
        //  insert the "this" node as the first argument.
        //
        //  Can generate an unresolved node if allowUnresolvedCalls() is
        //  true.
        //
        //  If the matched function requires free variable bindings, then the
        //  current scope is used to find the bindings. To avoid this behavior
        //  use the callFunction
        //

        Node* callBestFunction(const FunctionVector&, NodeList);

        //
        //  Looks up name in the current scope and tries to call
        //  it. Requires the current scope be accurate.
        //

        Node* callBestFunction(const char* name, NodeList);

        //
        //	Create a node for a MemberFunction. This is a variation on
        //	callFunction. No partial application occurs. The self argument
        //	becomes the object in which dynamic look up of the method
        // function 	occurs. The second version requires self be the first
        // argument.
        //

        Node* callMethod(const Function*, Node* self, NodeList arguments);
        Node* callMethod(const Function*, NodeList arguments);

        //
        //  Create a thunk in which the self paramater of method is partially
        //  applied.
        //

        Node* methodThunk(const MemberFunction*, Node* self);

        //
        //  Abstract call -- Symbol could be function, variable. If
        //  dynamicDispatch is false, and the Symbol is a method (or anything
        //  else that normally involves dynamicDispatch) the call is made
        //  literal -- i.e. the call will not go through dynamic dispatch.
        //

        Node* call(const Symbol*, NodeList, bool dynamicDispatch = true);

        //
        //  Call expression. See above for dynamicDispatch explanation
        //

        Node* call(Node*, NodeList, bool dynamicDispatch = true);

        //
        //  Suffix conversion. (Finds the correct suffix function)
        //

        Node* suffix(Node*, Name);

        //
        //  Iteration statements -- these may or may not expand to more
        //  complex expressions.
        //

        Node* foreachStatement(Node*, Node*, Node*);

        //
        //  Unresolvable call, cast, or reference
        //

        Node* unresolvableCall(Name, NodeList, const Symbol* u = 0);
        Node* unresolvableCast(Name, NodeList);
        Node* unresolvableMemberCall(Name, NodeList);

        //
        //  Unresolved Constructor
        //

        Node* unresolvableConstructor(const Type*, NodeList);

        //
        //  Do a dynamic partial evaluation of the function object -or- dynamic
        //  partial application. Its required that the function object type
        //  have operator() defined. You pass that in here.  If reduce is true,
        //  the function will return a node that does actual partial evaluation
        //  otherwise it generates a function which applies the arguments
        //  (instead of completing a partial evaluation).
        //

        Node* dynamicPartialEvalOrApply(Node* self, NodeList arguments,
                                        bool reduce = false,
                                        bool dynamicDispatch = true);

        //
        //	Replace node
        //

        Node* replaceNode(Node*, const Symbol*, NodeFunc) const;

        //
        //	Setting Process state. Typeically you call this upon
        //	completion of parsing.
        //

        void setProcessRoot(Node*);

        //
        //	Recursive node stack. There is a single stack indexed
        //	backwards. You can recursively ask for new stack (frame)
        //	pointers and cons together lists of nodes. However, you can
        //	only do so depth first --- in other words, you can start as
        //	many as you want, but once you start finishing them they have
        //	to unwind.
        //

        NodeList emptyNodeList();
        NodeList newNodeList(Node*);
        NodeList newNodeList(const NodeVector&);
        NodeList newNodeListFromArgs(Node*);
        void insertNodeAtFront(NodeList, Node*);
        void removeNodeList(NodeList);

        void clearNodeListStack() { _nodeStack.clear(); }

        bool containsNoOps(NodeList) const;

        //
        //	Symbol Lists
        //

        SymbolList emptySymbolList();
        SymbolList newSymbolList(Symbol*);
        void removeSymbolList(SymbolList);

        void clearSymbolListStack() { _symbolStack.clear(); }

        void insertSymbolAtFront(SymbolList, Symbol*);

        //
        //  Name Lists
        //

        NameList emptyNameList();
        NameList newNameList(Name);
        void removeNameList(NameList);

        void clearNameListStack() { _nameStack.clear(); }

        //
        //	int list
        //

        void addInt(size_t i) { _intList.push_back(i); }

        void clearInts() { _intList.clear(); }

        const IntList& intList() const { return _intList; }

        //
        //	InitializerList
        //

        void declarationType(const Type*, bool global = false);
        Node* declareInitializer(Name n, Node* node);

        void clearInitializerList() { _initializerList.clear(); }

        const InitializerList& initializers() const { return _initializerList; }

        enum VariableType
        {
            Stack,
            Global
        };

        Node* declareVariables(const Type*, const char* asop, VariableType);

        bool declareMemberVariables(const Type*);

        Node* declareStackVariables(const Type* t, const char* asop)
        {
            return declareVariables(t, asop, Stack);
        }

        Node* declareGlobalVariables(const Type* t, const char* asop)
        {
            return declareVariables(t, asop, Global);
        }

        void declareParameters(SymbolList);

        Node* unresolvableStackDeclaration(Name);

        //
        //  Patterns
        //

        Pattern* newPattern(Name n);
        Pattern* newPattern(Node*); // constant
        Pattern* newPattern(Pattern*, const char*);
        void appendPattern(Pattern* list, Pattern* p);
        Node* resolvePatterns(Pattern*, Node*);
        NodeList resolvePatternList(Pattern*, Node*, Variable*);

        //
        //  Takes a flat pattern list and makes a tree out of it
        //  Mainly for use when matching lists
        //

        void unflattenPattern(Pattern*, const char*, bool keepLeaf = false);

        //
        //  The caller must add the free variable to the proper scope.
        //

        FreeVariable* declareFreeVariable(const Type* t, Name n);

        //
        //  Declarations without initialization. Used primarily by
        //  archiving.
        //

        Namespace* declareNamespace(Name n);
        StackVariable* declareStackVariable(const Type* t, Name n,
                                            unsigned int attrs);
        GlobalVariable* declareGlobalVariable(const Type* t, Name n);

        //
        //	Stack frame funtions. you don't need to call newStackFrame()
        //	right away, there is a new one when the NodeAssembler is
        //	created.
        //
        //	endStackFrame(NodeList) will return a node which allocates
        //	the necessary stack space for any variables that where needed
        //	-- the argument is the node for the code to be executed in the
        //	frame. If there were no stack variables allocated, the
        //	argument node will be returned.
        //
        //	endStackFrame() (no arguments) will finish the stack frame. Its
        // up 	to the caller to deal with the proper node type.
        //

        void newStackFrame();
        StackVariable* findStackVariable(StackVariable*) const;
        const StackVariable* findStackVariable(const StackVariable*) const;
        Node* endStackFrame(NodeList);
        int endStackFrame();

        //
        //	LValue handling
        //

        Node* referenceVariable(const Variable*);
        Node* dereferenceVariable(const Variable*);
        Node* dereferenceVariable(const char* name);
        Node* dereferenceLValue(Node*);

        Node* referenceMemberVariable(const MemberVariable*, Node*);

        Node* unresolvableReference(Name);
        Node* unresolvableStackReference(const StackVariable*);
        Node* unresolvableMemberReference(Name, Node*);

        //
        //	Patching unresolved symbols
        //

        void patchUnresolved();
        void patchFunction(Function*);

        //
        //  Function constant will return an expression of type ambiguous
        //  function for overloaded functions unless literal is true.
        //

        Node* functionConstant(const Function*, bool literal = false);

        //
        //  Tuples (anonymous classes)
        //

        Class* declareTupleType(SymbolList);

        Node* tupleNode(NodeList);

        //
        //  Lists
        //

        Node* listNode(NodeList);

        //
        //  Variant Type
        //

        VariantType* declareVariantType(const char*);

        VariantTagType* declareVariantTagType(const char*, const Type*);

        //
        //  Case statemen / expr
        //  begin returns held result
        //

        Node* beginCase(Node* switchExpr, const Function* predicate = 0);
        Node* finishCase(Node* switchVal, NodeList nl);
        Node* casePatternStatement(Node*, NodeList);
        Node* casePattern(Pattern*);
        bool casePattern(const Type*); // constructor only pattern

        //
        //  Classes.
        //
        //  declareClass() takes a name and a list of classes/interfaces for
        //  inheritance.
        //
        //  generateDefaults() generates default contructor(s) for the class.
        //

        Class* declareClass(const char*, SymbolList, bool globalScope = false);
        void generateDefaults(Class*);

        //
        //  Interfaces.
        //
        //

        Interface* declareInterface(const char*, SymbolList);
        void generateDefaults(Interface*);

        //
        //  TypeVariables
        //  Where appropriate, it adds the '
        //

        TypeVariable* declareTypeVariable(const char*);

        //
        //  Placeholder Parameter
        //

        //
        //  Documentation
        //

        void setDocumentationModule(Symbol*);
        void addDocumentation(Name n, Object* o, const Type* t = 0);
        Object* retrieveDocumentation(Name n);
        void clearDocumentation();

    private:
        Node* retainOrRelease(Node*, bool retain = true) const;

    private:
        Context* _context;
        Process* _process;
        const ScopeState* _scope;
        Symbol* _prefixScope;
        Module* _suffixModule;
        Symbol* _docmodule;
        StackVarVector _stackVariables;
        StackVarStack _stackVariableStack;
        OffsetStack _offsetStack;
        NodeVector _nodeStack;
        SymbolVector _symbolStack;
        NameVector _nameStack;
        const Type* _initializerType;
        bool _initializerGlobal;
        InitializerList _initializerList;
        IntList _intList;
        TypeStack _typeStack;
        CaseStateStack _caseStack;
        UnresolvedFunctions _unresolvedFunctions;
        int _stackOffset;
        Name _sourceName;
        int _line;
        int _char;
        Thread* _thread;
        bool _constReduce : 1;
        bool _allowUnresolved : 1;
        bool _throwOnError : 1;
        bool _simplify : 1;
        bool _mythread : 1;
        mutable bool _error : 1;
    };

    template <class T> T* NodeAssembler::findScopeOfType() const
    {
        for (const ScopeState* ss = _scope; ss; ss = ss->parent)
        {
            if (T* typePointer = dynamic_cast<T*>(ss->symbol))
            {
                return typePointer;
            }
        }

        return 0;
    }

    template <class T> T* NodeAssembler::findTypeInScope(Name name) const
    {
        //
        //	For some reason, gcc 3 can not call an explicit template
        //	function with a parameter type from another. It gets
        //	confused. Otherwise, this function would simply call
        //	Symbol::findSymbolOfType() directly instead of reimplementing
        //	it here. Maybe there was a change in ANSI C++ that caused this
        //	behavior, but I can't see why that would be.
        //

        if (_prefixScope)
        {
            if (Symbol* s = _prefixScope->findSymbol(name))
            {
                for (s = s->firstOverload(); s; s = s->nextOverload())
                {
                    if (T* typePointer = dynamic_cast<T*>(s))
                        return typePointer;
                }
            }
        }
        else
        {
            for (const ScopeState* ss = _scope; ss; ss = ss->parent)
            {
                Symbol* scope = ss->symbol;
                if (Symbol* s = scope->findSymbol(name))
                {
                    for (s = s->firstOverload(); s; s = s->nextOverload())
                    {
                        if (T* typePointer = dynamic_cast<T*>(s))
                            return typePointer;
                    }
                }
            }
        }

        return 0;
    }

    template <class T>
    bool NodeAssembler::findTypesInScope(
        Name name, typename STLVector<const T*>::Type& array) const
    {
        array.clear();

        if (_prefixScope)
        {
            Symbol::ConstSymbolVector syms;
            _prefixScope->findSymbols(name, syms);

            for (size_t i = 0; i < syms.size(); i++)
            {
                if (const T* typePointer = dynamic_cast<const T*>(syms[i]))
                {
                    array.push_back(typePointer);
                }
            }
        }
        else
        {
            for (const ScopeState* ss = _scope; ss; ss = ss->parent)
            {
                Symbol* scope = ss->symbol;
                Symbol::ConstSymbolVector syms;
                scope->findSymbols(name, syms);

                for (size_t i = 0; i < syms.size(); i++)
                {
                    if (const T* typePointer = dynamic_cast<const T*>(syms[i]))
                    {
                        array.push_back(typePointer);
                    }
                }
            }
        }

        return !array.empty();
    }

} // namespace Mu

#endif // __Mu__NodeAssembler__h__
