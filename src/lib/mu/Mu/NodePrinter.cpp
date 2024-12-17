//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/Context.h>
#include <Mu/ASTNode.h>
#include <Mu/Node.h>
#include <Mu/NodePrinter.h>
#include <Mu/Symbol.h>
#include <Mu/Type.h>
#include <Mu/Unresolved.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    NodePrinter::NodePrinter(Node* n, ostream& o, NodePrinter::Style s)
        : NodeVisitor(n)
        , _ostream(&o)
        , _style(s)
        , _state(0)
    {
    }

    NodePrinter::NodePrinter(Node* n, ostream& o, Type::ValueOutputState& state,
                             NodePrinter::Style s)
        : NodeVisitor(n)
        , _ostream(&o)
        , _style(s)
        , _state(&state)
    {
    }

    NodePrinter::~NodePrinter() {}

    void NodePrinter::preOrderVisit(Node* node, int depth)
    {
        if (_style == Lispy)
        {
            const Context* c = node->symbol()->context();

            if (!node)
            {
                out() << "*NIL*";
                return;
            }

            int n = node->numArgs();
            const Symbol* s = node->symbol();

            if (node->type() == c->unresolvedType())
            {
                out() << "(";
                ASTNode* astnode = static_cast<ASTNode*>(node);
                out() << astnode->symbol()->name();

                if (ASTName* astname = dynamic_cast<ASTName*>(astnode))
                {
                    cout << "[" << astname->name << "]";
                }
                else if (ASTSymbol* astsym = dynamic_cast<ASTSymbol*>(astnode))
                {
                    cout << "[" << astsym->symbol->fullyQualifiedName() << "]";
                }
            }
            else if (n == 0)
            {
                if (const Type* type = dynamic_cast<const Type*>(s))
                {
                    DataNode* dnode = static_cast<DataNode*>(node);
                    if (_state)
                    {
                        type->outputValueRecursive(
                            out(),
                            type->machineRep()->valuePointer(dnode->_data),
                            *_state);
                    }
                    else
                    {
                        type->outputValue(out(), dnode->_data);
                    }
                }
                else if (const Function* f = dynamic_cast<const Function*>(s))
                {
                    out() << "(";
                    out() << node->symbol()->fullyQualifiedName();
                }
                else
                {
                    out() << node->symbol()->fullyQualifiedName();
                }
            }
            else
            {
                out() << "(";

                if (dynamic_cast<const UnresolvedCall*>(s))
                {
                    ASTCall* astcall = static_cast<ASTCall*>(node);
                    out() << astcall->name << "*";
                }
                else
                {
                    out() << node->symbol()->fullyQualifiedName();
                }
            }
        }
    }

    void NodePrinter::postOrderVisit(Node* node, int depth)
    {
        if (_style == Tree)
        {
            for (int i = 0; i < depth; i++)
                out() << " ";

            node->symbol()->outputNode(out(), node);

            out() << endl << flush;
        }
        else if (_style == Lispy)
        {
            if (!node)
                return;

            int n = node->numArgs();
            const Symbol* s = node->symbol();
            const Type* type = dynamic_cast<const Type*>(s);
            const Function* f = dynamic_cast<const Function*>(s);

            if ((n == 0 || type) && !f)
            {
                // nothing
            }
            else
            {
                out() << ")";
            }
        }
    }

    void NodePrinter::childVisit(Node* parent, Node* child, size_t)
    {
        if (_style == Lispy)
        {
            out() << " ";
        }
    }

} // namespace Mu
