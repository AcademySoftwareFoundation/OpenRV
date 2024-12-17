//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/DynamicArrayType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/ExceptionType.h>
#include <Mu/Archive.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <iostream>
#include <memory.h>
#include <string>
#include <stdio.h>

namespace Mu
{
    using namespace std;

    //----------------------------------------------------------------------

    DynamicArrayType::DynamicArrayType(Context* context, const char* name,
                                       Class* super, const Type* elementType,
                                       int dimensions)
        : Class(context, name, super)
        , _elementType(elementType)
    {
        _dimensions = dimensions;
        _isAggregate = true;
        _isCollection = true;
        _isSequence = true;
        _isFixedSize = false;
        _isGCAtomic = false;
    }

    DynamicArrayType::~DynamicArrayType() {}

    void DynamicArrayType::freeze()
    {
        Class::freeze();
        _isGCAtomic = false;
    }

    Type::MatchResult DynamicArrayType::match(const Type* t, Bindings& b) const
    {
        if (const DynamicArrayType* atype =
                dynamic_cast<const DynamicArrayType*>(t))
        {
            return elementType()->match(atype->elementType(), b);
        }
        else
        {
            return Class::match(t, b);
        }
    }

    Object* DynamicArrayType::newObject() const
    {
        return new DynamicArray(this, _dimensions);
    }

    size_t DynamicArrayType::objectSize() const { return sizeof(DynamicArray); }

    void DynamicArrayType::constructInstance(Pointer obj) const
    {
        new (obj) DynamicArray(this, 1);
    }

    void DynamicArrayType::copyInstance(Pointer a, Pointer b) const
    {
        DynamicArray* src = reinterpret_cast<DynamicArray*>(a);
        DynamicArray* dst = reinterpret_cast<DynamicArray*>(b);
        dst->resize(src->dimensions());
        memcpy(dst->data<byte>(), src->data<byte>(),
               src->size() * src->elementType()->machineRep()->size());
    }

    void DynamicArrayType::deleteObject(Object* obj) const
    {
        delete static_cast<DynamicArray*>(obj);
    }

    Value DynamicArrayType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._PointerFunc)(*n, thread));
    }

    void DynamicArrayType::outputValueRecursive(ostream& o,
                                                const ValuePointer vp,
                                                ValueOutputState& state) const
    {
        const Type* etype = elementType();
        const DynamicArray* a = *reinterpret_cast<const DynamicArray**>(vp);

        if (a)
        {
            o << fullyQualifiedName() << " {";

            if (state.traversedObjects.find(a) != state.traversedObjects.end())
            {
                o << "...ad infinitum...";
            }
            else
            {
                state.traversedObjects.insert(a);

                for (int i = 0, s = a->size(); i < s; i++)
                {
                    if (i)
                        o << ", ";
                    etype->outputValueRecursive(
                        o, ValuePointer(a->elementPointer(i)), state);

                    if (!state.fullOutput && i > 80 && s > 81)
                    {
                        o << ", ...truncated...";
                        break;
                    }
                }

                state.traversedObjects.erase(a);
            }

            o << "}";
        }
        else
        {
            o << "nil";
        }
    }

    const Type* DynamicArrayType::fieldType(size_t) const
    {
        return _elementType;
    }

    ValuePointer DynamicArrayType::fieldPointer(Object* o, size_t index) const
    {
        DynamicArray* array = reinterpret_cast<DynamicArray*>(o);
        if (index < array->size())
            return array->elementPointer(index);
        return 0;
    }

    const ValuePointer DynamicArrayType::fieldPointer(const Object* o,
                                                      size_t index) const
    {
        const DynamicArray* array = reinterpret_cast<const DynamicArray*>(o);
        if (index < array->size())
            return (const ValuePointer)array->elementPointer(index);
        return 0;
    }

    void DynamicArrayType::serialize(std::ostream& o, Archive::Writer& archive,
                                     const ValuePointer p) const
    {
        const DynamicArray* array = *reinterpret_cast<const DynamicArray**>(p);
        size_t s = array->size();
        o.write((const char*)&s, sizeof(size_t));
        Class::serialize(o, archive, p);
    }

    void DynamicArrayType::deserialize(std::istream& in,
                                       Archive::Reader& archive,
                                       ValuePointer p) const
    {
        DynamicArray* array = *reinterpret_cast<DynamicArray**>(p);
        size_t s;
        in.read((char*)&s, sizeof(size_t));
        array->resize(s);
        Class::deserialize(in, archive, p);
    }

    void DynamicArrayType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        Name voidName = c->lookupName("void");
        const Type* voidType = c->voidType();

        String tname = name();
        String ftname = fullyQualifiedName();
        String rname = tname + "&";
        String frname = ftname + "&";
        String ename = elementType()->fullyQualifiedName();

        const char* tn = tname.c_str();
        const char* ftn = ftname.c_str();
        const char* rn = rname.c_str();
        const char* frn = frname.c_str();
        const char* et = elementType()->fullyQualifiedName().c_str();
        const char* fet = et;
        const char* re =
            elementType()->referenceType()->fullyQualifiedName().c_str();

        //
        //	If a new MachineRep is added, this section of code must be
        // updated.
        //

        const MachineRep* erep = elementType()->machineRep();
        NodeFunc pushBackFunc;
        NodeFunc popBackFunc;
        NodeFunc eraseFunc;

        if (erep == FloatRep::rep())
        {
            pushBackFunc = push_back_float;
            popBackFunc = pop_back_float;
            eraseFunc = erase_float;
        }
        else if (erep == DoubleRep::rep())
        {
            pushBackFunc = push_back_double;
            popBackFunc = pop_back_double;
            eraseFunc = erase_double;
        }
        else if (erep == IntRep::rep())
        {
            pushBackFunc = push_back_int;
            popBackFunc = pop_back_int;
            eraseFunc = erase_int;
        }
        else if (erep == Int64Rep::rep())
        {
            pushBackFunc = push_back_int64;
            popBackFunc = pop_back_int64;
            eraseFunc = erase_int64;
        }
        else if (erep == PointerRep::rep())
        {
            pushBackFunc = push_back_Pointer;
            popBackFunc = pop_back_Pointer;
            eraseFunc = erase_Pointer;
        }
        else if (erep == BoolRep::rep())
        {
            pushBackFunc = push_back_bool;
            popBackFunc = pop_back_bool;
            eraseFunc = erase_bool;
        }
        else if (erep == Vector3FloatRep::rep())
        {
            pushBackFunc = push_back_Vector3f;
            popBackFunc = pop_back_Vector3f;
            eraseFunc = erase_Vector3f;
        }
        else if (erep == Vector2FloatRep::rep())
        {
            pushBackFunc = push_back_Vector2f;
            popBackFunc = pop_back_Vector2f;
            eraseFunc = erase_Vector2f;
        }
        else if (erep == Vector4FloatRep::rep())
        {
            pushBackFunc = push_back_Vector4f;
            popBackFunc = pop_back_Vector4f;
            eraseFunc = erase_Vector4f;
        }
        else if (erep == CharRep::rep())
        {
            pushBackFunc = push_back_char;
            popBackFunc = pop_back_char;
            eraseFunc = erase_char;
        }
        else if (erep == ShortRep::rep())
        {
            pushBackFunc = push_back_short;
            popBackFunc = pop_back_short;
            eraseFunc = erase_short;
        }
        else
        {
            abort();
        }

        s->addSymbols(new ReferenceType(c, rn, this),

                      new Function(c, tn, DynamicArrayType::dyn_construct, None,
                                   Return, ftn, End),

                      new Function(c, tn,
                                   DynamicArrayType::dyn_construct_aggregate,
                                   Mapped, Args, et, Optional, "?+", Maximum,
                                   999999, Return, ftn, End),

                      new Function(c, tn, DynamicArrayType::dyn_copyconstruct,
                                   None, Return, ftn, Args, ftn, End),

                      new Function(c, tn, BaseFunctions::dereference, Cast,
                                   Return, ftn, Args, frn, End),
                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, frn, Args,
                         frn, ftn, End),

            new Function(c, "eq", BaseFunctions::eq, CommOp, Return, "bool",
                         Args, ftn, ftn, End),

            new Function(c, "==", DynamicArrayType::dyn_equals, Mapped, Return,
                         "bool", Args, ftn, ftn, End),

            new Function(c, "print", DynamicArrayType::dyn_print, None, Return,
                         "void", Args, ftn, End),

            EndArguments);

        addSymbols(new Function(c, "size", DynamicArrayType::dyn_size, Mapped,
                                Return, "int", Args, ftn, End),

                   new Function(c, "empty", DynamicArrayType::dyn_empty, Mapped,
                                Return, "bool", Args, ftn, End),

                   EndArguments);

        if (_dimensions > 1)
        {
            STLVector<ParameterVariable*>::Type parameters;
            Type* intType =
                globalScope()->findSymbolOfType<Type>(c->lookupName("int"));

            parameters.push_back(new ParameterVariable(c, "this", this));
            for (int i = 0; i < _dimensions; i++)
            {
                char temp[80];
                sprintf(temp, "index%d", i);
                parameters.push_back(new ParameterVariable(c, temp, intType));
            }

            addSymbol(new Function(
                c, "[]", (Type*)elementType()->referenceType(),
                parameters.size(), &parameters.front(),
                DynamicArrayType::dyn_indexN,
                Mapped & Function::Native & Function::MemberOperator));

            addSymbol(new Function(
                c, "resize", voidType, parameters.size(), &parameters.front(),
                DynamicArrayType::dyn_resizeN, Function::Native));
        }

        if (_dimensions == 1)
        {
            //
            //  Add only if its one dimensional
            //

            addSymbols(new Function(c, "front", DynamicArrayType::front, None,
                                    Return, re, Args, ftn, End),

                       new Function(c, "back", DynamicArrayType::back, None,
                                    Return, re, Args, ftn, End),

                       new Function(c, "[]", DynamicArrayType::dyn_index1,
                                    Mapped & Function::MemberOperator, Return,
                                    re, Args, ftn, "int", End),

                       new Function(c, "resize", DynamicArrayType::dyn_resize1,
                                    None, Return, "void", Args, ftn, "int",
                                    End),

                       EndArguments);
        }

        addSymbols(new Function(c, "clear", DynamicArrayType::clear, None,
                                Return, "void", Args, ftn, End),

                   new Function(c, "rest", DynamicArrayType::dyn_rest, Mapped,
                                Return, ftn, Args, ftn, End),

                   new Function(c, "push_back", pushBackFunc, Retaining, Return,
                                fet, Args, ftn, fet, End),

                   new Function(c, "pop_back", popBackFunc, None, Return, fet,
                                Args, ftn, End),

                   new Function(c, "erase", eraseFunc, None, Return, "void",
                                Args, ftn, "int", "int", End),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_construct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const DynamicArrayType* c =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* o = new DynamicArray(c, c->_dimensions);
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_construct_aggregate, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const DynamicArrayType* c =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        const Type* etype = c->elementType();
        DynamicArray* o = new DynamicArray(c, c->_dimensions);
        o->resize(NODE_THIS.numArgs());

        size_t size = c->elementRep()->size();
        const Node* n;
        unsigned char* data = o->data<unsigned char>();

        for (size_t i = 0; n = NODE_THIS.argNode(i); i++)
        {
            etype->nodeEval(data, n, NODE_THREAD);
            data += size;
        }

        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_rest, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const DynamicArrayType* c =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());

        DynamicArray* in = NODE_ARG_OBJECT(0, DynamicArray);
        if (!in)
            throw NilArgumentException(NODE_THREAD);
        DynamicArray* o = new DynamicArray(c, c->_dimensions);

        if (in->size())
        {
            o->resize(in->size() - 1);

            if (in->size() != 1)
            {
                size_t esize = in->arrayType()->elementRep()->size();

                memcpy(o->elementPointer(0), in->elementPointer(1),
                       esize * (in->size() - 1));
            }
        }

        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_equals, bool)
    {
        DynamicArray* a = NODE_ARG_OBJECT(0, DynamicArray);
        DynamicArray* b = NODE_ARG_OBJECT(1, DynamicArray);

        if (!a && !b)
            return true;

        if (a && b)
        {
            if (a->size() == b->size())
            {
                return !memcmp(a->elementPointer(0), b->elementPointer(0),
                               a->size()
                                   * a->elementType()->machineRep()->size());
            }
        }

        NODE_RETURN(false);
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_copyconstruct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const DynamicArrayType* c =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());

        DynamicArray* o = reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!o)
            throw NilArgumentException(NODE_THREAD);
        DynamicArray* n = new DynamicArray(c, o->dimensions());

        n->resize(o->dimensions());

        memcpy(n->data<char>(), o->data<char>(),
               o->size() * o->type()->machineRep()->size());

        NODE_RETURN(Pointer(n));
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_print, void)
    {
        DynamicArray* o = reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (o)
            o->type()->outputValue(cout, Value((void*)o));
        else
            cout << "nil";
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_size, int)
    {
        DynamicArray* o = reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!o)
            throw NilArgumentException(NODE_THREAD);
        NODE_RETURN(o->size(0));
    }

    NODE_IMPLEMENTATION(DynamicArrayType::clear, void)
    {
        DynamicArray* o = reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!o)
            throw NilArgumentException(NODE_THREAD);
        o->clear();
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_empty, bool)
    {
        DynamicArray* o = reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!o)
            throw NilArgumentException(NODE_THREAD);
        return o->size() == 0;
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_index1, Pointer)
    {
        DynamicArray* self =
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!self)
            throw NilArgumentException(NODE_THREAD);
        int a = NODE_ARG(1, int);
        size_t s = self->size(0);

        if (a < 0)
            a += s;

        if (a >= s)
        {
            throw OutOfRangeException(NODE_THREAD);
        }

        NODE_RETURN(self->elementPointer(a));
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_indexN, Pointer)
    {
        DynamicArray* self =
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!self)
            throw NilArgumentException(NODE_THREAD);

        size_t nargs = NODE_THIS.numArgs();

        if ((nargs - 1) != self->dimensions().size())
        {
            throw OutOfRangeException(NODE_THREAD);
        }

        int indices[5];
        for (int i = 1; i < nargs; i++)
        {
            size_t dim = self->size(i - 1);
            int a = NODE_ARG(i, int);
            if (a < 0)
                a += dim;

            if (a >= dim)
            {
                throw OutOfRangeException(NODE_THREAD);
            }

            indices[i - 1] = a;
        }

        switch (nargs)
        {
        case 2:
            NODE_RETURN(self->elementPointer(indices[0]));
        case 3:
            NODE_RETURN(self->elementPointer(indices[0], indices[1]));
        case 4:
            NODE_RETURN(
                self->elementPointer(indices[0], indices[1], indices[2]));
        default:
            throw UnimplementedFeatureException(NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_resize1, void)
    {
        DynamicArray* self =
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!self)
            throw NilArgumentException(NODE_THREAD);
        int a = NODE_ARG(1, int);

        if (a < 0)
        {
            throw OutOfRangeException(NODE_THREAD);
        }

        self->resize(a);
    }

    NODE_IMPLEMENTATION(DynamicArrayType::dyn_resizeN, void)
    {
        DynamicArray* self =
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!self)
            throw NilArgumentException(NODE_THREAD);

        size_t nargs = NODE_THIS.numArgs();

        if ((nargs - 1) != self->dimensions().size())
        {
            throw OutOfRangeException(NODE_THREAD);
        }

        DynamicArray::SizeVector sizes;

        for (int i = 1; i < nargs; i++)
        {
            int a = NODE_ARG(i, int);

            if (a < 0)
            {
                throw OutOfRangeException(NODE_THREAD);
            }

            sizes.push_back(a);
        }

        self->resize(sizes);
    }

    NODE_IMPLEMENTATION(DynamicArrayType::front, Pointer)
    {
        DynamicArray* self =
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!self)
            throw NilArgumentException(NODE_THREAD);

        if (self->size(0) == 0)
        {
            throw OutOfRangeException(NODE_THREAD);
        }

        NODE_RETURN(self->elementPointer(0));
    }

    NODE_IMPLEMENTATION(DynamicArrayType::back, Pointer)
    {
        DynamicArray* self =
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer));
        if (!self)
            throw NilArgumentException(NODE_THREAD);

        size_t s = self->size(0);

        if (s == 0)
        {
            throw OutOfRangeException(NODE_THREAD);
        }

        NODE_RETURN(self->elementPointer(s - 1));
    }

#define NODE_PUSH_BACK(TYPE)                                       \
    NODE_IMPLEMENTATION(DynamicArrayType::push_back_##TYPE, TYPE)  \
    {                                                              \
        DynamicArray* self =                                       \
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer)); \
        if (!self)                                                 \
            throw NilArgumentException(NODE_THREAD);               \
        TYPE value = NODE_ARG(1, TYPE);                            \
        size_t s = self->size();                                   \
        self->resize(s + 1);                                       \
        self->element<TYPE>(s) = value;                            \
        NODE_RETURN(value);                                        \
    }

    NODE_PUSH_BACK(int)
    NODE_PUSH_BACK(int64)
    NODE_PUSH_BACK(float)
    NODE_PUSH_BACK(double)
    NODE_PUSH_BACK(bool)
    NODE_PUSH_BACK(char)
    NODE_PUSH_BACK(short)
    NODE_PUSH_BACK(Pointer)
    NODE_PUSH_BACK(Vector4f)
    NODE_PUSH_BACK(Vector3f)
    NODE_PUSH_BACK(Vector2f)

#define NODE_POP_BACK(TYPE)                                        \
    NODE_IMPLEMENTATION(DynamicArrayType::pop_back_##TYPE, TYPE)   \
    {                                                              \
        DynamicArray* self =                                       \
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer)); \
        if (!self)                                                 \
            throw NilArgumentException(NODE_THREAD);               \
        size_t s = self->size();                                   \
                                                                   \
        if (s == 0)                                                \
        {                                                          \
            throw OutOfRangeException(NODE_THREAD);                \
        }                                                          \
                                                                   \
        TYPE value = self->element<TYPE>(s - 1);                   \
        self->resize(s - 1);                                       \
        NODE_RETURN(value);                                        \
    }

    NODE_POP_BACK(int)
    NODE_POP_BACK(int64)
    NODE_POP_BACK(float)
    NODE_POP_BACK(double)
    NODE_POP_BACK(bool)
    NODE_POP_BACK(char)
    NODE_POP_BACK(short)
    NODE_POP_BACK(Pointer)
    NODE_POP_BACK(Vector4f)
    NODE_POP_BACK(Vector3f)
    NODE_POP_BACK(Vector2f)

#define NODE_ERASE(TYPE)                                           \
    NODE_IMPLEMENTATION(DynamicArrayType::erase_##TYPE, void)      \
    {                                                              \
        DynamicArray* self =                                       \
            reinterpret_cast<DynamicArray*>(NODE_ARG(0, Pointer)); \
        if (!self)                                                 \
            throw NilArgumentException(NODE_THREAD);               \
        int index = NODE_ARG(1, int);                              \
        int n = NODE_ARG(2, int);                                  \
                                                                   \
        size_t s = self->size();                                   \
                                                                   \
        if (s == 0)                                                \
        {                                                          \
            throw OutOfRangeException(NODE_THREAD);                \
        }                                                          \
                                                                   \
        self->erase(index, n);                                     \
    }

    NODE_ERASE(int)
    NODE_ERASE(int64)
    NODE_ERASE(float)
    NODE_ERASE(double)
    NODE_ERASE(bool)
    NODE_ERASE(char)
    NODE_ERASE(short)
    NODE_ERASE(Pointer)
    NODE_ERASE(Vector4f)
    NODE_ERASE(Vector3f)
    NODE_ERASE(Vector2f)

} // namespace Mu
