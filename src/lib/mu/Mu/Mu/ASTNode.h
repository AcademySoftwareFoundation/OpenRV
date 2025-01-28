#ifndef __Mu__ASTNode__h__
#define __Mu__ASTNode__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Node.h>
#include <Mu/Name.h>
#include <Mu/NodePatch.h>
#include <Mu/BaseFunctions.h>

namespace Mu
{
    class NodeAssembler;
    class StackVariable;
    class NodePatch;

    //
    //  Base of the ASTNode hierarchy. ASTNodes should be completely
    //  removed by the end of parsing.
    //
    //  ASTNodes are AnnotatedNodes so they have source file line
    //  information. They also store the current scope that existed when
    //  the ASTNode was created. This is the scope that any declarations
    //  and lookups will occur in when resolving the ASTNode tree for the
    //  given ASTNode.
    //

    class ASTNode : public AnnotatedNode
    {
    public:
        typedef NodeAssembler::ScopeState ScopeState;
        typedef NodeAssembler::NodeList NodeList;

        ASTNode(NodeAssembler& as, int numArgs, const Symbol* s)
            : AnnotatedNode(numArgs, BaseFunctions::unresolved, s, as.lineNum(),
                            as.charNum(), as.sourceName())
            , scope(as.scopeState())
        {
        }

        virtual ~ASTNode();

        const ScopeState* scope;

        //
        //  The NodePatch's NodeAssembler will have its scope restored to
        //  this ASTNode's scope state prior to calling these.
        //

        virtual Node* preResolve(NodePatch&);
        virtual Node* resolve(NodePatch&) = 0;
        virtual void childVisit(NodePatch&, Node*, size_t);
    };

    //
    //  ASTName holds a single name
    //

    class ASTName : public ASTNode
    {
    public:
        ASTName(NodeAssembler& as, int numArgs, const Symbol* s, Name n)
            : ASTNode(as, numArgs, s)
            , name(n)
        {
        }

        Name name;
    };

    //
    //  ASTSymbol holds a single symbol
    //

    class ASTSymbol : public ASTNode
    {
    public:
        ASTSymbol(NodeAssembler& as, int numArgs, const Symbol* s,
                  const Symbol* sym)
            : ASTNode(as, numArgs, s)
            , symbol(sym)
        {
        }

        const Symbol* symbol;
    };

    //
    //  ASTCall is a call on an any name. This basically means its either
    //  a function, a memeber function, or a class instance with
    //  operator() defined (until that can be removed)
    //

    class ASTCall : public ASTName
    {
    public:
        ASTCall(NodeAssembler& as, int numArgs, const Symbol* s, Name n)
            : ASTName(as, numArgs, s, n)
        {
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTAssign
    //

    class ASTAssign : public ASTCall
    {
    public:
        ASTAssign(NodeAssembler& as, const Symbol* s, Node* lhs, Node* rhs);
        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTCast
    //

    class ASTCast : public ASTCall
    {
    public:
        ASTCast(NodeAssembler& as, int numArgs, const Symbol* s, Name n)
            : ASTCall(as, numArgs, s, n)
        {
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTMemberCall
    //

    class ASTMemberCall : public ASTCall
    {
    public:
        ASTMemberCall(NodeAssembler& as, int numArgs, const Symbol* s, Name n)
            : ASTCall(as, numArgs, s, n)
        {
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTMemberReference
    //

    class ASTMemberReference : public ASTName
    {
    public:
        ASTMemberReference(NodeAssembler& as, int numArgs, const Symbol* s,
                           Name n)
            : ASTName(as, numArgs, s, n)
        {
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTStackReference is a reference (or dereference) to a (not yet fixed)
    //  location on the stack.
    //

    class ASTStackReference : public ASTSymbol
    {
    public:
        ASTStackReference(NodeAssembler& as, int numArgs, const Symbol* s,
                          const Symbol* sym)
            : ASTSymbol(as, numArgs, s, sym)
            , dereference(false)
        {
        }

        bool dereference;
        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTReference is a reference to a name that probably has not been
    //  defined (yet). The name is probably being dereferenced.
    //

    class ASTReference : public ASTName
    {
    public:
        ASTReference(NodeAssembler& as, const Symbol* s, Name n)
            : ASTName(as, 0, s, n)
        {
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTDereference
    //

    class ASTDereference : public ASTNode
    {
    public:
        ASTDereference(NodeAssembler& as, const Symbol* s, Node* n)
            : ASTNode(as, 1, s)
        {
            setArg(n, 0);
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTDeclaration
    //

    class ASTDeclaration : public ASTName
    {
    public:
        ASTDeclaration(NodeAssembler& as, const Symbol* s, Name n)
            : ASTName(as, 0, s, n)
        {
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTStackDeclaration
    //

    class ASTStackDeclaration : public ASTName
    {
    public:
        ASTStackDeclaration(NodeAssembler& as, const Symbol* s, Name n,
                            StackVariable* sv)
            : ASTName(as, 0, s, n)
            , dummy(sv)
        {
        }

        StackVariable* dummy;
        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTListConstructor
    //

    class ASTListConstructor : public ASTNode
    {
    public:
        ASTListConstructor(NodeAssembler& as, int numArgs, Node** args,
                           const Symbol* s)
            : ASTNode(as, numArgs, s)
        {
            setArgs(args, numArgs);
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTTupleConstructor
    //

    class ASTTupleConstructor : public ASTNode
    {
    public:
        ASTTupleConstructor(NodeAssembler& as, int numArgs, Node** args,
                            const Symbol* s)
            : ASTNode(as, numArgs, s)
        {
            setArgs(args, numArgs);
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTTupleConstructor
    //

    class ASTIndexMember : public ASTNode
    {
    public:
        ASTIndexMember(NodeAssembler& as, int numArgs, Node** args,
                       const Symbol* s)
            : ASTNode(as, numArgs, s)
        {
            setArgs(args, numArgs);
        }

        virtual Node* resolve(NodePatch&);
    };

    //
    //  ASTForEach
    //

    class ASTForEach : public ASTNode
    {
    public:
        ASTForEach(NodeAssembler& as, Node** args, const Symbol* s)
            : ASTNode(as, 3, s)
        {
            setArgs(args, 3);
        }

        Node* declNode;

        virtual Node* resolve(NodePatch&);
        virtual void childVisit(NodePatch&, Node*, size_t);
    };

} // namespace Mu

#endif // __Mu__ASTNode__h__
