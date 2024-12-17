#ifndef __Mu__Function__h__
#define __Mu__Function__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Node.h>
#include <Mu/Symbol.h>
#include <Mu/Type.h>
#include <Mu/Signature.h>
#include <vector>
#include <stdarg.h>

namespace Mu
{
    class ParameterVariable;
    class FreeVariable;
    class FunctionType;

    //
    //  class Function
    //
    //  Function is a symbol which provides a NodeFunc for a Node. The
    //  function symbol describes the function -- its arguments, its
    //  return Type properties, optimizing characteristics, etc.
    //

    class Function : public Symbol
    {
    public:
        //
        //  Types
        //  Note: requirements: sizeof(ArgKeyword) == sizeof(void*)
        //  on all architectures.
        //

        typedef APIAllocatable::STLVector<Value>::Type ArgumentVector;
        typedef STLVector<const Type*>::Type TypeVector;
        typedef STLVector<String>::Type StringVector;
        typedef STLVector<ParameterVariable*>::Type ParamList;
        typedef unsigned int Attributes;
        typedef const size_t ArgKeyword;

        typedef void(*CompiledFunction);

        //
        //  Attributes. These are passed into the constructor or init
        //  function. You should be as specific as possible with these
        //  since they are used for rewriting and symbolic optimizations.
        //
        //  What's MaybePure?  MaybePure is a assigned to higher-order
        //  functions that will produce pure functions given pure inputs
        //  and might produce impure functions otherwise. It means that
        //  the purity of the function is a function of its arguments. It
        //  could be said that the result is definitely pure regardless of
        //  whether the output function is pure or not, but this is
        //  currently not the way its being defined.
        //

        enum Attribute
        {
            None = 0,
            Operator = 1 << 0,           // multiple calling conventions
            MemberOperator = 1 << 1,     // First argument must match exactly
            Commutative = 1 << 2,        // Argument order independant
            Cast = 1 << 3,               // Cast type->type
            Lossy = 1 << 4,              // Cast only - Lossy cast
            Mapped = 1 << 5,             // Input set maps to output set
            NoSideEffects = 1 << 6,      // Function has no side effects
            ContextDependent = 1 << 7,   // _func Depends on arguments
            Native = 1 << 8,             // native function
            NonNative = 1 << 9,          // opposite of native (limited use)
            Retaining = 1 << 10,         // may retain some of its arguments
            DynamicActivation = 1 << 11, // a special case
            LambdaExpression = 1 << 12,  // function contains lambda expression
            HiddenArgument = 1 << 13,    // Uses a DataNode (hacky)
            DependentSideEffects = 1 << 14,
            NativeInlined = 1 << 15, // Native (compiled) function is inlined
            Generated = 1 << 16,     // Automatically generated

            Pure = NoSideEffects | Mapped,
            MaybePure = DependentSideEffects | Mapped,
        };

        //
        //  Keywords for constructors.
        //
        //  NOTE: These can't be an enum because some (all?) 64 bit
        //  architectures will pass enums as unsigned ints to variadic
        //  (variable number of arguments) functions. The argument parser
        //  in the constructors tries to distiguish between pointers and
        //  keywords and in doing so assumes they are the same size. The
        //  remidy is to make the keywords unsigned longs which *should*
        //  be the same size as an unsigned int on all platforms.
        //
        //  "Compiled" Keyword is used to pass in a native version of the
        //  function. For example if the the Function object is
        //  sin(float;float), the compiled function might be the sin()
        //  function in the C math library. In other words, a function
        //  which will produce the same results in C++.
        //

        static const ArgKeyword End = 0;
        static const ArgKeyword Return = 1;
        static const ArgKeyword Args = 2;
        static const ArgKeyword ArgVector = 3;
        static const ArgKeyword Parameters = 4;
        static const ArgKeyword Optional = 5;
        static const ArgKeyword Maximum = 6;
        static const ArgKeyword Compiled = 7;
        static const ArgKeyword MaxArgValue = (1 << MU_MAX_ARGUMENT_BITS) - 1;

        //
        //	Example call to constructor for a C-like language with an
        //	optional 3rd argument of a double.
        //
        //  Function("foo",			// Function: void
        //  foo(int,float,double)
        // 	         &fooFunc,		// node function
        //	         Function::None,           // no attributes
        //	         Function::Return, "void",
        //	         Function::Args, "int", "float", Function::Optional,
        //"double", 	         Function::End);
        //
        //	Note: Once you use the "Optional" argument keyword, all
        // remaining 	declared arguments are optional. The "Maximum" keyword
        // is useful 	when there are pseudo types (pattern types) supplied.
        // Here's an 	example:
        //
        //  Function("foo",
        // 	         &fooFunc,
        //	         Function::None,
        //	         Function::Return, "void",
        //	         Function::Args, "int", Function::Optional, "...",
        //				        Function::Maximum, 3,
        //	         Function::End);
        //
        //	This will match foo(1,2,3), foo(1,2), and foo(1), but not
        //	foo(1,2,3,4) if the "..." type is Mu::VarArg
        //
        //	If you want to declare named parameters, you might do so like
        //	this:
        //
        //  Function("foo",
        // 	         &fooFunc,
        //	         Function::None,
        //	         Function::Return, "void",
        //	         Function::Parameters,
        //		    new ParameterVariable("a", "int"),
        //		    new ParameterVariable("b", "int"),
        //		    new ParameterVariable("c", "float"),
        //	         Function::End);
        //
        //	or like this:
        //
        //  Function("foo",
        // 	         &fooFunc,
        //	         Function::None,
        //	         Function::Return, "void",
        //	         Function::ParameterList, nparams, paramlist,
        //	         Function::End);
        //
        //	Where plist is a pointer to an array of ParameterVariable* of
        //	size nparams.
        //

        //
        //	This constructor is used for native functions. attributes will
        //	automatically have Native set. You can override this by
        //	passing in NonNative.
        //

        Function(Context* context, const char* name, NodeFunc,
                 Attributes attributes, ...);

        //
        //	This constructor is used for non-native functions. NonNative
        //	is implied, but can be overridden by supplying Native in the
        //	attributes.
        //

        Function(Context* context, const char* name, const Type* returnType,
                 int nparams, ParameterVariable**, Node*,
                 Attributes attributes);

        Function(Context* context, const char* name, const Type* returnType,
                 int nparams, ParameterVariable**, NodeFunc,
                 Attributes attributes);

        //
        //	Destructor
        //

        virtual ~Function();

        //
        //	Symbol API
        //

        virtual void output(std::ostream&) const;
        virtual void symbolDependancies(ConstSymbolVector&) const;
        virtual void addSymbol(Symbol*);
        virtual String mangledName() const;

        //
        //  Overloading
        //

        bool isFunctionOverloaded() const;

        Function* firstFunctionOverload();
        const Function* firstFunctionOverload() const;

        Function* nextFunctionOverload();
        const Function* nextFunctionOverload() const;

        //
        //	Node management and construction
        //

        virtual const Type* nodeReturnType(const Node*) const;

        //
        //	Type returned by this function (from its signature -- this
        //	will be the same as the nodeReturnType().
        //

        const FunctionType* type() const; // function type not return type
        const Signature* signature() const;

        const Type* returnType() const;
        QualifiedName returnTypeName() const;

        bool hasReturn() { return _returns; }

        void hasReturn(bool t) { _returns = t; }

        //
        //	Stack size for non-native functions
        //

        size_t stackSize() const { return _stackSize; }

        void stackSize(int s) { _stackSize = s; }

        //
        //	Arguments to the function. Note that the args() may not exist
        //	in the future since they can potentially be discarded after
        //	symbol resolution.
        //
        //  Some parameters may be free parameters. These are parameters
        //  that correspond to free variables in the function. (They are
        //  hoisted to parameters). The free parameters will be the last
        //  numFreeVariables() parameters.
        //
        //  numArgs() is always number of "declared" parameters. The total
        //  number of paramter is numArgs() + the number of free variable
        //  parameters. Free parameters will be FreeVariables instead of
        //  ParameterVariables.
        //

        int numArgs() const;
        int numFreeVariables() const;

        int minimumArgs() const { return _minimumArgs; }

        int maximumArgs() const { return _maximumArgs; }

        int requiredArgs() const { return _requiredArgs; }

        QualifiedName argTypeName(int i) const;
        const Type* argType(int i) const;
        ParameterVariable* parameter(int i);
        const ParameterVariable* parameter(int i) const;

        //
        //	This will expand out any typePatterns that cause repeating to
        //	the specified size. Returns true if it could get to size
        //	arguments else false.
        //

        bool expandArgTypes(TypeVector&, size_t size) const;

        //
        //	The function body. Either a NodeFunc or a Node.
        //

        virtual NodeFunc func(Node* node = 0) const;

        Node* body() const { return _code; }

        void setBody(Node* n);

        CompiledFunction compiledFunction() const { return _compiledFunction; }

        void setReturnType(const Type*);

        //
        //	Attribute provided by the creater of the function.
        //

        bool isOperator() const { return _operator; }

        bool isCast() const { return _cast; }

        bool isCommutative() const { return _commutative; }

        bool isMapped() const { return _mapped; }

        bool isLossy() const { return _lossy; }

        bool isMemberOperator() const { return _memberOp; }

        bool isMethod() const { return _method; }

        bool hasSideEffects() const { return _sideEffects; }

        bool isMultiSigniture() const { return _multiSignature; }

        bool native() const { return _native; }

        bool hasParameters() const { return _hasParameters; }

        bool isRetaining() const { return _retaining; }

        bool isLambda() const { return _lambda; }

        bool hasDependentSideEffects() const { return _dependentSideEffects; }

        bool isContextDependent() const { return _contextDependent; }

        bool hasHiddenArgument() const { return _hiddenArgument; }

        bool isGenerated() const { return _generated; }

        bool isPure() const
        {
            return isMapped() && !hasSideEffects()
                   && !hasDependentSideEffects();
        }

        bool maybePure() const
        {
            return isMapped() && hasDependentSideEffects();
        }

        bool isDynamicActivation() const { return _dynamicActivation; }

        bool isVariadic() const { return _variadic; }

        bool isPolymorphic() const;

        bool hasUnresolvedStubs() const { return _unresolvedStubs; }

        bool isConstructor() const
        {
            return fullyQualifiedName() == returnTypeName();
        }

        Attributes baseAttributes() const;

        //
        //  Used by interfaces
        //

        size_t interfaceIndex() const { return _interfaceIndex; }

        void setInterfaceIndex(size_t i) { _interfaceIndex = i; }

        //
        //	matches() returns true if the name, return value, and arguments
        // all 	match.
        //

        virtual bool matches(const Function*) const;

    protected:
        explicit Function(Context*, const char* name);
        void init(NodeFunc, Attributes, va_list&);
        void init(Node*, const Type*, int, ParameterVariable**, Attributes);

        //
        //	Resolve symbols needs to convert the argument names and return
        //	type name into actual symbol pointers. Since its called
        //	lazily, this may happen at any time after the function is
        //	constructed -- hence the mutable members.
        //

        virtual bool resolveSymbols() const;

        typedef FunctionType FType;
        typedef Signature Sig;

    protected:
        mutable const Sig* _signature;
        mutable const FType* _type;
        ParamList _parameters;
        NodeFunc _func;
        Node* _code;
        CompiledFunction _compiledFunction;
        size_t _stackSize;
        size_t _interfaceIndex;
        unsigned int _minimumArgs : MU_MAX_ARGUMENT_BITS;
        unsigned int _maximumArgs : MU_MAX_ARGUMENT_BITS;
        unsigned int _requiredArgs : MU_MAX_ARGUMENT_BITS;
        bool _noAttributes : 1;
        bool _operator : 1;
        bool _cast : 1;
        bool _lossy : 1;
        bool _commutative : 1;
        bool _mapped : 1;
        bool _sideEffects : 1;
        bool _memberOp : 1;
        bool _method : 1;
        bool _hasParameters : 1;
        bool _contextDependent : 1;
        bool _native : 1;
        bool _returns : 1;
        bool _retaining : 1;
        bool _dynamicActivation : 1;
        bool _lambda : 1;
        bool _hiddenArgument : 1;
        bool _dependentSideEffects : 1;
        bool _inlinedNative : 1;
        bool _nativeExists : 1;
        bool _generated : 1;
        mutable bool _polymorphic : 1;
        mutable bool _variadic : 1;
        mutable bool _multiSignature : 1;
        mutable bool _unresolvedStubs : 1;

        friend class NodeAssembler;
    };

} // namespace Mu

//
//  Big ugly define which makes pulls a bunch of crap in to the local scope
//

#define USING_MU_FUNCTION_SYMBOLS                                         \
    const unsigned int NoHint = 0;                                        \
    Mu::Function::ArgKeyword Return = Mu::Function::Return;               \
    Mu::Function::ArgKeyword Compiled = Mu::Function::Compiled;           \
    Mu::Function::ArgKeyword Args = Mu::Function::Args;                   \
    Mu::Function::ArgKeyword Optional = Mu::Function::Optional;           \
    Mu::Function::ArgKeyword Maximum = Mu::Function::Maximum;             \
    Mu::Function::ArgKeyword End = Mu::Function::End;                     \
    Mu::Function::ArgKeyword Generated = Mu::Function::Generated;         \
    Mu::Function::ArgKeyword Parameters = Mu::Function::Parameters;       \
    Mu::Function::Attributes Retaining = Mu::Function::Retaining;         \
    typedef Mu::ParameterVariable Param;                                  \
    typedef Mu::FreeVariable FreeParam;                                   \
    typedef Mu::Function::CompiledFunction CompiledFunction;              \
    Mu::Function::Attributes None = Mu::Function::None;                   \
    Mu::Function::Attributes NativeInlined = Mu::Function::NativeInlined; \
    Mu::Function::Attributes CommOp =                                     \
        Mu::Function::Mapped | Mu::Function::Commutative                  \
        | Mu::Function::Operator | Mu::Function::NoSideEffects;           \
    Mu::Function::Attributes Op = Mu::Function::Mapped                    \
                                  | Mu::Function::Operator                \
                                  | Mu::Function::NoSideEffects;          \
    Mu::Function::Attributes Mapped =                                     \
        Mu::Function::Mapped | Mu::Function::NoSideEffects;               \
    Mu::Function::Attributes Pure =                                       \
        Mu::Function::Mapped | Mu::Function::NoSideEffects;               \
    Mu::Function::Attributes Cast = Mapped | Mu::Function::Cast;          \
    Mu::Function::Attributes Lossy = Cast | Mu::Function::Lossy;          \
    Mu::Function::Attributes AsOp =                                       \
        Mu::Function::MemberOperator | Mu::Function::Operator;

// Mu::Symbol::VariadicKeyword EndArguments = Mu::Symbol::EndArguments;

#endif // __Mu__Function__h__
