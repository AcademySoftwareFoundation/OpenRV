#ifndef __Mu__NodePatch__h__
#define __Mu__NodePatch__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/NodeVisitor.h>
#include <Mu/NodeAssembler.h>
#include <vector>

namespace Mu
{
    class UnresolvedCall;
    class RootSymbol;

    //
    //  A NodeVisitor that patches previously unknown functions, fills in
    //  stubs, etc. Used for back-patching during parsing primarily.
    //

    class NodePatch : public NodeVisitor
    {
    public:
        //
        //  Types
        //

        typedef NodeAssembler::NodeList NodeList;
        typedef STLVector<const Symbol*>::Type SymbolStack;

        //
        //  NodePatch API
        //

        explicit NodePatch(NodeAssembler*, Function*);
        ~NodePatch();

        Node* patch();

        Function* function() const { return _function; }

        bool method() const { return _method; }

        NodeAssembler& as() { return *_as; }

        Node* parent() { return nodeParent(); }

        size_t index() const { return nodeIndex(); }

    protected:
        virtual void preOrderVisit(Node*, int depth);
        virtual void postOrderVisit(Node*, int depth);
        virtual void childVisit(Node*, Node*, size_t);

    private:
        Function* _function;
        NodeAssembler* _as;
        Name _this;
        Node** _slot;
        bool _method;
    };

} // namespace Mu

#endif // __Mu__NodePatch__h__
