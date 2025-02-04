//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/NodeSimplifier.h>
#include <Mu/NodeAssembler.h>
#include <Mu/Function.h>
#include <Mu/Context.h>

namespace Mu
{
    using namespace std;

    NodeSimplifier::NodeSimplifier(NodeAssembler* as)
        : NodeVisitor()
        , _as(as)
        , _context(as->context())
    {
    }

    Node* NodeSimplifier::operator()(Node* n)
    {
        set(n);
        traverse();
        return root();
    }

    void NodeSimplifier::preOrderVisit(Node* n, int depth)
    {
        //
        //  This is all very hacked to get things started. What's really
        //  needed here is basically an LR() parser framework to identify
        //  sub-trees and exectute actions on the non-terminals
        //

        const Symbol* s = n->symbol();

        if (s == _context->fixedFrameBlock())
        {
            //
            //  If its a fixed frame block and the frame size is 0 convert
            //  it into a simple block in place.
            //

            DataNode* dn = static_cast<DataNode*>(n);
            int size = dn->_data._int;

            if (size == 0)
            {
                const Function* f = _context->simpleBlock();
                n->_symbol = f;
                n->_func = f->func(n);
            }
        }

        if (s == _context->simpleBlock())
        {
            //
            //  If its a simple frame block and it has only a single
            //  argument just return the argument.
            //

            if (n->numArgs() == 1)
            {
                Node* nn = n->_argv[0];
                n->_argv[0] = 0;

                if (nodeParent() && nodeParent() != n)
                {
                    nodeParent()->_argv[nodeIndex()] = nn;
                }
                else
                {
                    set(nn);
                }

                n->deleteSelf();
            }
        }
    }

} // namespace Mu
