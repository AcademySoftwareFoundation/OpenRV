//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/BoolType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/FixedArray.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/MuLangContext.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/List.h>
#include <Mu/ListType.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/NodePrinter.h>
#include <Mu/Object.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Value.h>
#include <Mu/config.h>
#include <iostream>
#include <sstream>

namespace Mu
{
    using namespace std;
    void throw_assertion_failure(Mu::Thread&, const char*);

    BoolType::BoolType(Context* c)
        : PrimitiveType(c, "bool", BoolRep::rep())
    {
    }

    BoolType::~BoolType() {}

    PrimitiveObject* BoolType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value BoolType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._boolFunc)(*n, thread));
    }

    void BoolType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        bool* bp = reinterpret_cast<bool*>(p);
        *bp = (*n->func()._boolFunc)(*n, thread);
    }

    void BoolType::outputValue(ostream& o, const Value& value, bool full) const
    {
        o << (value._bool ? "true" : "false");
    }

    void BoolType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                        ValueOutputState& state) const
    {
        const bool* p = reinterpret_cast<const bool*>(vp);
        o << (*p ? "true" : "false");
    }

#define T Thread&

    bool __C_bool_bool(T) { return false; }

    bool __C_bool_boolAmp_(T, bool& a) { return a; }

    bool __C_Bang__bool_bool(T, bool a) { return !a; }

    bool __C_Amp_Amp__bool_bool_bool(T, bool a, bool b) { return a && b; }

    bool __C_Pipe_Pipe__bool_bool_bool(T, bool a, bool b) { return a || b; }

    bool& __C_EQ__boolAmp__boolAmp__bool(T, bool& a, bool b) { return a = b; }

    bool __C_QMark_Colon__bool_bool_bool(T, bool p, bool a, bool b)
    {
        return p ? a : b;
    }

    bool __C_eq_QMark_non_primitive_or_nil_QMark_non_primitive_or_nil(T,
                                                                      Pointer a,
                                                                      Pointer b)
    {
        return a == b;
    }

    bool
    __C_neq_QMark_non_primitive_or_nil_QMark_non_primitive_or_nil(T, Pointer a,
                                                                  Pointer b)
    {
        return a != b;
    }

    void __C_assert_void_bool(T t, bool a)
    {
        if (!a)
            throw_assertion_failure(t, "");
    }

#undef T

    void BoolType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        s->addSymbols(
            new ReferenceType(c, "bool&", this),

            new Function(c, "bool", BoolType::defaultBool,
                         Mapped | NativeInlined, Compiled, __C_bool_bool,
                         Return, "bool", End),

            new Function(c, "bool", BoolType::dereference, Cast | NativeInlined,
                         Compiled, __C_bool_boolAmp_, Return, "bool", Args,
                         "bool&", End),

            new Function(c, "=", BoolType::assign, AsOp | NativeInlined,
                         Compiled, __C_EQ__boolAmp__boolAmp__bool, Return,
                         "bool&", Args, "bool&", "bool", End),

            new Function(c, "!", BoolType::logicalNot, Op | NativeInlined,
                         Compiled, __C_Bang__bool_bool, Return, "bool", Args,
                         "bool", End),

            new Function(c, "&&", BoolType::logicalAnd, Op | NativeInlined,
                         Compiled, __C_Amp_Amp__bool_bool_bool, Return, "bool",
                         Args, "bool", "bool", End),

            new Function(c, "||", BoolType::logicalOr, Op | NativeInlined,
                         Compiled, __C_Pipe_Pipe__bool_bool_bool, Return,
                         "bool", Args, "bool", "bool", End),

            new Function(c, "?:", BoolType::conditionalExpr, Op, Compiled,
                         __C_QMark_Colon__bool_bool_bool, Return, "bool", Args,
                         "bool", "bool", "bool", End),

            new Function(c, "__if", BoolType::__if, None, Return, "void", Args,
                         "bool", "?", End),

            new Function(c, "__if", BoolType::__if_else, None, Return, "void",
                         Args, "bool", "?", "?", End),

            new Function(c, "__for", BoolType::__for, None, Return, "void",
                         Args, "?", "bool", "?", "?", End),

            new Function(c, "__repeat", BoolType::__repeat, None, Return,
                         "void", Args, "int", "?", End),

            new Function(c, "__for_each", BoolType::__for_each_fixed_array,
                         None, Return, "void", Args, "?reference",
                         "?fixed_array", "?", End),

            new Function(c, "__for_each", BoolType::__for_each_dynamic_array,
                         None, Return, "void", Args, "?reference", "?dyn_array",
                         "?", End),

            new Function(c, "__for_each", BoolType::__for_each_list, None,
                         Return, "void", Args, "?reference", "?list", "?", End),

            new Function(c, "__for_index", BoolType::__for_index_dynamic_array,
                         None, Return, "void", Args, "?reference", "?dyn_array",
                         "?", End),

            new Function(c, "__for_index", BoolType::__for_index_fixed1_array,
                         None, Return, "void", Args, "?reference",
                         "?fixed_array", "?", End),

            new Function(c, "__for_index", BoolType::__for_index_fixed2_array,
                         None, Return, "void", Args, "?reference", "?reference",
                         "?fixed_array", "?", End),

            new Function(c, "__for_index", BoolType::__for_index_fixed3_array,
                         None, Return, "void", Args, "?reference", "?reference",
                         "?reference", "?fixed_array", "?", End),

            new Function(c, "__while", BoolType::__while, None, Return, "void",
                         Args, "bool", "?", End),

            new Function(c, "__do_while", BoolType::__do_while, None, Return,
                         "void", Args, "?", "bool", End),

            new Function(c, "__break", BoolType::__break, None, Return, "void",
                         End),

            new Function(c, "__continue", BoolType::__continue, None, Return,
                         "void", End),

            new Function(c, "assert", BoolType::assertion, NativeInlined,
                         Compiled, __C_assert_void_bool, Return, "void", Args,
                         "bool", End),

            new Function(
                c, "eq", BaseFunctions::eq, CommOp | NativeInlined, Compiled,
                __C_eq_QMark_non_primitive_or_nil_QMark_non_primitive_or_nil,
                Return, "bool", Args, "?non_primitive_or_nil",
                "?non_primitive_or_nil", End),

            new Function(
                c, "neq", BaseFunctions::neq, CommOp | NativeInlined, Compiled,
                __C_neq_QMark_non_primitive_or_nil_QMark_non_primitive_or_nil,
                Return, "bool", Args, "?non_primitive_or_nil",
                "?non_primitive_or_nil", End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(BoolType::dereference, bool)
    {
        bool* bp = reinterpret_cast<bool*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*bp);
    }

    NODE_IMPLEMENTATION(BoolType::defaultBool, bool) { NODE_RETURN(false); }

    NODE_IMPLEMENTATION(BoolType::logicalNot, bool)
    {
        NODE_RETURN(!NODE_ARG(0, bool));
    }

    NODE_IMPLEMENTATION(BoolType::logicalOr, bool)
    {
        NODE_RETURN(NODE_ARG(0, bool) || NODE_ARG(1, bool));
    }

    NODE_IMPLEMENTATION(BoolType::logicalAnd, bool)
    {
        NODE_RETURN(NODE_ARG(0, bool) && NODE_ARG(1, bool));
    }

    NODE_IMPLEMENTATION(BoolType::conditionalExpr, bool)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, bool) : NODE_ARG(2, bool));
    }

#define ASOP(name, op)                                            \
    NODE_IMPLEMENTATION(name, Pointer)                            \
    {                                                             \
        using namespace Mu;                                       \
        bool* bp = reinterpret_cast<bool*>(NODE_ARG(0, Pointer)); \
        *bp op NODE_ARG(1, bool);                                 \
        NODE_RETURN((Pointer)bp);                                 \
    }

    ASOP(BoolType::assign, =)
#undef ASOP

    NODE_IMPLEMENTATION(BoolType::__if, void)
    {
        //
        //	There is some hackery here. Since we don't know the return
        //	type of the conditional part, its assumed to be int. This is a
        //	really bad assumption. The other route is MU_SAFE_ARGUMENTS. It
        // calls the 	type to get the correct type.
        //

        if (NODE_ARG(0, bool))
        {
            NODE_ANY_TYPE_ARG(1);
        }
    }

    NODE_IMPLEMENTATION(BoolType::__if_else, void)
    {
        if (NODE_ARG(0, bool))
        {
            NODE_ANY_TYPE_ARG(1);
        }
        else
        {
            NODE_ANY_TYPE_ARG(2);
        }
    }

    NODE_IMPLEMENTATION(BoolType::__break, void)
    {
        NODE_THREAD.jump(JumpReturnCode::Break);
    }

    NODE_IMPLEMENTATION(BoolType::__continue, void)
    {
        NODE_THREAD.jump(JumpReturnCode::Continue);
    }

    NODE_IMPLEMENTATION(BoolType::__for, void)
    {
        //
        //	See above regarding safety.
        //

        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        for (NODE_ANY_TYPE_ARG(0); NODE_ARG(1, bool); NODE_ANY_TYPE_ARG(2))
        {
            if (sj)
            {
                rv = SETJMP(NODE_THREAD.jumpPoint());
                sj = false;
            }

            if (rv == JumpReturnCode::NoJump)
            {
                NODE_ANY_TYPE_ARG(3);
            }
            else
            {
                NODE_THREAD.jumpPointRestore();
                sj = true;

                if (rv == JumpReturnCode::Continue)
                    continue;
                else
                    break;
            }
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__repeat, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        //
        //  See below for explaination about i++
        //

        for (int i = 0, e = NODE_ARG(0, int); i < e;)
        {
            if (sj)
            {
                rv = SETJMP(NODE_THREAD.jumpPoint());
                sj = false;
            }

            if (rv == JumpReturnCode::NoJump)
            {
                NODE_ANY_TYPE_ARG(1);
            }
            else
            {
                NODE_THREAD.jumpPointRestore();
                sj = true;

                if (rv == JumpReturnCode::Continue)
                {
                    i++;
                    continue;
                }
                else
                    break;
            }

            i++;
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__for_each_fixed_array, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        const Node* n0 = NODE_THIS.argNode(0);
        const Node* n1 = NODE_THIS.argNode(1);

        const ReferenceType* rtype =
            dynamic_cast<const ReferenceType*>(n0->type());
        const Type* etype = dynamic_cast<const Type*>(rtype->dereferenceType());
        const MachineRep* rep = etype->machineRep();

        Pointer var = NODE_ARG(0, Pointer);
        FixedArray* farray = NODE_ARG_OBJECT(1, FixedArray);

        // BAD  :MUST END JUMP POINT!
        // if (!farray) throw NilArgumentException(NODE_THREAD);

        //
        //  The following loops put the index increment at the end of the
        //  loop because of a possible bug in gcc 4. With that compiler
        //  (on PPC) if the increment is the final for(;;) position it
        //  looks as though the compiler assumes nothing like longjmp is
        //  going to happen and migrates the i++ to a position *before*
        //  the call that could result in a longjmp->setjmp. This mucks up
        //  the "continue" jump.
        //

        if (farray)
        {
            for (size_t i = 0, s = farray->size(), n = rep->size(); i < s;)
            {
                if (sj)
                {
                    rv = SETJMP(NODE_THREAD.jumpPoint());
                    sj = false;
                }

                if (rv == JumpReturnCode::NoJump)
                {
                    memcpy(var, farray->elementPointer(i), n);
                    NODE_ANY_TYPE_ARG(2);
                }
                else
                {
                    NODE_THREAD.jumpPointRestore();
                    sj = true;

                    if (rv == JumpReturnCode::Continue)
                    {
                        i++;
                        continue;
                    }
                    else
                        break;
                }

                i++; // make PPC optimizer happy in presence of longjmp?
            }
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__for_each_dynamic_array, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        const Node* n0 = NODE_THIS.argNode(0);
        const Node* n1 = NODE_THIS.argNode(1);

        const ReferenceType* rtype =
            dynamic_cast<const ReferenceType*>(n0->type());
        const Type* etype = dynamic_cast<const Type*>(rtype->dereferenceType());
        const MachineRep* rep = etype->machineRep();

        Pointer var = NODE_ARG(0, Pointer);
        DynamicArray* darray = NODE_ARG_OBJECT(1, DynamicArray);

        // MUST END JUMP POINT!
        // if (!darray) throw NilArgumentException(NODE_THREAD);

        //
        //  The following loops put the index increment at the end of the
        //  loop because of a possible bug in gcc 4. With that compiler
        //  (on PPC) if the increment is the final for(;;) position it
        //  looks as though the compiler assumes nothing like longjmp is
        //  going to happen and migrates the i++ to a position *before*
        //  the call that could result in a longjmp->setjmp. This mucks up
        //  the "continue" jump.
        //

        if (darray)
        {
            for (size_t i = 0, s = darray->size(), n = rep->size(); i < s;)
            {
                if (sj)
                {
                    rv = SETJMP(NODE_THREAD.jumpPoint());
                    sj = false;
                }

                if (rv == JumpReturnCode::NoJump)
                {
                    memcpy(var, darray->elementPointer(i), n);
                    NODE_ANY_TYPE_ARG(2);
                }
                else
                {
                    NODE_THREAD.jumpPointRestore();
                    sj = true;

                    if (rv == JumpReturnCode::Continue)
                    {
                        i++;
                        continue;
                    }
                    else
                        break;
                }

                i++; // make PPC optimizer happy in presence of longjmp?
            }
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__for_each_list, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        const Node* n0 = NODE_THIS.argNode(0);
        const Node* n1 = NODE_THIS.argNode(1);

        const ReferenceType* rtype =
            dynamic_cast<const ReferenceType*>(n0->type());
        const Type* etype = dynamic_cast<const Type*>(rtype->dereferenceType());
        const MachineRep* rep = etype->machineRep();

        Pointer var = NODE_ARG(0, Pointer);
        ClassInstance* obj = NODE_ARG_OBJECT(1, ClassInstance);

        //
        // DONT DO THIS! you need to end the jump point created above!
        // if (!obj) throw NilArgumentException(NODE_THREAD);

        //
        //  The following loops put the index increment at the end of the
        //  loop because of a possible bug in gcc 4. With that compiler
        //  (on PPC) if the increment is the final for(;;) position it
        //  looks as though the compiler assumes nothing like longjmp is
        //  going to happen and migrates the i++ to a position *before*
        //  the call that could result in a longjmp->setjmp. This mucks up
        //  the "continue" jump.
        //

        size_t n = rep->size();

        if (obj)
        {
            for (List list(NODE_THREAD.process(), obj); list.isNotNil();)
            {
                if (sj)
                {
                    rv = SETJMP(NODE_THREAD.jumpPoint());
                    sj = false;
                }

                if (rv == JumpReturnCode::NoJump)
                {
                    memcpy(var, list.valuePointer(), n);
                    NODE_ANY_TYPE_ARG(2);
                }
                else
                {
                    NODE_THREAD.jumpPointRestore();
                    sj = true;

                    if (rv == JumpReturnCode::Continue)
                    {
                        ++list;
                        continue;
                    }
                    else
                        break;
                }

                ++list; // make PPC optimizer happy in presence of longjmp?
            }
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__for_index_fixed1_array, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        int* index0 = NODE_ARG_OBJECT(0, int);
        FixedArray* farray = NODE_ARG_OBJECT(1, FixedArray);

        int size0 = farray ? farray->size(0) : 0;

        // BAD! see above
        // if (!farray) throw NilArgumentException(NODE_THREAD);

        //
        //  The following loops put the index increment at the end of the
        //  loop because of a possible bug in gcc 4. With that compiler
        //  (on PPC) if the increment is the final for(;;) position it
        //  looks as though the compiler assumes nothing like longjmp is
        //  going to happen and migrates the i++ to a position *before*
        //  the call that could result in a longjmp->setjmp. This mucks up
        //  the "continue" jump.
        //

        for (size_t i = 0; i < size0;)
        {
            if (sj)
            {
                rv = SETJMP(NODE_THREAD.jumpPoint());
                sj = false;
            }

            if (rv == JumpReturnCode::NoJump)
            {
                *index0 = i;
                NODE_ANY_TYPE_ARG(3);
            }
            else
            {
                NODE_THREAD.jumpPointRestore();
                sj = true;

                if (rv == JumpReturnCode::Continue)
                {
                    i++;
                    continue;
                }
                else
                    break;
            }

            i++; // make PPC optimizer happy in presence of longjmp?
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__for_index_fixed2_array, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        int* index0 = NODE_ARG_OBJECT(0, int);
        int* index1 = NODE_ARG_OBJECT(1, int);
        FixedArray* farray = NODE_ARG_OBJECT(2, FixedArray);

        int size0;
        int size1;

        if (farray)
        {
            size0 = farray->size(0);
            size1 = farray->size(1);
        }
        else
        {
            size0 = 0;
            size1 = 0;
        }

        // BAD! see above
        // if (!farray) throw NilArgumentException(NODE_THREAD);

        //
        //  The following loops put the index increment at the end of the
        //  loop because of a possible bug in gcc 4. With that compiler
        //  (on PPC) if the increment is the final for(;;) position it
        //  looks as though the compiler assumes nothing like longjmp is
        //  going to happen and migrates the i++ to a position *before*
        //  the call that could result in a longjmp->setjmp. This mucks up
        //  the "continue" jump.
        //

        for (size_t i = 0; i < size0;)
        {
            *index0 = i;

            for (size_t j = 0; j < size1;)
            {
                if (sj)
                {
                    rv = SETJMP(NODE_THREAD.jumpPoint());
                    sj = false;
                }

                if (rv == JumpReturnCode::NoJump)
                {
                    *index1 = j;
                    NODE_ANY_TYPE_ARG(3);
                }
                else
                {
                    NODE_THREAD.jumpPointRestore();
                    sj = true;

                    if (rv == JumpReturnCode::Continue)
                    {
                        j++;
                        continue;
                    }
                    else
                        break;
                }

                j++;
            }

            i++; // make PPC optimizer happy in presence of longjmp?
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__for_index_fixed3_array, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        int* index0 = NODE_ARG_OBJECT(0, int);
        int* index1 = NODE_ARG_OBJECT(1, int);
        int* index2 = NODE_ARG_OBJECT(2, int);
        FixedArray* farray = NODE_ARG_OBJECT(3, FixedArray);

        int size0;
        int size1;
        int size2;

        if (farray)
        {
            size0 = farray->size(0);
            size1 = farray->size(1);
            size2 = farray->size(2);
        }
        else
        {
            size0 = size1 = size2 = 0;
        }

        // BAD! see above
        // if (!farray) throw NilArgumentException(NODE_THREAD);

        //
        //  The following loops put the index increment at the end of the
        //  loop because of a possible bug in gcc 4. With that compiler
        //  (on PPC) if the increment is the final for(;;) position it
        //  looks as though the compiler assumes nothing like longjmp is
        //  going to happen and migrates the i++ to a position *before*
        //  the call that could result in a longjmp->setjmp. This mucks up
        //  the "continue" jump.
        //

        for (size_t i = 0; i < size0;)
        {
            *index0 = i;

            for (size_t j = 0; j < size1;)
            {
                *index1 = j;

                for (size_t k = 0; k < size2;)
                {
                    if (sj)
                    {
                        rv = SETJMP(NODE_THREAD.jumpPoint());
                        sj = false;
                    }

                    if (rv == JumpReturnCode::NoJump)
                    {
                        *index2 = k;
                        NODE_ANY_TYPE_ARG(4);
                    }
                    else
                    {
                        NODE_THREAD.jumpPointRestore();
                        sj = true;

                        if (rv == JumpReturnCode::Continue)
                        {
                            k++;
                            continue;
                        }
                        else
                            break;
                    }
                    k++;
                }

                j++;
            }

            i++; // make PPC optimizer happy in presence of longjmp?
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__for_index_dynamic_array, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        int rv = JumpReturnCode::NoJump;
        bool sj = true;

        int* index = NODE_ARG_OBJECT(0, int);
        DynamicArray* darray = NODE_ARG_OBJECT(1, DynamicArray);

        // BAD! see other for_ jump point!
        // if (!darray) throw NilArgumentException(NODE_THREAD);

        //
        //  The following loops put the index increment at the end of the
        //  loop because of a possible bug in gcc 4. With that compiler
        //  (on PPC) if the increment is the final for(;;) position it
        //  looks as though the compiler assumes nothing like longjmp is
        //  going to happen and migrates the i++ to a position *before*
        //  the call that could result in a longjmp->setjmp. This mucks up
        //  the "continue" jump.
        //

        if (darray)
        {
            for (size_t i = 0, s = darray->size(); i < s;)
            {
                if (sj)
                {
                    rv = SETJMP(NODE_THREAD.jumpPoint());
                    sj = false;
                }

                if (rv == JumpReturnCode::NoJump)
                {
                    *index = i;
                    NODE_ANY_TYPE_ARG(2);
                }
                else
                {
                    NODE_THREAD.jumpPointRestore();
                    sj = true;

                    if (rv == JumpReturnCode::Continue)
                    {
                        i++;
                        continue;
                    }
                    else
                        break;
                }

                i++; // make PPC optimizer happy in presence of longjmp?
            }
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__while, void)
    {
        //
        //	See above regarding safety.
        //

        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        while (NODE_ARG(0, bool))
        {
            int rv = SETJMP(NODE_THREAD.jumpPoint());

            if (rv == JumpReturnCode::NoJump)
            {
                NODE_ANY_TYPE_ARG(1);
            }
            else
            {
                NODE_THREAD.jumpPointRestore();

                if (rv == JumpReturnCode::Continue)
                    continue;
                else
                    break;
            }
        }

        NODE_THREAD.jumpPointEnd();
    }

    NODE_IMPLEMENTATION(BoolType::__do_while, void)
    {
        NODE_THREAD.jumpPointBegin(JumpReturnCode::Continue
                                   | JumpReturnCode::Break);

        //
        //	See above regarding safety.
        //

        do
        {
            int rv = SETJMP(NODE_THREAD.jumpPoint());

            if (rv == JumpReturnCode::NoJump)
            {
                NODE_ANY_TYPE_ARG(0);
            }
            else
            {
                NODE_THREAD.jumpPointRestore();

                if (rv == JumpReturnCode::Continue)
                    continue;
                else
                    break;
            }
        } while NODE_ARG(1, bool);

        NODE_THREAD.jumpPointEnd();
    }

    void throw_assertion_failure(Mu::Thread& NODE_THREAD, const char* expr)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* context = (MuLangContext*)p->context();
        ostringstream str;
        str << "Assertion failed: " << expr;

        ExceptionType::Exception* e =
            new ExceptionType::Exception(context->exceptionType());
        string temp = str.str();
        e->string() += temp.c_str();
        NODE_THREAD.setException(e);
        ProgramException exc(NODE_THREAD);
        exc.message() = temp.c_str();
        throw exc;
    }

    NODE_IMPLEMENTATION(BoolType::assertion, void)
    {
        if (NODE_ARG(0, bool) == false)
        {
            Process* p = NODE_THREAD.process();
            MuLangContext* context = (MuLangContext*)p->context();

            ostringstream str;
            str << "Assertion failed: ";
            NodePrinter printer(const_cast<Node*>(NODE_THIS.argNode(0)), str,
                                NodePrinter::Lispy);
            printer.traverse();

            ExceptionType::Exception* e =
                new ExceptionType::Exception(context->exceptionType());
            string temp = str.str();
            e->string() += temp.c_str();
            NODE_THREAD.setException(e);
            ProgramException exc(NODE_THREAD);
            exc.message() = temp.c_str();
            throw exc;
        }
    }

} // namespace Mu
