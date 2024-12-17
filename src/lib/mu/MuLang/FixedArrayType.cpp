//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/FixedArrayType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/FixedArray.h>
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

    FixedArrayType::FixedArrayType(Context* c, const char* name, Class* super,
                                   const Type* elementType,
                                   const SizeVector& dimensions)
        : Class(c, name, super)
        , _elementType(elementType)
    {
        _dimensions = dimensions;
        _fixedSize = 1;
        _isAggregate = true;
        _isCollection = true;
        _isSequence = true;

        for (int i = 0; i < dimensions.size(); i++)
        {
            size_t d = dimensions[i];
            _fixedSize *= dimensions[i];
        }
    }

    FixedArrayType::FixedArrayType(Context* c, const char* name, Class* super,
                                   const Type* elementType,
                                   const size_t* dimensions, size_t nDimensions)
        : Class(c, name, super)
        , _elementType(elementType)
        , _dimensions(nDimensions)
    {
        std::copy(dimensions, dimensions + nDimensions, _dimensions.begin());

        _fixedSize = 1;
        _isAggregate = true;
        _isCollection = true;
        _isSequence = true;

        for (int i = 0; i < nDimensions; i++)
        {
            size_t d = dimensions[i];
            _fixedSize *= dimensions[i];
        }
    }

    FixedArrayType::~FixedArrayType() {}

    Type::MatchResult FixedArrayType::match(const Type* t, Bindings& b) const
    {
        // if (const FixedArrayType* atype = dynamic_cast<const
        // FixedArrayType*>(t))
        // {
        //     if (atype == this) return Match;

        //     if (elementType()->match(atype->elementType(), b) == Match)
        //     {
        //         if (atype->dimensions().size() == dimensions.size())
        //         {
        //             for (size_t i = 0; i < dimensions.size(); i++)
        //             {
        //                 if (dimensions()[i] != atype->dimensions()[i])
        //                 {
        //                     return NoMatch;
        //                 }
        //             }

        //             return Match;
        //         }
        //     }
        // }
        // else
        // {
        return Class::match(t, b);
        //}
    }

    void FixedArrayType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                              ValueOutputState& state) const
    {
        const Type* etype = elementType();
        const FixedArray* a = *reinterpret_cast<FixedArray**>(vp);

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

    const Type* FixedArrayType::fieldType(size_t) const { return _elementType; }

    ValuePointer FixedArrayType::fieldPointer(Object* o, size_t index) const
    {
        FixedArray* array = reinterpret_cast<FixedArray*>(o);
        if (index < array->size())
            return array->elementPointer(index);
        return 0;
    }

    const ValuePointer FixedArrayType::fieldPointer(const Object* o,
                                                    size_t index) const
    {
        const FixedArray* array = reinterpret_cast<const FixedArray*>(o);
        if (index < array->size())
            return (const ValuePointer)array->elementPointer(index);
        return 0;
    }

    void FixedArrayType::freeze()
    {
        Class::freeze();
        _instanceSize = _fixedSize * elementType()->machineRep()->size();
        _isGCAtomic = false;
    }

    void FixedArrayType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        Name voidName = c->lookupName("void");
        const Type* voidType = globalScope()->findSymbolOfType<Type>(voidName);

        String tname = name();
        String ftname = fullyQualifiedName();
        String rname = tname + "&";
        String frname = ftname + "&";
        String ename = elementType()->fullyQualifiedName();

        const char* tn = tname.c_str();
        const char* ftn = ftname.c_str();
        const char* rn = rname.c_str();
        const char* frn = rname.c_str();
        const char* et = elementType()->fullyQualifiedName().c_str();
        const char* re =
            elementType()->referenceType()->fullyQualifiedName().c_str();

        //
        //	If a new MachineRep is added, this section of code must be
        // updated.
        //

        const MachineRep* erep = elementType()->machineRep();

        s->addSymbols(new ReferenceType(c, rn, this),

                      new Function(c, tn, FixedArrayType::fixed_construct, None,
                                   Return, ftn, End),

                      new Function(c, tn,
                                   FixedArrayType::fixed_construct_aggregate,
                                   Mapped, Args, et, Optional, "?+", Maximum,
                                   fixedSize(), Return, ftn, End),

                      new Function(c, tn, FixedArrayType::fixed_copyconstruct,
                                   None, Return, ftn, Args, ftn, End),

                      new Function(c, tn, BaseFunctions::dereference, Cast,
                                   Return, ftn, Args, frn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "==", FixedArrayType::fixed_equals, Mapped, Return,
                         "bool", Args, ftn, ftn, End),

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, frn, Args,
                         frn, ftn, End),

            new Function(c, "eq", BaseFunctions::eq, CommOp, Return, "bool",
                         Args, ftn, ftn, End),

            new Function(c, "print", FixedArrayType::fixed_print, None, Return,
                         "void", Args, ftn, End),

            EndArguments);

        addSymbols(new Function(c, "size", FixedArrayType::fixed_size, Mapped,
                                Return, "int", Args, ftn, End),

                   EndArguments);

        if (_dimensions.size() > 1)
        {
            STLVector<ParameterVariable*>::Type parameters;
            Type* intType =
                globalScope()->findSymbolOfType<Type>(c->lookupName("int"));

            parameters.push_back(new ParameterVariable(c, "this", this));
            for (int i = 0; i < _dimensions.size(); i++)
            {
                char temp[80];
                sprintf(temp, "index%d", i);
                parameters.push_back(new ParameterVariable(c, temp, intType));
            }

            addSymbol(new Function(
                c, "[]", (Type*)elementType()->referenceType(),
                parameters.size(), &parameters.front(),
                FixedArrayType::fixed_indexN,
                Mapped & Function::Native & Function::MemberOperator));
        }

        if (_dimensions.size() == 1)
        {
            //
            //  Add only if its one dimensional
            //

            addSymbol(new Function(c, "[]", FixedArrayType::fixed_index1,
                                   Mapped & Function::MemberOperator, Return,
                                   re, Args, ftn, "int", End));
        }
    }

    NODE_IMPLEMENTATION(FixedArrayType::fixed_construct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const FixedArrayType* c =
            static_cast<const FixedArrayType*>(NODE_THIS.type());
        ClassInstance* o = ClassInstance::allocate(c);
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(FixedArrayType::fixed_construct_aggregate, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const FixedArrayType* c =
            static_cast<const FixedArrayType*>(NODE_THIS.type());
        const Type* etype = c->elementType();
        FixedArray* o = static_cast<FixedArray*>(ClassInstance::allocate(c));

        const Node* n;
        size_t size = c->elementRep()->size();
        unsigned char* data = o->data<unsigned char>();

        for (size_t i = 0; n = NODE_THIS.argNode(i); i++)
        {
            etype->nodeEval(data, n, NODE_THREAD);
            data += size;
        }

        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(FixedArrayType::fixed_copyconstruct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const FixedArrayType* c =
            static_cast<const FixedArrayType*>(NODE_THIS.type());

        FixedArray* o = reinterpret_cast<FixedArray*>(NODE_ARG(0, Pointer));
        if (!o)
            throw NilArgumentException(NODE_THREAD);
        FixedArray* n = static_cast<FixedArray*>(ClassInstance::allocate(c));
        memcpy(n->data<unsigned char>(), o->data<unsigned char>(),
               c->elementRep()->size() * c->fixedSize());

        NODE_RETURN(Pointer(n));
    }

    NODE_IMPLEMENTATION(FixedArrayType::fixed_equals, bool)
    {
        FixedArray* a = NODE_ARG_OBJECT(0, FixedArray);
        FixedArray* b = NODE_ARG_OBJECT(1, FixedArray);

        if (!a && !b)
            return true;

        if (a && b && a->size() == b->size())
        {
            return !memcmp(
                a->elementPointer(0), b->elementPointer(0),
                a->size()
                    * a->arrayType()->elementType()->machineRep()->size());
        }

        NODE_RETURN(false);
    }

    NODE_IMPLEMENTATION(FixedArrayType::fixed_print, void)
    {
        FixedArray* o = reinterpret_cast<FixedArray*>(NODE_ARG(0, Pointer));
        if (o)
            o->type()->outputValue(cout, Value((void*)o));
        else
            cout << "nil";
    }

    NODE_IMPLEMENTATION(FixedArrayType::fixed_size, int)
    {
        FixedArray* o = reinterpret_cast<FixedArray*>(NODE_ARG(0, Pointer));
        if (!o)
            throw NilArgumentException(NODE_THREAD);
        NODE_RETURN(o->size(0));
    }

    NODE_IMPLEMENTATION(FixedArrayType::fixed_index1, Pointer)
    {
        FixedArray* self = reinterpret_cast<FixedArray*>(NODE_ARG(0, Pointer));
        if (!self)
            throw NilArgumentException(NODE_THREAD);
        const FixedArrayType* type = self->arrayType();
        int a = NODE_ARG(1, int);
        size_t s = type->_fixedSize;

        if (a < 0)
            a += s;

        if (a >= s)
        {
            throw OutOfRangeException(NODE_THREAD);
        }

        NODE_RETURN(self->elementPointer(a));
    }

    NODE_IMPLEMENTATION(FixedArrayType::fixed_indexN, Pointer)
    {
        FixedArray* self = reinterpret_cast<FixedArray*>(NODE_ARG(0, Pointer));
        if (!self)
            throw NilArgumentException(NODE_THREAD);
        const FixedArrayType* type = self->arrayType();

        size_t nargs = NODE_THIS.numArgs();

        if ((nargs - 1) != type->dimensions().size())
        {
            throw OutOfRangeException(NODE_THREAD);
        }

        int indices[5];
        for (int i = 1; i < nargs; i++)
        {
            size_t dim = type->dimensions()[i - 1];

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

} // namespace Mu
