//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/NodeVisitor.h>
#include <Mu/Node.h>

namespace Mu
{
    using namespace std;

    NodeVisitor::NodeVisitor(Node* node)
        : _root(node)
        , _parent(0)
        , _index(0)
    {
    }

    NodeVisitor::NodeVisitor()
        : _root(0)
        , _parent(0)
        , _index(0)
    {
    }

    NodeVisitor::~NodeVisitor() {}

    void NodeVisitor::set(Node* node) { _root = node; }

    void NodeVisitor::postOrderVisit(Node*, int)
    {
        // default is to do nothing
    }

    void NodeVisitor::preOrderVisit(Node*, int)
    {
        // default is to do nothing
    }

    void NodeVisitor::childVisit(Node*, Node*, size_t)
    {
        // default is to do nothing
    }

    //---

    void NodeVisitor::traverse()
    {
        if (_root)
            traverseRecursive(_root, 0);
    }

    void NodeVisitor::traverseRecursive(Node* node, int depth)
    {
        Node* saved_parent = _parent;
        preOrderVisit(node, depth);

        if (node)
        {
            _parent = node;
            size_t saved_index = _index;

            for (int i = 0, s = node->numArgs(); i < s; i++)
            {
                _index = i;
                childVisit(node, node->argNode(i), i);
                traverseRecursive(node->argNode(i), depth + 1);
            }

            _index = saved_index;
            _parent = saved_parent;
        }

        postOrderVisit(node, depth);
    }

} // namespace Mu
