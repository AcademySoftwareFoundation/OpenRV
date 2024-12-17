//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/OpaqueType.h>
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Value.h>
#include <Mu/config.h>
#include <iostream>

namespace Mu
{
    using namespace std;

    OpaqueType::OpaqueType(Context* c, const char* name)
        : PrimitiveType(c, name, PointerRep::rep())
    {
    }

    OpaqueType::~OpaqueType() {}

    PrimitiveObject* OpaqueType::newObject() const
    {
        return new PrimitiveObject(this);
    }

    Value OpaqueType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._PointerFunc)(*n, thread));
    }

    void OpaqueType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        Pointer* ip = reinterpret_cast<Pointer*>(p);
        *ip = (*n->func()._PointerFunc)(*n, thread);
    }

    void OpaqueType::outputValue(ostream& o, const Value& value,
                                 bool full) const
    {
        o << "<#" << fullyQualifiedName() << " 0x" << hex << value._Pointer
          << dec << ">";
    }

    void OpaqueType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                          ValueOutputState& state) const
    {
        Pointer p = *reinterpret_cast<Pointer*>(vp);

        o << "<#" << fullyQualifiedName() << " 0x" << hex << p << dec << ">";
    }

    //
    //  These are the non-inlined versions of the functions
    //  in Native.h. They are passed in as the Compiled function
    //

#define T Thread&

    Pointer __C_Pointer_Pointer(T) { return Pointer(0); }

    Pointer __C_Pointer_Pointer_PointerAmp_(T, Pointer& a) { return a; }

    Pointer& __C_EQ__PointerAmp__PointerAmp__Pointer(T, Pointer& a, Pointer b)
    {
        return a = b;
    }

    Pointer __C_QMark_Colon__bool_Pointer_Pointer(T, bool p, Pointer a,
                                                  Pointer b)
    {
        return p ? a : b;
    }

#undef T

    void OpaqueType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;
        Symbol* s = scope();

        Mapped |= NativeInlined;
        CommOp |= NativeInlined;
        Op |= NativeInlined;
        AsOp |= NativeInlined;
        Lossy |= NativeInlined;
        Cast |= NativeInlined;

        string rname = name();
        rname += "&";
        const char* n = fullyQualifiedName().c_str();
        string nref = n;
        nref += "&";
        const char* nr = nref.c_str();

        Context* c = context();

        s->addSymbols(new ReferenceType(c, rname.c_str(), this),

                      new Function(c, name().c_str(), OpaqueType::dereference,
                                   Cast, Compiled,
                                   __C_Pointer_Pointer_PointerAmp_, Return, n,
                                   Args, nr, End),
                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "=", OpaqueType::assign, AsOp, Compiled,
                         __C_EQ__PointerAmp__PointerAmp__Pointer, Return, nr,
                         Args, nr, n, End),

            new Function(c, "?:", OpaqueType::conditionalExpr,
                         Op ^ NativeInlined, // not inlined
                         Compiled, __C_QMark_Colon__bool_Pointer_Pointer,
                         Return, n, Args, "bool", n, n, End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(OpaqueType::dereference, Pointer)
    {
        Pointer* ip = reinterpret_cast<Pointer*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*ip);
    }

    NODE_IMPLEMENTATION(OpaqueType::conditionalExpr, Pointer)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_ARG(1, Pointer)
                                      : NODE_ARG(2, Pointer));
    }

    NODE_IMPLEMENTATION(OpaqueType::assign, Pointer)
    {
        Pointer* ip = reinterpret_cast<Pointer*>(NODE_ARG(0, Pointer));
        *ip = NODE_ARG(1, Pointer);
        NODE_RETURN((Pointer)ip);
    }

} // namespace Mu
