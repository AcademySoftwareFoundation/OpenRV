//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ASTNode.h>
#include <Mu/NodeAssembler.h>
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

    ASTNode::~ASTNode() {}

    Node* ASTNode::preResolve(NodePatch&) { return this; }

    void ASTNode::childVisit(NodePatch&, Node*, size_t) {}

    ASTAssign::ASTAssign(NodeAssembler& as, const Symbol* s, Node* lhs,
                         Node* rhs)
        : ASTCall(as, 2, s, as.context()->internName("="))
    {
        setArg(lhs, 0);
        setArg(rhs, 1);
    }

    Node* ASTAssign::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        Node* lhs = argNode(0);
        Node* rhs = argNode(1);
        const ReferenceType* lhsRefType =
            dynamic_cast<const ReferenceType*>(lhs->type());
        const Type* lhsType = lhsRefType ? lhsRefType->dereferenceType() : 0;

        if (lhs->type() == as.context()->unresolvedType())
        {
            if (lhs->symbol() == as.context()->unresolvedStackReference())
            {
                ASTStackReference* astnode =
                    static_cast<ASTStackReference*>(lhs);
                const StackVariable* sv =
                    static_cast<const StackVariable*>(astnode->symbol);

                if (sv->isImplicitlyTyped())
                {
                    const ReferenceType* reftype =
                        dynamic_cast<const ReferenceType*>(rhs->type());
                    const Type* t =
                        reftype ? reftype->dereferenceType() : rhs->type();
                    ((StackVariable*)sv)->setStorageClass(t);

                    Node* Nrhs = as.dereferenceLValue(rhs);
                    Node* Nlhs = as.referenceVariable(sv);
                    Node* n = as.assignmentOperator("=", Nlhs, Nrhs);

                    if (!n)
                    {
                        as.freportError(
                            this, "cannot assign type \"%s\" to %s.",
                            Nrhs->type()->fullyQualifiedName().c_str(),
                            sv->name().c_str());
                    }

                    return n;
                }
            }
        }
        else if (lhsType && !dynamic_cast<const UnresolvedType*>(lhs->type()))
        {
            if (lhsType != rhs->type())
            {
                if (Node* n = as.cast(rhs, lhsType))
                {
                    Node* rnode = as.assignmentOperator("=", lhs, n);

                    if (!rnode)
                    {
                        as.freportError(
                            this, "cannot assign \"%s\".",
                            n->type()->fullyQualifiedName().c_str());
                    }

                    return rnode;
                }
                else
                {
                    as.freportError(
                        this, "cannot cast \"%s\" to \"%s\" for assignment.",
                        rhs->type()->fullyQualifiedName().c_str(),
                        lhsType ? lhsType->fullyQualifiedName().c_str() : "?");
                }
            }
        }
        else
        {
            as.freportError(
                this, "Failed to resolve assignment from \"%s\" to \"%s\".",
                rhs->type()->fullyQualifiedName().c_str(),
                lhsType ? lhsType->fullyQualifiedName().c_str() : "?");
        }

        return ASTCall::resolve(visitor);
    }

    Node* ASTCall::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        const Symbol* s = symbol();
        Name uname = name;

        NodeAssembler::NodeList nl = as.newNodeListFromArgs(this);

        NodeAssembler::FunctionVector functions;
        STLVector<const Type*>::Type types;
        const MemberFunction* method = 0;
        Node* nthis = 0;

        for (const ScopeState* ss = scope; ss; ss = ss->parent)
        {
            //
            //    It could be either a direct reference to a function, or
            //    it could be a reference to a type constructor, in that
            //    case, we need to check in the type too.
            //

            if (const Type* t =
                    ss->symbol->findSymbolOfTypeByQualifiedName<Type>(uname))
            {
                if (const Function* f =
                        t->findSymbolOfTypeByQualifiedName<Function>(t->name()))
                {
                    for (const Function* fo = f->firstFunctionOverload(); fo;
                         fo = fo->nextFunctionOverload())
                    {
                        functions.push_back(fo);
                    }
                }
            }

            if (const Function* f =
                    ss->symbol->findSymbolOfTypeByQualifiedName<Function>(
                        uname))
            {
                for (const Function* fo = f->firstFunctionOverload(); fo;
                     fo = fo->nextFunctionOverload())
                {
                    functions.push_back(fo);

                    if (visitor.method()
                        && (method = dynamic_cast<const MemberFunction*>(fo)))
                    {
                        if (fo->scope() == f->scope())
                        {
                            //
                            //  There's a peer method to the one being
                            //  patched with the correct name. Insert
                            //  "this" at the front of the argument list
                            //  if the original parsing did not do so.
                            //

                            if (nl.size() == fo->numArgs() - 1)
                            {
                                Name thisname =
                                    as.context()->internName("this");
                                ParameterVariable* pthis =
                                    visitor.function()
                                        ->findSymbolOfType<ParameterVariable>(
                                            thisname);
                                nthis = as.dereferenceVariable(pthis);
                            }

                            break;
                        }
                    }
                }
            }
        }

        if (method && nthis)
        {
            if (Node* nn = as.callMethod(method, nthis, nl))
            {
                return nn;
            }
        }
        else if (functions.size())
        {
            if (Node* nn = as.callBestFunction(functions, nl))
            {
                return nn;
            }
        }

        as.freportError(this, "Unresolved function call to \"%s\"",
                        uname.c_str());

        throw UnresolvedFunctionException();
    }

    Node* ASTCast::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        const Symbol* s = symbol();
        Name uname = name;

        const Type* t = 0;

        for (const ScopeState* ss = scope; ss; ss = ss->parent)
        {
            if (t = ss->symbol->findSymbolOfTypeByQualifiedName<Type>(uname))
            {
                break;
            }
        }

        if (t)
        {
            if (Node* nn = as.cast(argNode(0), t))
            {
                return nn;
            }
        }

        as.freportError(this, "Cannot cast \"%s\" to \"%s\".",
                        argNode(0)->type()->fullyQualifiedName().c_str(),
                        uname.c_str());

        throw BadCastException();
    }

    Node* ASTMemberCall::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        Name n = name;
        const size_t nargs = numArgs();

        const Class* ctype = 0;

        if (nargs == 2 || nargs == 1)
        {
            Node* lhs = argNode(0);
            Node* rhs = nargs == 2 ? argNode(1) : 0;

            if (n == "()")
            {
                if (lhs->symbol() == as.context()->unresolvedMemberReference())
                {
                    ASTMemberReference* astref =
                        static_cast<ASTMemberReference*>(lhs);
                    Name n = astref->name;
                    Node* prefix = astref->argNode(0);
                    const Type* t = prefix->type();

                    if (t->isReferenceType())
                    {
                        t = static_cast<const ReferenceType*>(t)
                                ->dereferenceType();
                    }

                    if (const MemberFunction* f =
                            t->findSymbolOfType<MemberFunction>(n))
                    {
                        //
                        //  This is a method call
                        //

                        Node* nthis = astref->argNode(0);
                        NodeAssembler::NodeList nl =
                            rhs ? as.newNodeList(rhs) : as.emptyNodeList();
                        Node* nn = as.callMethod(f, nthis, nl);

                        as.removeNodeList(nl);
                        return nn;
                    }
                }
            }
        }

        throw UnresolvedReferenceException();
    }

    Node* ASTMemberReference::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        Name uname = name;
        Node* prefix = argNode(0);
        const Type* t = prefix->type();

        if (t->isReferenceType())
        {
            t = static_cast<const ReferenceType*>(t)->dereferenceType();
        }

        if (const MemberVariable* v =
                t->findSymbolOfType<MemberVariable>(uname))
        {
            if (Node* nn = as.referenceMemberVariable(v, prefix))
            {
                return nn;
            }
        }
        else if (const MemberFunction* f =
                     t->findSymbolOfType<MemberFunction>(uname))
        {
            if (visitor.parent()->symbol()
                == as.context()->unresolvedMemberCall())
            {
                // handle it up in the hierarchy
                return this;
            }
        }

        as.freportError(this,
                        "Unresolved member reference to \"%s\" in type \"%s\"",
                        uname.c_str(), t->fullyQualifiedName().c_str());

        throw UnresolvedReferenceException();
    }

    Node* ASTDereference::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        return as.dereferenceLValue(argNode(0));
    }

    Node* ASTStackReference::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        const StackVariable* sv = static_cast<const StackVariable*>(symbol);

        if (dereference)
        {
            if (Node* nn = as.dereferenceVariable(sv))
            {
                return nn;
            }
            else
            {
                as.freportError(this,
                                "Unresolved stack dereference of \"%s\" "
                                "(probably an internal error)",
                                sv->fullyQualifiedName().c_str());

                throw UnresolvedFunctionException();
            }
        }
        else
        {
            if (sv->storageClass() != as.context()->unresolvedType())
            {
                if (Node* nn = as.referenceVariable(sv))
                {
                    return nn;
                }
                else
                {
                    as.freportError(this,
                                    "Unresolved stack dereference of \"%s\" "
                                    "(probably an internal error)",
                                    sv->fullyQualifiedName().c_str());

                    throw UnresolvedFunctionException();
                }
            }
        }

        return this;
    }

    Node* ASTReference::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        Name uname = name;

        Symbol* s = as.nonAnonymousScope();
        bool inclass =
            dynamic_cast<Function*>(s) && dynamic_cast<Class*>(s->scope());

        if (inclass)
        {
            if (const MemberVariable* v =
                    visitor.function()
                        ->scope()
                        ->findSymbolOfType<MemberVariable>(uname))
            {
                Name thisname = as.context()->internName("this");
                ParameterVariable* pthis =
                    visitor.function()->findSymbolOfType<ParameterVariable>(
                        thisname);

                if (pthis)
                {
                    if (Node* prefix = as.dereferenceVariable(pthis))
                    {
                        if (Node* nn = as.referenceMemberVariable(v, prefix))
                        {
                            return nn;
                        }
                    }
                }
            }
            else if (const MemberFunction* f =
                         visitor.function()
                             ->scope()
                             ->findSymbolOfType<MemberFunction>(uname))
            {
                Name thisname = as.context()->internName("this");
                ParameterVariable* pthis =
                    visitor.function()->findSymbolOfType<ParameterVariable>(
                        thisname);

                if (pthis)
                {
                    if (Node* prefix = as.dereferenceVariable(pthis))
                    {
                        if (Node* nn = as.methodThunk(f, prefix))
                        {
                            return nn;
                        }
                    }
                }
            }
        }

        for (const ScopeState* ss = scope; ss; ss = ss->parent)
        {
            if (const Variable* var =
                    ss->symbol->findSymbolOfTypeByQualifiedName<Variable>(
                        uname))
            {
                return as.referenceVariable(var);
            }
        }

        as.freportError(this, "Unresolved reference to \"%s\"", uname.c_str());

        throw UnresolvedReferenceException();
    }

    Node* ASTDeclaration::resolve(NodePatch& visitor) { return this; }

    Node* ASTStackDeclaration::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();

        for (const ScopeState* ss = scope; ss; ss = ss->parent)
        {
            if (const StackVariable* svar =
                    ss->symbol->findSymbolOfType<StackVariable>(name))
            {
                return as.referenceVariable(svar);
            }
        }

        return 0;
    }

    Node* ASTListConstructor::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        NodeAssembler::NodeList nl = as.emptyNodeList();

        for (int i = 0; i < numArgs(); i++)
        {
            nl.push_back(argNode(i));
        }

        Node* rn = as.listNode(nl);
        as.removeNodeList(nl);
        return rn; // could be 0
    }

    Node* ASTTupleConstructor::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        NodeAssembler::NodeList nl = as.emptyNodeList();

        for (int i = 0; i < numArgs(); i++)
        {
            nl.push_back(argNode(i));
        }

        Node* rn = as.tupleNode(nl);
        as.removeNodeList(nl);
        return rn; // could be 0
    }

    Node* ASTIndexMember::resolve(NodePatch& visitor)
    {
        NodeAssembler& as = visitor.as();
        NodeAssembler::NodeList nl = as.emptyNodeList();

        for (int i = 1; i < numArgs(); i++)
        {
            nl.push_back(argNode(i));
        }

        Node* rn = as.memberOperator("[]", argNode(0), nl);
        as.removeNodeList(nl);

        return rn; // could be 0
    }

    void ASTForEach::childVisit(NodePatch& visitor, Node* child, size_t index)
    {
        NodeAssembler& as = visitor.as();

        if (index == 1)
        {
            //
            //  the collection expression has already been resolved (or
            //  not). So now we need to declare the iterator symbol before the
            //  body is resolved.
            //

            const Type* stype = argNode(0)->type();
            const Type* vtype = argNode(1)->type();

            if (stype->isCollection())
            {
                if (vtype->isUnresolvedType())
                {
                    if (const Mu::Type* ftype = stype->fieldType(0))
                    {
                        ASTStackDeclaration* astdecl =
                            static_cast<ASTStackDeclaration*>(argNode(1));

                        Variable::Attributes rw = Variable::ReadWrite;
                        Variable::Attributes rwi = rw | Variable::ImplicitType;

                        StackVariable* svar = new StackVariable(
                            as.context(), astdecl->name.c_str(), ftype,
                            astdecl->dummy->address(), rwi);

                        astdecl->scope->symbol->addSymbol(svar);
                        declNode = as.referenceVariable(svar);
                    }
                }
                else
                {
                    declNode = argNode(1);
                }
            }
            else
            {
                as.freportError(this,
                                "The for_each statement requires a collection; "
                                "Type %s is not a collection",
                                stype->fullyQualifiedName().c_str());

                throw UnresolvedFunctionException();
            }
        }
    }

    Node* ASTForEach::resolve(NodePatch& visitor)
    {
        if (!declNode)
            return 0;
        NodeAssembler& as = visitor.as();

        //
        //  NOTE: the ASTForEach arg order differs from the actual
        //  for_each node.
        //

        NodeAssembler::NodeList nl = as.newNodeList(declNode);
        nl.push_back(argNode(0));
        nl.push_back(argNode(2));

        Node* n = as.callBestFunction("__for_each", nl);
        as.removeNodeList(nl);

        return n;
    }

} // namespace Mu
