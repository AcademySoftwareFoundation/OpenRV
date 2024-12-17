//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ASTNode.h>
#include <Mu/NodePatch.h>
#include <Mu/Node.h>
#include <Mu/Function.h>
#include <Mu/ReferenceType.h>
#include <Mu/ParameterVariable.h>
#include <Mu/Context.h>
#include <Mu/Exception.h>
#include <Mu/MemberVariable.h>
#include <Mu/MemberFunction.h>
#include <Mu/MachineRep.h>
#include <Mu/Unresolved.h>
#include <algorithm>

namespace Mu
{
    using namespace std;

    NodePatch::NodePatch(NodeAssembler* as, Function* f)
        : NodeVisitor(f->body())
        , _as(as)
        , _method(dynamic_cast<MemberFunction*>(f))
        , _function(f)
        , _slot(0)
    {
        if (_as->context()->verbose())
        {
            cout << ">>> Mu: patching " << f->fullyQualifiedName() << endl;
        }
    }

    NodePatch::~NodePatch() {}

    void NodePatch::preOrderVisit(Node* node, int depth)
    {
        const Symbol* s = node->symbol();

        if (dynamic_cast<const UnresolvedSymbol*>(s))
        {
            ASTNode* astnode = static_cast<ASTNode*>(node);
            _as->setScope(astnode->scope);
            astnode->preResolve(*this);
        }
    }

    void NodePatch::postOrderVisit(Node* node, int depth)
    {
        const Symbol* s = node->symbol();

        if (dynamic_cast<const UnresolvedSymbol*>(s))
        {
            ASTNode* astnode = static_cast<ASTNode*>(node);
            _as->setScope(astnode->scope);

            if (Node* newNode = astnode->resolve(*this))
            {
                if (astnode != newNode)
                {
                    if (root() == node)
                    {
                        //
                        //  Its the root of the function so we need to check
                        //  that the types match and fix if not.
                        //

                        if (Node* cn =
                                _as->cast(newNode, _function->returnType()))
                        {
                            newNode = cn;
                        }
                        else
                        {
                            _as->freportError(
                                newNode,
                                "Cannot cast from type \"%s\" to "
                                "type \"%s\"",
                                newNode->type()->fullyQualifiedName().c_str(),
                                _function->returnType()
                                    ->fullyQualifiedName()
                                    .c_str());

                            throw BadCastException();
                        }

                        _function->setBody(newNode);
                    }
                    else
                    {
                        node->releaseArgv();
                        node->deleteSelf();
                        nodeParent()->setArg(newNode, nodeIndex());
                    }
                }
            }
            else
            {
                _as->freportError(node,
                                  "Unresolvable expression in function \"%s\"",
                                  _function->fullyQualifiedName().c_str());

                throw UnresolvedFunctionException();
            }
        }
    }

    void NodePatch::childVisit(Node* node, Node* child, size_t index)
    {
        const Symbol* s = node->symbol();

        if (dynamic_cast<const UnresolvedSymbol*>(s))
        {
            ASTNode* astnode = static_cast<ASTNode*>(node);
            _as->setScope(astnode->scope);
            astnode->childVisit(*this, child, index);
        }
    }

    Node* NodePatch::patch()
    {
        _as->allowUnresolvedCalls(false);
        traverse();
        _as->allowUnresolvedCalls(true);
        return root();
    }

} // namespace Mu
