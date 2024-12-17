#ifndef __MuLang__VectorType__h__
#define __MuLang__VectorType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/Exception.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/Node.h>
#include <Mu/NodeAssembler.h>
#include <Mu/PrimitiveObject.h>
#include <Mu/PrimitiveType.h>
#include <Mu/ReferenceType.h>
#include <Mu/Value.h>
#include <Mu/Vector.h>
#include <iostream>
#include <math.h>
#include <stdio.h>

namespace Mu
{

    //
    //  class VectorType
    //
    //  A Vector (small primitive array) of some machine rep. The class is
    //  parameterized by the machine type (Vector<> most likely).
    //

    template <class T> class VectorType : public PrimitiveType
    {
    public:
        MU_GC_NEW_DELETE

        VectorType(Context* c, const char* name, const Type* elementType,
                   MachineRep* rep);
        ~VectorType();

        //
        //	Type API
        //

        virtual PrimitiveObject* newObject() const;
        virtual Value nodeEval(const Node*, Thread&) const;
        virtual void nodeEval(void*, const Node*, Thread&) const;

        //
        //	Symbol API
        //

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void load();

        //
        //	The element type
        //

        const Type* elementType() const { return _elementType; }

        //
        //	Template magic. These two inline functions are specialized.
        //

        static inline typename T::value_type elementArg(const Node&, Thread&,
                                                        int i);
        static inline T vectorArg(const Node&, Thread&, int i);
        static inline T nodeData(const Node&);

        //
        //	Constant / Constructors
        //

        static NODE_DECLARATION(defaultVector, T);
        static NODE_DECLARATION(dereference, T);
        static NODE_DECLARATION(construct4, T);
        static NODE_DECLARATION(construct3, T);
        static NODE_DECLARATION(construct2, T);
        static NODE_DECLARATION(construct1, T);

        //
        //  Type scope functions
        //

        static NODE_DECLARATION(indexop, typename T::value_type);
        static NODE_DECLARATION(indexopr, Pointer);

        //
        //	operators
        //

        static NODE_DECLARATION(add, T);
        static NODE_DECLARATION(sub, T);
        static NODE_DECLARATION(mult, T);
        static NODE_DECLARATION(div, T);
        static NODE_DECLARATION(mod, T);
        static NODE_DECLARATION(negate, T);
        static NODE_DECLARATION(conditionalExpr, T);
        static NODE_DECLARATION(__assign, void);
        static NODE_DECLARATION(assign, Pointer);
        static NODE_DECLARATION(assignPlus, Pointer);
        static NODE_DECLARATION(assignSub, Pointer);
        static NODE_DECLARATION(assignMult, Pointer);
        static NODE_DECLARATION(assignDiv, Pointer);
        static NODE_DECLARATION(equals, bool);
        static NODE_DECLARATION(notEquals, bool);

        static NODE_DECLARATION(cross, T);
        static NODE_DECLARATION(dot, typename T::value_type);
        static NODE_DECLARATION(mag, typename T::value_type);
        static NODE_DECLARATION(normalize, T);

        //
        //	Basic Math functions
        //

        static NODE_DECLARATION(print, void);

    private:
        const Type* _elementType;
    };

    //----------------------------------------------------------------------
    //
    //  Specializations
    //

    template <>
    inline float VectorType<Vector4f>::elementArg(const Node& n, Thread& t,
                                                  int i)
    {
        return n.floatArg(i, t);
    }

    template <>
    inline float VectorType<Vector3f>::elementArg(const Node& n, Thread& t,
                                                  int i)
    {
        return n.floatArg(i, t);
    }

    template <>
    inline float VectorType<Vector2f>::elementArg(const Node& n, Thread& t,
                                                  int i)
    {
        return n.floatArg(i, t);
    }

    template <>
    inline Vector4f VectorType<Vector4f>::vectorArg(const Node& n, Thread& t,
                                                    int i)
    {
        return n.Vector4fArg(i, t);
    }

    template <>
    inline Vector3f VectorType<Vector3f>::vectorArg(const Node& n, Thread& t,
                                                    int i)
    {
        return n.Vector3fArg(i, t);
    }

    template <>
    inline Vector2f VectorType<Vector2f>::vectorArg(const Node& n, Thread& t,
                                                    int i)
    {
        return n.Vector2fArg(i, t);
    }

    template <> inline Vector4f VectorType<Vector4f>::nodeData(const Node& n)
    {
        const DataNode& dn = static_cast<const DataNode&>(n);
        return dn._data._Vector4f;
    }

    template <> inline Vector3f VectorType<Vector3f>::nodeData(const Node& n)
    {
        const DataNode& dn = static_cast<const DataNode&>(n);
        return dn._data._Vector3f;
    }

    template <> inline Vector2f VectorType<Vector2f>::nodeData(const Node& n)
    {
        const DataNode& dn = static_cast<const DataNode&>(n);
        return dn._data._Vector2f;
    }

#define NODE_EARG(I) elementArg(NODE_THIS, NODE_THREAD, I)
#define NODE_VARG(I) vectorArg(NODE_THIS, NODE_THREAD, I)
#define NODE_VDATA() nodeData(NODE_THIS)

    //
    //
    //

    template <class T>
    VectorType<T>::VectorType(Context* c, const char* name, const Type* eType,
                              MachineRep* rep)
        : PrimitiveType(c, name, rep)
        , _elementType(eType)
    {
    }

    template <class T> VectorType<T>::~VectorType() {}

    template <class T> PrimitiveObject* VectorType<T>::newObject() const
    {
        return new PrimitiveObject(this);
    }

    template <class T>
    Value VectorType<T>::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._Vector4fFunc)(*n, thread));
    }

    template <class T>
    void VectorType<T>::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        T* tp = reinterpret_cast<T*>(p);
        *tp = (*n->func()._Vector4fFunc)(*n, thread);
    }

    template <>
    void VectorType<Vector4f>::nodeEval(void* p, const Node* n,
                                        Thread& thread) const
    {
        Vector4f* tp = reinterpret_cast<Vector4f*>(p);
        *tp = (*n->func()._Vector4fFunc)(*n, thread);
    }

    template <>
    void VectorType<Vector3f>::nodeEval(void* p, const Node* n,
                                        Thread& thread) const
    {
        Vector3f* tp = reinterpret_cast<Vector3f*>(p);
        *tp = (*n->func()._Vector3fFunc)(*n, thread);
    }

    template <>
    void VectorType<Vector2f>::nodeEval(void* p, const Node* n,
                                        Thread& thread) const
    {
        Vector2f* tp = reinterpret_cast<Vector2f*>(p);
        *tp = (*n->func()._Vector2fFunc)(*n, thread);
    }

    template <>
    Value VectorType<Vector4f>::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._Vector4fFunc)(*n, thread));
    }

    template <>
    Value VectorType<Vector3f>::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._Vector3fFunc)(*n, thread));
    }

    template <>
    Value VectorType<Vector2f>::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._Vector2fFunc)(*n, thread));
    }

    template <class T>
    void VectorType<T>::outputValue(std::ostream& o, const Value& value,
                                    bool full) const
    {
        // nothing
    }

    template <class T>
    void VectorType<T>::outputValueRecursive(std::ostream& o,
                                             const ValuePointer vp,
                                             ValueOutputState& state) const
    {
        o << *reinterpret_cast<const T*>(vp);
    }

    template <>
    void VectorType<Vector4f>::outputValue(std::ostream& o, const Value& value,
                                           bool full) const
    {
        o << value._Vector4f;
    }

    template <>
    void VectorType<Vector3f>::outputValue(std::ostream& o, const Value& value,
                                           bool full) const
    {
        o << value._Vector3f;
    }

    template <>
    void VectorType<Vector2f>::outputValue(std::ostream& o, const Value& value,
                                           bool full) const
    {
        o << value._Vector2f;
    }

    template <class T> void VectorType<T>::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();
        const MachineRep* rep = machineRep();
        char rn[80], ern[80];
        const char* tn = name().c_str();
        sprintf(rn, "%s&", tn);
        const char* en = elementType()->name().c_str();
        sprintf(ern, "%s&", en);

        const char* memberNames = "xyzw";

        for (int i = 0; i < rep->width(); i++)
        {
            char vname[2];
            vname[0] = memberNames[i];
            vname[1] = 0;
            addSymbol(new MemberVariable(c, vname, en, i));

            if (i == 2)
            {
                s->addSymbols(
                    new Function(c, tn, VectorType<T>::construct3, Mapped,
                                 Return, tn, Args, en, en, en, End),

                    new Function(c, "cross", VectorType<T>::cross, Mapped,
                                 Return, tn, Args, tn, tn, End),
                    EndArguments);
            }

            if (i == 3)
            {
                s->addSymbol(new Function(c, tn, VectorType<T>::construct4,
                                          Mapped, Return, tn, Args, en, en, en,
                                          en, End));
            }
        }

        s->addSymbols(new ReferenceType(c, rn, this),

                      new Function(c, tn, VectorType<T>::defaultVector, Mapped,
                                   Return, tn, End),

                      new Function(c, tn, VectorType<T>::dereference, Cast,
                                   Return, tn, Args, rn, End),

                      new Function(c, tn, VectorType<T>::construct2, Mapped,
                                   Return, tn, Args, en, en, End),

                      new Function(c, tn, VectorType<T>::construct1, Cast,
                                   Return, tn, Args, en, End),

                      new Function(c, "+", VectorType<T>::add, CommOp, Return,
                                   tn, Args, tn, tn, End),

                      new Function(c, "-", VectorType<T>::sub, Op, Return, tn,
                                   Args, tn, tn, End),

                      new Function(c, "-", VectorType<T>::negate, Op, Return,
                                   tn, Args, tn, End),

                      new Function(c, "*", VectorType<T>::mult, CommOp, Return,
                                   tn, Args, tn, tn, End),

                      new Function(c, "/", VectorType<T>::div, Op, Return, tn,
                                   Args, tn, tn, End),

                      new Function(c, "__assign", VectorType<T>::assign, AsOp,
                                   Return, "void", Args, rn, tn, Optional,
                                   "??+", End),

                      new Function(c, "=", VectorType<T>::assign, AsOp, Return,
                                   rn, Args, rn, tn, End),

                      new Function(c, "+=", VectorType<T>::assignPlus, AsOp,
                                   Return, rn, Args, rn, tn, End),

                      new Function(c, "-=", VectorType<T>::assignSub, AsOp,
                                   Return, rn, Args, rn, tn, End),

                      new Function(c, "*=", VectorType<T>::assignMult, AsOp,
                                   Return, rn, Args, rn, tn, End),

                      new Function(c, "/=", VectorType<T>::assignDiv, AsOp,
                                   Return, rn, Args, rn, tn, End),

                      new Function(c, "?:", VectorType<T>::conditionalExpr, Op,
                                   Return, tn, Args, "bool", tn, tn, End),

                      new Function(c, "print", VectorType<T>::print, None,
                                   Return, "void", Args, tn, End),

                      new Function(c, "==", VectorType<T>::equals, CommOp,
                                   Return, "bool", Args, tn, tn, End),

                      new Function(c, "!=", VectorType<T>::notEquals, CommOp,
                                   Return, "bool", Args, tn, tn, End),

                      new Function(c, "dot", VectorType<T>::dot, Mapped, Return,
                                   en, Args, tn, tn, End),

                      new Function(c, "mag", VectorType<T>::mag, Mapped, Return,
                                   en, Args, tn, End),

                      new Function(c, "normalize", VectorType<T>::normalize,
                                   Mapped, Return, tn, Args, tn, End),

                      EndArguments);

        addSymbols(new Function(c, "[]", VectorType<T>::indexop, Mapped, Return,
                                en, Args, tn, "int", End),

                   new Function(c, "[]", VectorType<T>::indexopr, Mapped,
                                Return, ern, Args, rn, "int", End),
                   0);
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::dereference, T)
    {
        T* vp = reinterpret_cast<T*>(NODE_ARG(0, Pointer));
        NODE_RETURN(*vp);
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::defaultVector, T)
    {
        T vp;
        assignOp(vp, 0.0f);
        NODE_RETURN(vp);
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::construct4, T)
    {
        T v;
        v[0] = NODE_EARG(0);
        v[1] = NODE_EARG(1);
        v[2] = NODE_EARG(2);
        v[3] = NODE_EARG(3);
        NODE_RETURN(v);
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::construct3, T)
    {
        T v;
        v[0] = NODE_EARG(0);
        v[1] = NODE_EARG(1);
        v[2] = NODE_EARG(2);
        NODE_RETURN(v);
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::construct2, T)
    {
        T v;
        v[0] = NODE_EARG(0);
        v[1] = NODE_EARG(1);
        NODE_RETURN(v);
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::construct1, T)
    {
        T v;
        assignOp(v, NODE_EARG(0));
        NODE_RETURN(v);
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::print, void)
    {
        T v = NODE_VARG(0);
        std::cout << "PRINT: " << v << std::endl << std::flush;
    }

#define OP(name, op)                                \
    template <class T> NODE_IMPLEMENTATION(name, T) \
    {                                               \
        NODE_RETURN(NODE_VARG(0) op NODE_VARG(1));  \
    }

    OP(VectorType<T>::add, +)
    OP(VectorType<T>::sub, -)
    OP(VectorType<T>::mult, *)
    OP(VectorType<T>::div, /)
#undef OP

#define RELOP(name, op)                                \
    template <class T> NODE_IMPLEMENTATION(name, bool) \
    {                                                  \
        NODE_RETURN(NODE_VARG(0) op NODE_VARG(1));     \
    }

    RELOP(VectorType<T>::equals, ==)
    RELOP(VectorType<T>::notEquals, !=)
#undef RELOP

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::negate, T)
    {
        NODE_RETURN(-NODE_VARG(0));
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::cross, T)
    {
        NODE_RETURN(Mu::cross(NODE_VARG(0), NODE_VARG(1)));
    }

    template <class T>
    NODE_IMPLEMENTATION(VectorType<T>::dot, typename T::value_type)
    {
        NODE_RETURN(Mu::dot(NODE_VARG(0), NODE_VARG(1)));
    }

    template <class T>
    NODE_IMPLEMENTATION(VectorType<T>::mag, typename T::value_type)
    {
        NODE_RETURN(Mu::mag(NODE_VARG(0)));
    }

    template <class T>
    NODE_IMPLEMENTATION(VectorType<T>::indexop, typename T::value_type)
    {
        int index = NODE_ARG(1, int);
        if (index < 0 || index > T::dimension())
            throw OutOfRangeException(NODE_THREAD);
        NODE_RETURN(NODE_VARG(0)[index]);
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::indexopr, Pointer)
    {
        int index = NODE_ARG(1, int);
        if (index < 0 || index > T::dimension())
            throw OutOfRangeException(NODE_THREAD);
        T* vp = reinterpret_cast<T*>(NODE_ARG(0, Pointer));
        NODE_RETURN(&((*vp)[index]));
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::normalize, T)
    {
        NODE_RETURN(Mu::normalize(NODE_VARG(0)));
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::conditionalExpr, T)
    {
        NODE_RETURN(NODE_ARG(0, bool) ? NODE_VARG(1) : NODE_VARG(2));
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::__assign, void)
    {
        for (int i = 0, s = NODE_NUM_ARGS(); i < s; i += 2)
        {
            T* vp = reinterpret_cast<T*>(NODE_ARG(i, Pointer));
            *vp = NODE_VARG(i + 1);
        }
    }

    template <class T> NODE_IMPLEMENTATION(VectorType<T>::assign, Pointer)
    {
        T* vp = reinterpret_cast<T*>(NODE_ARG(0, Pointer));
        *vp = NODE_VARG(1);
        NODE_RETURN(vp);
    }

#define ASOP(name, op)                                      \
    template <class T> NODE_IMPLEMENTATION(name, Pointer)   \
    {                                                       \
        T* vp = reinterpret_cast<T*>(NODE_ARG(0, Pointer)); \
        *vp = *vp op NODE_VARG(1);                          \
        NODE_RETURN(vp);                                    \
    }

    ASOP(VectorType<T>::assignPlus, +)
    ASOP(VectorType<T>::assignSub, -)
    ASOP(VectorType<T>::assignMult, *)
    ASOP(VectorType<T>::assignDiv, /)
#undef ASOP

#undef NODE_EARG
#undef NODE_VARG

    typedef VectorType<Vector4f> Vector4fType;
    typedef VectorType<Vector3f> Vector3fType;
    typedef VectorType<Vector2f> Vector2fType;

} // namespace Mu

#endif // __MuLang__VectorType__h__
