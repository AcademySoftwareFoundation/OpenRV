#ifndef __Mu__NodeVisitor__h__
#define __Mu__NodeVisitor__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <sys/types.h>

namespace Mu
{

    class Node;

    //
    //  class NodeVisitor
    //
    //  Visit each node in a node tree. There are multiple methods to
    //  traverse the tree using a NodeVisitor. The tree can be edited from
    //  this class.
    //
    //  Some typical uses of a NodeVisitor are for printing a Node tree,
    //  collapsing constant expressions, sub-expression elimination.
    //

    class NodeVisitor
    {
    public:
        explicit NodeVisitor(Node*);
        NodeVisitor();
        ~NodeVisitor();

        //
        //	Traversal methods
        //

        void traverse();

        Node* root() const { return _root; }

    protected:
        virtual void preOrderVisit(Node*, int depth);
        virtual void postOrderVisit(Node*, int depth);
        virtual void childVisit(Node*, Node*, size_t);

        size_t nodeIndex() const { return _index; }

        Node* nodeParent() const { return _parent; }

        void set(Node*);

    private:
        void traverseRecursive(Node*, int);

    private:
        Node* _root;
        Node* _parent;
        size_t _index;
    };

} // namespace Mu

#endif // __Mu__NodeVisitor__h__
