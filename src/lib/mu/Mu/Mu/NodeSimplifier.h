#ifndef __Mu__NodeSimplifier__h__
#define __Mu__NodeSimplifier__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/NodeVisitor.h>

namespace Mu
{
    class NodeAssembler;
    class Context;
    class Function;

    //
    //  class NodeSimplifier
    //
    //  Takes a node tree expression and applies various heuristics to do
    //  symbolic simplification.
    //

    class NodeSimplifier : protected NodeVisitor
    {
    public:
        explicit NodeSimplifier(NodeAssembler*);

        //
        //  This is the main entry point
        //

        Node* operator()(Node*);

    protected:
        virtual void preOrderVisit(Node*, int depth);

    private:
        NodeAssembler* _as;
        Context* _context;
    };

} // namespace Mu

#endif // __Mu__NodeSimplifier__h__
