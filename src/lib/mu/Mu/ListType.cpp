//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ListType.h>
#include <Mu/List.h>
#include <Mu/Function.h>
#include <Mu/ReferenceType.h>
#include <Mu/MachineRep.h>
#include <Mu/BaseFunctions.h>
#include <Mu/MemberVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/ClassInstance.h>

namespace Mu
{
    using namespace std;

    ListType::ListType(Context* context, const char* name, const Type* type)
        : Class(context, name, 0)
        , _elementType(type)
    {
        _isMutable = false;
        _isCollection = true;
    }

    ListType::~ListType() {}

    void ListType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                        ValueOutputState& state) const
    {
        ClassInstance* obj = *reinterpret_cast<ClassInstance**>(vp);

        if (obj)
        {
            o << "[";

            for (List list(0, obj); *list; ++list)
            {
                list.type()->elementType()->outputValueRecursive(
                    o, list.valuePointer(), state);
                if (list.next())
                    o << ", ";
            }

            o << "]";
        }
        else
        {
            o << "nil";
        }
    }

    Type::MatchResult ListType::match(const Type* t, Bindings& b) const
    {
        if (const ListType* ltype = dynamic_cast<const ListType*>(t))
        {
            return elementType()->match(ltype->elementType(), b);
        }
        else
        {
            return Class::match(t, b);
        }
    }

    void ListType::load()
    {
        USING_MU_FUNCTION_SYMBOLS
        Class::load();

        //
        //  Generate reference type
        //

        String tname = name().c_str();
        String rtname = tname;
        rtname += "&";
        String ename = elementType()->fullyQualifiedName().c_str();
        String ername = ename + "&";

        const char* tn = tname.c_str();
        const char* rn = rtname.c_str();
        const char* en = ename.c_str();
        const char* ern = ername.c_str();

        const MachineRep* erep = elementType()->machineRep();
        NodeFunc head;

        if (erep == FloatRep::rep())
        {
            head = head_float;
        }
        else if (erep == IntRep::rep())
        {
            head = head_int;
        }
        else if (erep == Int64Rep::rep())
        {
            head = head_int64;
        }
        else if (erep == PointerRep::rep())
        {
            head = head_Pointer;
        }
        else if (erep == BoolRep::rep())
        {
            head = head_bool;
        }
        else if (erep == Vector3FloatRep::rep())
        {
            head = head_Vector3f;
        }
        else if (erep == Vector2FloatRep::rep())
        {
            head = head_Vector2f;
        }
        else if (erep == Vector4FloatRep::rep())
        {
            head = head_Vector4f;
        }
        else if (erep == CharRep::rep())
        {
            head = head_char;
        }
        else if (erep == ShortRep::rep())
        {
            head = head_short;
        }
        else
        {
            abort();
        }

        //
        //  Dereference
        //

        Symbol* s = scope();
        Context* c = context();

        s->addSymbols(
            new ReferenceType(c, rn, this),

            new Function(c, tn, BaseFunctions::dereference, Cast, Return, tn,
                         Args, tn, End),

            new Function(c, tn, ListType::construct_aggregate, Mapped, Args, en,
                         Optional, "?+", Maximum, 999999, Return, tn, End),

#if 0
                  new Function(c, tn, ListType::copyconstruct, None,
                               Return, tn,
                               Args, tn, End),
#endif

            new Function(c, "=", BaseFunctions::assign,
                         Function::MemberOperator | Function::Operator, Return,
                         rn, Args, rn, tn, End),

            new Function(c, "eq", BaseFunctions::eq, CommOp, Return, "bool",
                         Args, tn, tn, End),

#if 0
                  new Function(c, "==", ListType::equals, Mapped,
                               Return, "bool", 
                               Args, tn, tn, End),
                  
                  new Function(c, "print", ListType::print, None,
                               Return, "void",
                               Args, tn, End),
#endif

            EndArguments);

        globalScope()->addSymbols(
            new Function(c, "cons", ListType::cons, Mapped, Return, tn, Args,
                         en, tn, End),

            new Function(c, "tail", ListType::tail, Mapped, Return, tn, Args,
                         tn, End),

            new Function(c, "head", head, Mapped, Return, en, Args, tn, End),

            EndArguments);

        addSymbols(new MemberVariable(c, "value", en),
                   new MemberVariable(c, "next", tn), EndArguments);

        freeze();
    }

    void ListType::freeze()
    {
        Class::freeze();
        _valueOffset = memberVariables()[0]->instanceOffset();
        _nextOffset = memberVariables()[1]->instanceOffset();
        _isGCAtomic = false;
    }

    NODE_IMPLEMENTATION(ListType::construct_aggregate, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* c = static_cast<const ListType*>(NODE_THIS.type());
        const Node* n = 0;

        List list(static_cast<const ListType*>(NODE_THIS.type()), NODE_THREAD,
                  NODE_THIS.argNode(0));

        for (size_t i = 1; n = NODE_THIS.argNode(i); i++)
        {
            list.append(NODE_THREAD, NODE_THIS.argNode(i));
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(ListType::cons, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* c = static_cast<const ListType*>(NODE_THIS.type());
        const Type* etype = c->elementType();
        ClassInstance* head = ClassInstance::allocate(c);
        etype->nodeEval(head->structure() + c->valueOffset(),
                        NODE_THIS.argNode(0), NODE_THREAD);
        ClassInstance** next =
            (ClassInstance**)(head->structure() + c->nextOffset());
        *next = NODE_ARG_OBJECT(1, ClassInstance);
        NODE_RETURN(head);
    }

    NODE_IMPLEMENTATION(ListType::tail, Pointer)
    {
        ClassInstance* head = NODE_ARG_OBJECT(0, ClassInstance);
        if (!head)
            throw NilArgumentException(NODE_THREAD);
        const ListType* c = static_cast<const ListType*>(NODE_THIS.type());
        ClassInstance** next =
            (ClassInstance**)(head->structure() + c->nextOffset());
        NODE_RETURN(*next);
    }

#define NODE_HEAD(TYPE)                                                    \
    NODE_IMPLEMENTATION(ListType::head_##TYPE, TYPE)                       \
    {                                                                      \
        ClassInstance* head = NODE_ARG_OBJECT(0, ClassInstance);           \
        if (!head)                                                         \
            throw NilArgumentException(NODE_THREAD);                       \
        const ListType* c = static_cast<const ListType*>(head->type());    \
        TYPE* p =                                                          \
            reinterpret_cast<TYPE*>(head->structure() + c->valueOffset()); \
        NODE_RETURN(*p);                                                   \
    }

    NODE_HEAD(int)
    NODE_HEAD(int64)
    NODE_HEAD(float)
    NODE_HEAD(bool)
    NODE_HEAD(char)
    NODE_HEAD(short)
    NODE_HEAD(Pointer)
    NODE_HEAD(Vector4f)
    NODE_HEAD(Vector3f)
    NODE_HEAD(Vector2f)

} // namespace Mu
