#ifndef __Mu__NodeFunc__h__
#define __Mu__NodeFunc__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Value.h>

namespace Mu
{

    class Symbol;
    class Type;
    class Node;
    class Thread;
    class Object;

    //
    //  Mu can be configured so that nodes return a Mu::Value structure or
    //  each node function has a unique return type and they are stored in
    //  a union.
    //

#ifdef MU_FUNCTION_UNION

    typedef void (*VoidFunc)(const Node&, Thread&);
    typedef Pointer (*PointerFunc)(const Node&, Thread&);
    typedef Value (*ValueFunc)(const Node&, Thread&);
    typedef float (*FloatFunc)(const Node&, Thread&);
    typedef double (*DoubleFunc)(const Node&, Thread&);
    typedef int (*IntFunc)(const Node&, Thread&);
    typedef int64 (*Int64Func)(const Node&, Thread&);
    typedef short (*ShortFunc)(const Node&, Thread&);
    typedef char (*CharFunc)(const Node&, Thread&);
    typedef bool (*BoolFunc)(const Node&, Thread&);
    typedef Vector4f (*Vector4fFunc)(const Node&, Thread&);
    typedef Vector3f (*Vector3fFunc)(const Node&, Thread&);
    typedef Vector2f (*Vector2fFunc)(const Node&, Thread&);

    template <typename T> struct TypedNodeFunc
    {
        MU_GC_NEW_DELETE
        typedef T (*Function)(const Node&, Thread&);
    };

    union NodeFunc
    {
        NodeFunc() { _voidFunc = 0; }

        NodeFunc(const NodeFunc& nf) { _voidFunc = nf._voidFunc; }

        explicit NodeFunc(int) { _valueFunc = 0; }

        NodeFunc(VoidFunc f) { _voidFunc = f; }

        NodeFunc(ValueFunc f) { _valueFunc = f; }

        NodeFunc(Vector4fFunc f) { _Vector4fFunc = f; }

        NodeFunc(Vector3fFunc f) { _Vector3fFunc = f; }

        NodeFunc(Vector2fFunc f) { _Vector2fFunc = f; }

        NodeFunc(FloatFunc f) { _floatFunc = f; }

        NodeFunc(DoubleFunc f) { _doubleFunc = f; }

        NodeFunc(PointerFunc f) { _PointerFunc = f; }

        NodeFunc(IntFunc f) { _intFunc = f; }

        NodeFunc(Int64Func f) { _int64Func = f; }

        NodeFunc(ShortFunc f) { _shortFunc = f; }

        NodeFunc(CharFunc f) { _charFunc = f; }

        NodeFunc(BoolFunc f) { _boolFunc = f; }

        VoidFunc _voidFunc;
        PointerFunc _PointerFunc;
        ValueFunc _valueFunc;
        Vector4fFunc _Vector4fFunc;
        Vector3fFunc _Vector3fFunc;
        Vector2fFunc _Vector2fFunc;
        FloatFunc _floatFunc;
        DoubleFunc _doubleFunc;
        IntFunc _intFunc;
        Int64Func _int64Func;
        ShortFunc _shortFunc;
        CharFunc _charFunc;
        BoolFunc _boolFunc;

        void operator=(const NodeFunc& n) { _voidFunc = n._voidFunc; }

        operator bool() const { return _voidFunc != 0; }
    };

    //
    //  Some sorcery: declare a struct to hold a typedef that will be
    //  the function return type for evaluate() function
    //
    //  NodeFunc F = ...;
    //  float value = evalNodeFunc<float>(F, node, thread);
    //

    template <typename T>
    inline T evalNodeFunc(NodeFunc F, const Node& N, Thread& Th)
    {
        return reinterpret_cast<typename TypedNodeFunc<T>::Function>(
            F._voidFunc)(N, Th);
    }

    template <typename T> inline NodeFunc nodefunc_cast(T f)
    {
        return NodeFunc(VoidFunc(f));
    }

    inline bool operator==(NodeFunc a, NodeFunc b)
    {
        return a._voidFunc == b._voidFunc;
    }

    inline bool operator!=(NodeFunc a, NodeFunc b)
    {
        return a._voidFunc != b._voidFunc;
    }

#else

    typedef Value (*NodeFunc)(const Node&, Thread&);

#endif

} // namespace Mu

#endif // __Mu__NodeFunc__h__
