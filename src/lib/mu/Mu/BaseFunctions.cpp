//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/ASTNode.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Class.h>
#include <Mu/Context.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/GlobalVariable.h>
#include <Mu/Interface.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/PartialApplicator.h>
// #include <Mu/PartialEvaluator.h>
#include <Mu/FunctionSpecializer.h>
#include <Mu/PrimitiveObject.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Signature.h>
#include <Mu/Exception.h>
#include <Mu/StackVariable.h>
#include <Mu/VariantInstance.h>
#include <Mu/Thread.h>
#include <Mu/Variable.h>
#include <Mu/config.h>
#include <Mu/Unresolved.h>
#include <iostream>
#include <sstream>
#include <vector>

namespace Mu
{
    using namespace std;

    NoOp::NoOp(Context* context, const char* name)
        : Function(context, name, NoOp::node, Function::Pure, Function::Return,
                   "void", Function::End)
    {
    }

    NODE_IMPLEMENTATION(NoOp::node, void) { return; }

    //----------------------------------------------------------------------

    SimpleBlock::SimpleBlock(Context* context, const char* name)
        : Function(context, name, NodeFunc(0), Function::ContextDependent,
                   Function::Return, "void", Function::Args, "?",
                   Function::Optional, "?varargs", Function::Maximum,
                   Function::MaxArgValue, Function::End)
    {
    }

    const Type* SimpleBlock::nodeReturnType(const Node* node) const
    {
        const Node* n = node->argNode(node->numArgs() - 1);
        if (!n)
            throw UnresolvableSymbolException();
        return n->type();
    }

    NodeFunc SimpleBlock::func(Node* node) const
    {
        if (node)
        {
            const Type* t = node->type();
            const MachineRep* rep = t->machineRep();
            return rep->simpleBlockFunc();
        }
        else
        {
            return Function::func(node);
        }
    }

    //----------------------------------------------------------------------

    FixedFrameBlock::FixedFrameBlock(Context* context, const char* name)
        : Function(context, name, FixedFrameBlock::node,
                   Function::ContextDependent | Function::HiddenArgument,
                   Function::Return, "void", Function::Args, "?",
                   Function::Optional, "?varargs", Function::Maximum,
                   Function::MaxArgValue, Function::End)
    {
    }

    NodeFunc FixedFrameBlock::func(Node* node) const
    {
        if (node)
        {
            const Type* t = node->type();
            const MachineRep* rep = t->machineRep();
            return rep->frameBlockFunc();
        }
        else
        {
            return Function::func(node);
        }
    }

    const Type* FixedFrameBlock::nodeReturnType(const Node* node) const
    {
        return node->argNode(node->numArgs() - 1)->type();
    }

    NODE_IMPLEMENTATION(FixedFrameBlock::node, void)
    {
        //
        //	Allocate local variable space
        //

        Thread::StackRecord record(NODE_THREAD);
        record.newStackFrame(NODE_DATA(int));

        //
        //	Evaluate frame statements. See simple block above for comments.
        //

        const Node* n = NODE_THIS.argNode(0);

        for (int i = 1; n; i++)
        {
            NODE_ANY_TYPE_ARG(i - 1);
            n = NODE_THIS.argNode(i);
        }
    }

    //----------------------------------------------------------------------

    DynamicCast::DynamicCast(Context* context, const char* name)
        : Function(context, name, DynamicCast::node, Function::Mapped,
                   Function::Return, "?class_or_interface", Function::Args,
                   "?class_or_interface", "?class_or_interface", Function::End)
    {
    }

    const Type* DynamicCast::nodeReturnType(const Node* n) const
    {
        return dynamic_cast<const Type*>(n->argNode(0)->symbol());
    }

    NODE_IMPLEMENTATION(DynamicCast::node, Pointer)
    {
        const Symbol* sym = NODE_THIS.argNode(0)->symbol();

        if (const Class* c0 = dynamic_cast<const Class*>(sym))
        {
            if (ClassInstance* o = NODE_ARG_OBJECT(1, ClassInstance))
            {
                if (const Class* c1 = dynamic_cast<const Class*>(o->type()))
                {
                    if (ClassInstance* dobj = c1->dynamicCast(o, c0, true))
                    {
                        NODE_RETURN(dobj);
                    }
                    // if (c1->isA(c0)) NODE_RETURN((Pointer)o);
                }
            }
            else
            {
                //
                //	Let nil pass through
                //

                NODE_RETURN((Pointer)o);
            }
        }
        else if (const Interface* i = dynamic_cast<const Interface*>(sym))
        {
            if (Object* o = reinterpret_cast<Object*>(NODE_ARG(1, Pointer)))
            {
                if (const Class* c1 = dynamic_cast<const Class*>(o->type()))
                {
                    if (c1->implementation(i))
                    {
                        NODE_RETURN((Pointer)o);
                    }
                }
            }
            else
            {
                //
                //	Can't attempt to cast nil to an interface. Let it throw.
                //

                //  ALLOWING --NEEDS FIXING

                NODE_RETURN(Pointer(0));
            }
        }

        throw BadDynamicCastException(NODE_THREAD);
    }

    //----------------------------------------------------------------------

    DynamicActivation::DynamicActivation(Context* context,
                                         const String& returnType,
                                         const STLVector<String>::Type& args)
        : Function(context, "()", NodeFunc(0),
                   Function::DynamicActivation | Function::ContextDependent
                       | Function::MaybePure,
                   Function::Return, returnType.c_str(), Function::ArgVector,
                   &args)
    {
    }

    NodeFunc DynamicActivation::func(Node* node) const
    {
        if (node)
        {
            const Type* t;

            if (node->numArgs())
            {
                t = ((const Node*)node)->argNode(0)->type();
            }
            else
            {
                t = returnType();
            }

            const MachineRep* rep = t->machineRep();
            return rep->dynamicActivationFunc();
        }
        else
        {
            return Function::func(node);
        }
    }

    //----------------------------------------------------------------------

    ReturnFromFunction::ReturnFromFunction(Context* context, const char* name,
                                           bool arg)
        : Function(context, name, NodeFunc(0), Function::ContextDependent,
                   Function::Return, (arg ? "?" : "void"), Function::Args,
                   (arg ? "?" : 0), Function::End)
    {
    }

    const Type* ReturnFromFunction::nodeReturnType(const Node* n) const
    {
        const ReturnFromFunction* R =
            static_cast<const ReturnFromFunction*>(n->symbol());

        if (R->numArgs())
        {
            return dynamic_cast<const Type*>(n->argNode(0)->type());
        }
        else
        {
            return R->globalModule()->context()->voidType();
        }
    }

    NodeFunc ReturnFromFunction::func(Node* node) const
    {
        if (node)
        {
            const Type* t;

            if (node->numArgs())
            {
                t = ((const Node*)node)->argNode(0)->type();
            }
            else
            {
                t = returnType();
            }

            const MachineRep* rep = t->machineRep();
            return rep->functionReturnFunc();
        }
        else
        {
            return Function::func(node);
        }
    }

    //----------------------------------------------------------------------

    TailFuseReturn::TailFuseReturn(Context* context, const char* name)
        : Function(context, name, NodeFunc(0), Function::ContextDependent,
                   Function::Return, ("?"), Function::Args, "?", Function::End)
    {
    }

    const Type* TailFuseReturn::nodeReturnType(const Node* n) const
    {
        return n->argNode(0)->type();
    }

    NodeFunc TailFuseReturn::func(Node* node) const
    {
        if (node)
        {
            const Type* t = ((const Node*)node)->argNode(0)->type();
            const MachineRep* rep = t->machineRep();
            return rep->functionTailFuseFunc();
        }
        else
        {
            return Function::func(node);
        }
    }

    //----------------------------------------------------------------------

    Curry::Curry(Context* context, const char* name)
        : Function(context, name, Curry::node, Function::Pure, Function::Return,
                   "?function", Function::Args, "?function", "?function",
                   "?bool_rep", "?varargs", Function::End)
    {
    }

    Curry::~Curry() {}

    const Type* Curry::nodeReturnType(const Node* n) const
    {
        return dynamic_cast<const Type*>(n->argNode(0)->symbol());
    }

    FunctionObject* Curry::evaluate(Thread& t, FunctionObject* inObj,
                                    const Function::ArgumentVector& args,
                                    const vector<bool>& mask,
                                    bool dynamicDispatch)
    {
        //
        //  Never do partial application on the result of a lambda
        //  expression (its too difficult to archive). Instead do partial
        //  evaluation. The good side is that there will never be more
        //  than one level of indirection in multiple-curried lambda
        //  expressions. the bad side is that there will be more overhead
        //  upfront.
        //

        Process* p = t.process();
        const Function* F = inObj->function();
        const bool apply = !F->isLambda();

        try
        {
            if (apply)
            {
                PartialApplicator evaluator(F, p, &t, args, mask,
                                            dynamicDispatch);

                const FunctionType* rt = evaluator.result()->type();
                FunctionObject* outObj = new FunctionObject(rt);
                outObj->setDependent(inObj);
                outObj->setFunction(evaluator.result());
                return outObj;
            }
            else
            {
                FunctionSpecializer evaluator(F, p, &t);
                evaluator.partiallyEvaluate(args, mask);

                const FunctionType* rt = evaluator.result()->type();
                FunctionObject* outObj = new FunctionObject(rt);
                outObj->setFunction(evaluator.result());
                return outObj;
            }
        }
        catch (Exception& e)
        {
            ProgramException exc(t);
            exc.message() = e.message();
            exc.message() += " during partial ";
            exc.message() += (apply ? "application" : "evaluation");
            exc.message() += " of ";
            exc.message() += F->name().c_str();
            throw exc;
        }
    }

    NODE_IMPLEMENTATION(Curry::node, Pointer)
    {
        Process* p = NODE_THREAD.process();
        Context* c = p->context();
        FunctionObject* o = NODE_ARG_OBJECT(1, FunctionObject);
        bool d = NODE_ARG(2, bool);
        const Function* F = o->function();
        Function::ArgumentVector args(F->numArgs() + F->numFreeVariables());
        vector<bool> mask(args.size());

        for (int i = 0, s = args.size(); i < s; i++)
        {
            mask[i] = NODE_THIS.argNode(i + 3)->symbol() != c->noop();

            if (mask[i])
            {
                args[i] = NODE_ANY_TYPE_ARG(i + 3);
            }
        }

        NODE_RETURN(Pointer(evaluate(NODE_THREAD, o, args, mask, d)));
    }

    //----------------------------------------------------------------------

    DynamicPartialEvaluate::DynamicPartialEvaluate(Context* context,
                                                   const char* name)
        : Function(context, name, DynamicPartialEvaluate::node, Function::Pure,
                   Function::Return, "?function", Function::Args, "?function",
                   "?function", "?varargs", Function::End)
    {
    }

    DynamicPartialEvaluate::~DynamicPartialEvaluate() {}

    const Type* DynamicPartialEvaluate::nodeReturnType(const Node* n) const
    {
        return dynamic_cast<const Type*>(n->argNode(0)->symbol());
    }

    NODE_IMPLEMENTATION(DynamicPartialEvaluate::node, Pointer)
    {
        typedef FunctionSpecializer Generator;

        FunctionObject* f = NODE_ARG_OBJECT(1, FunctionObject);
        Generator::ArgumentVector args(f->function()->numArgs()
                                       + f->function()->numFreeVariables());
        Generator::ArgumentMask mask(args.size());
        Process* p = NODE_THREAD.process();
        Context* c = p->context();

        for (int i = 0; i < args.size(); i++)
        {
            mask[i] = NODE_THIS.argNode(i + 2)->symbol() != c->noop();

            if (mask[i])
            {
                args[i] = NODE_ANY_TYPE_ARG(i + 2);
            }
        }

        try
        {
            Generator evaluator(f->function(), p, &NODE_THREAD);
            evaluator.partiallyEvaluate(args, mask);

            const FunctionType* rt = evaluator.result()->type();
            assert(rt == NODE_THIS.argNode(0)->type());
            FunctionObject* o = new FunctionObject(rt);
            o->setFunction(evaluator.result());
            NODE_RETURN(Pointer(o));
        }
        catch (Exception& e)
        {
            ProgramException exc(NODE_THREAD);
            exc.message() = e.message();
            exc.message() += " during partial evaluation of ";
            exc.message() += f->function()->name().c_str();
            throw exc;
        }
    }

    //----------------------------------------------------------------------

    DynamicPartialApplication::DynamicPartialApplication(Context* context,
                                                         const char* name)
        : Function(context, name, DynamicPartialApplication::node,
                   Function::Pure, Function::Return, "?function",
                   Function::Args, "?function", "?function", "?varargs",
                   Function::End)
    {
    }

    DynamicPartialApplication::~DynamicPartialApplication() {}

    const Type* DynamicPartialApplication::nodeReturnType(const Node* n) const
    {
        return dynamic_cast<const Type*>(n->argNode(0)->symbol());
    }

    NODE_IMPLEMENTATION(DynamicPartialApplication::node, Pointer)
    {
        //
        //  Never do partial application on the result of a lambda
        //  expression (its too difficult to archive). Instead do
        //  partial evaluation. The good side is that there will never
        //  be more than one level of indirection in multiple-curried
        //  lambda expressions. the bad side is that there will be
        //  more overhead upfront.
        //

        Process* p = NODE_THREAD.process();
        Context* c = p->context();
        FunctionObject* f = NODE_ARG_OBJECT(1, FunctionObject);
        bool apply = f->function()->isLambda();

        try
        {
            if (apply)
            {
                typedef PartialApplicator Generator;
                Generator::ArgumentVector args(
                    f->function()->numArgs()
                    + f->function()->numFreeVariables());
                Generator::ArgumentMask mask(args.size());

                for (int i = 0; i < args.size(); i++)
                {
                    mask[i] = NODE_THIS.argNode(i + 2)->symbol() != c->noop();
                    if (mask[i])
                    {
                        args[i] = NODE_ANY_TYPE_ARG(i + 2);
                    }
                }

                Generator evaluator(f->function(), p, &NODE_THREAD, args, mask);

                const FunctionType* rt = evaluator.result()->type();
                assert(rt == NODE_THIS.argNode(0)->type());
                FunctionObject* o = new FunctionObject(rt);
                o->setDependent(f);
                o->setFunction(evaluator.result());
                NODE_RETURN(Pointer(o));
            }
            else
            {
                typedef FunctionSpecializer Generator;
                Generator::ArgumentVector args(
                    f->function()->numArgs()
                    + f->function()->numFreeVariables());
                Generator::ArgumentMask mask(args.size());

                for (int i = 0; i < args.size(); i++)
                {
                    mask[i] = NODE_THIS.argNode(i + 2)->symbol() != c->noop();

                    if (mask[i])
                    {
                        args[i] = NODE_ANY_TYPE_ARG(i + 2);
                    }
                }

                Generator evaluator(f->function(), p, &NODE_THREAD);
                evaluator.partiallyEvaluate(args, mask);

                const FunctionType* rt = evaluator.result()->type();
                assert(rt == NODE_THIS.argNode(0)->type());
                FunctionObject* o = new FunctionObject(rt);
                o->setFunction(evaluator.result());
                NODE_RETURN(Pointer(o));
            }
        }
        catch (Exception& e)
        {
            ProgramException exc(NODE_THREAD);
            exc.message() = e.message();
            exc.message() += " during partial ";
            exc.message() += (apply ? "application" : "evaluation");
            exc.message() += " of ";
            exc.message() += f->function()->name().c_str();
            throw exc;
        }
    }

    //----------------------------------------------------------------------

    NonPrimitiveCondExr::NonPrimitiveCondExr(Context* context, const char* name)
        : Function(context, name, NonPrimitiveCondExr::node, Function::Pure,
                   Function::Return, "?non_primitive_or_nil", Function::Args,
                   "bool", "?non_primitive_or_nil", "?non_primitive_or_nil",
                   Function::End)
    {
    }

    NonPrimitiveCondExr::~NonPrimitiveCondExr() {}

    const Type* NonPrimitiveCondExr::nodeReturnType(const Node* n) const
    {
        const Type* t = n->argNode(1)->type();

        if (t == n->symbol()->globalModule()->context()->nilType())
        {
            return n->argNode(2)->type();
        }
        else
        {
            return t;
        }
    }

    NODE_IMPLEMENTATION(NonPrimitiveCondExr::node, Pointer)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, Pointer)
                                      : NODE_ARG(2, Pointer));
    }

    //----------------------------------------------------------------------

    VariantMatch::VariantMatch(Context* context, const char* name)
        : Function(context, name, VariantMatch::node, Function::None,
                   Function::Return, "void", Function::Args, "?reference", "?",
                   Function::Optional, "?varargs", Function::Maximum,
                   Function::MaxArgValue, Function::End)
    {
    }

    VariantMatch::~VariantMatch() {}

    NODE_IMPLEMENTATION(VariantMatch::node, void)
    {
        //
        //  Get result of assignment
        //

        VariantInstance* i =
            *reinterpret_cast<VariantInstance**>(NODE_ARG(0, Pointer));

        //
        //  Eval argument corresponding to tag
        //

        if (i)
        {
            const VariantTagType* tt = i->tagType();
            size_t index = tt->index() + 1;
            if (index >= NODE_THIS.numArgs())
                throw MissingMatchException(NODE_THREAD);
            NODE_ANY_TYPE_ARG(index);
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    //----------------------------------------------------------------------

    namespace BaseFunctions
    {

        NODE_IMPLEMENTATION(dereference, Pointer)
        {
            Pointer* i = reinterpret_cast<Pointer*>(NODE_ARG(0, Pointer));
            NODE_RETURN(*i);
        }

        NODE_IMPLEMENTATION(assign, Pointer)
        {
            Pointer* i = reinterpret_cast<Pointer*>(NODE_ARG(0, Pointer));
            Pointer o = NODE_ARG(1, Pointer);
            *i = o;
            NODE_RETURN(Pointer(i));
        }

        NODE_IMPLEMENTATION(eq, bool)
        {
            Pointer a = NODE_ARG(0, Pointer);
            Pointer b = NODE_ARG(1, Pointer);
            NODE_RETURN(a == b);
        }

        NODE_IMPLEMENTATION(neq, bool)
        {
            Pointer a = NODE_ARG(0, Pointer);
            Pointer b = NODE_ARG(1, Pointer);
            NODE_RETURN(a != b);
        }

        NODE_IMPLEMENTATION(print, void)
        {
            if (Object* obj = NODE_ARG_OBJECT(0, Object))
            {
                obj->type()->outputValue(cout, Value(obj));
            }
            else
            {
                cout << "nil" << endl;
            }
        }

        NODE_IMPLEMENTATION(equals, bool)
        {
            ClassInstance* a = NODE_ARG_OBJECT(0, ClassInstance);
            ClassInstance* b = NODE_ARG_OBJECT(1, ClassInstance);
            NODE_RETURN(a == b);
        }

        NODE_IMPLEMENTATION(unresolved, void)
        {
            const ASTNode* astnode = static_cast<const ASTNode*>(&NODE_THIS);
            const Context* c = NODE_THREAD.context();
            Name n;

            if (const ASTName* nnode = dynamic_cast<const ASTName*>(astnode))
            {
                n = nnode->name;
            }
            else if (const ASTSymbol* snode =
                         dynamic_cast<const ASTSymbol*>(astnode))
            {
                n = snode->symbol->fullyQualifiedName();
            }

            ostringstream str;
            str << " \"" << n.c_str() << "\""
                << " at " << astnode->sourceFileName().c_str() << ", line "
                << astnode->linenum() << ", char " << astnode->charnum();

            string s = str.str();

            if (NODE_THIS.symbol() == c->unresolvedCall())
            {
                UnresolvedFunctionException exc(NODE_THREAD);
                exc.message() += s.c_str();
                throw exc;
            }
            else
            {
                UnresolvedReferenceException exc(NODE_THREAD);
                exc.message() += s.c_str();
                throw exc;
            }
        }

        NODE_IMPLEMENTATION(abstract, void)
        {
            AbstractCallException exc(NODE_THREAD);
            exc.message() += " \"";
            exc.message() += NODE_THIS.symbol()->name().c_str();
            exc.message() += "\"";
            throw exc;
        }

        NODE_IMPLEMENTATION(classAllocate, Pointer)
        {
            const Class* c =
                static_cast<const Class*>(NODE_THIS.symbol()->scope());
            NODE_RETURN(ClassInstance::allocate(c));
        }

        NODE_IMPLEMENTATION(objIdent, Pointer)
        {
            NODE_RETURN((Pointer)NODE_ARG_OBJECT(0, Pointer));
        }

    } // namespace BaseFunctions

    AssignAsReference::AssignAsReference(Context* c)
        : Function(c, "=", BaseFunctions::assign, Function::None,
                   Function::Return, "?", Function::Args,
                   "?non_primitive_reference", "?non_primitive_or_nil",
                   Function::End)
    {
    }

    AssignAsReference::~AssignAsReference() {}

    const Type* AssignAsReference::nodeReturnType(const Node* n) const
    {
        return n->argNode(0)->type();
    }

    PatternTest::PatternTest(Context* c)
        : Function(c, "__pattern_test", PatternTest::node, Function::None,
                   Function::Return,
                   c->boolType()->fullyQualifiedName().c_str(), Function::Args,
                   "?", Function::Optional, "?", Function::End)
    {
    }

    PatternTest::~PatternTest() {}

    NODE_IMPLEMENTATION(PatternTest::node, bool)
    {
        Thread::JumpRecord record(NODE_THREAD, JumpReturnCode::PatternFail);
        int rv = SETJMP(NODE_THREAD.jumpPoint());
        bool b = false;

        switch (rv)
        {
        case JumpReturnCode::PatternFail:
            NODE_THREAD.jumpPointRestore();
            break;

        case JumpReturnCode::NoJump:
            if (NODE_THIS.numArgs() == 1)
            {
                NODE_ANY_TYPE_ARG(0);
            }
            else
            {
                if (NODE_THIS.argNode(0)->type() == NODE_THIS.type())
                {
                    if (b = NODE_ARG(0, bool))
                        NODE_ANY_TYPE_ARG(1);
                }
                else
                {
                    NODE_ANY_TYPE_ARG(0);
                    NODE_ANY_TYPE_ARG(1);
                    b = true;
                }
            }
            break;

        default:
            abort();
        }

        NODE_RETURN(b);
    }

    BoolPatternTest::BoolPatternTest(Context* c)
        : Function(c, "__bool_pattern_test", BoolPatternTest::node,
                   Function::None, Function::Return,
                   c->boolType()->fullyQualifiedName().c_str(), Function::Args,
                   "?", Function::Optional, "?", Function::End)
    {
    }

    BoolPatternTest::~BoolPatternTest() {}

    NODE_IMPLEMENTATION(BoolPatternTest::node, bool)
    {
        if (!NODE_ARG(0, bool))
        {
            NODE_THREAD.jump(JumpReturnCode::PatternFail, 1);
        }

        NODE_RETURN(true);
    }

    CaseTest::CaseTest(Context* c)
        : Function(c, "__case_test", CaseTest::node, Function::None,
                   Function::Return,
                   c->boolType()->fullyQualifiedName().c_str(), Function::Args,
                   "?reference", "?", Function::Optional, "?varargs",
                   Function::Maximum, Function::MaxArgValue, Function::End)
    {
    }

    CaseTest::~CaseTest() {}

    NODE_IMPLEMENTATION(CaseTest::node, bool)
    {
        NODE_ANY_TYPE_ARG(0);

        for (size_t i = 1; NODE_THIS.argNode(i); i++)
        {
            if (NODE_ARG(i, bool))
                NODE_RETURN(true);
        }

        NODE_RETURN(false);
    }

    //----------------------------------------------------------------------

    PatternBlock::PatternBlock(Context* context, const char* name)
        : Function(context, name, NodeFunc(0), Function::ContextDependent,
                   Function::Return, "void", Function::Args, "?",
                   Function::Optional, "?varargs", Function::Maximum,
                   Function::MaxArgValue, Function::End)
    {
    }

    const Type* PatternBlock::nodeReturnType(const Node* node) const
    {
        const Node* n = node->argNode(node->numArgs() - 1);
        if (!n)
            throw UnresolvableSymbolException();
        return n->type();
    }

    NodeFunc PatternBlock::func(Node* node) const
    {
        if (node)
        {
            const Type* t = node->type();
            const MachineRep* rep = t->machineRep();
            return rep->patternBlockFunc();
        }
        else
        {
            return Function::func(node);
        }
    }

} // namespace Mu
