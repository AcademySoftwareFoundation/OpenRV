#ifndef __Mu__Value__h__
#define __Mu__Value__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <assert.h>
#include <Mu/config.h>
#include <Mu/Vector.h>
#include <Mu/Name.h>
#include <string.h>

#ifdef _MSC_VER
#define _int64 _value_int64
#endif

namespace Mu
{
    class Type;

    //
    //  The Value object is used for two purposesin Mu: for storage of all
    //  any object (in the C sense) or reference to a Mu::Object for
    //  purposes of return-by-value or for permanant storage (or stack
    //  storage).
    //
    //  A Type which has a physical representation, like float or int,
    //  must be able to store is value in one of these fields. More than
    //  one Type may share the same field (such as boolean and int.)  If
    //  you want to add a new type which does not fit, you need to add a
    //  field to this union.
    //
    //  Reference types are handled by the _object field -- so for example
    //  a string object could be returned as an object pointer instead of
    //  requiring an explicit value here. In fact, all objects which are
    //  required to be garbage collected will need to be stored here.
    //

    union Value
    {
        Vector4f _Vector4f;
        Vector3f _Vector3f;
        Vector2f _Vector2f;
        float _float;
        double _double;
        int _int;
        int64 _int64;
        short _short;
        char _char;
        byte _byte;
        bool _bool;
        Pointer _Pointer;
        Name::Ref _name;

        Value() { memset(this, 0, sizeof(Value)); }

        explicit Value(const Vector4f& v) { assignOp(_Vector4f, v); }

        explicit Value(const Vector3f& v) { assignOp(_Vector3f, v); }

        explicit Value(const Vector2f& v) { assignOp(_Vector2f, v); }

        explicit Value(char i) { _char = i; }

        explicit Value(byte i) { _byte = i; }

        explicit Value(short i) { _short = i; }

        explicit Value(int i) { _int = i; }

        explicit Value(int64 i) { _int64 = i; }

        explicit Value(float f) { _float = f; }

        explicit Value(double f) { _double = f; }

        explicit Value(bool b) { _bool = b; }

        explicit Value(Pointer p) { _Pointer = p; }

        explicit Value(Name n) { _name = n.nameRef(); }

        Value(unsigned int i) { _int = (unsigned int)i; }

        inline Value(const Value&);
        MU_GC_NEW_DELETE

        //
        //  Value v;
        //  float val = v.as<float>();
        //

        template <typename T> inline T as() const;
        template <typename T> inline T& as();
    };

    template <class T> union Valign
    {
        Vector4f _Vector4f;
        Vector3f _Vector3f;
        Vector2f _Vector2f;
        float _float;
        double _double;
        int _int;
        int64 _int64;
        short _short;
        char _char;
        bool _bool;
        Pointer _Pointer;
        Name::Ref _name;

        T t;
    };

    inline Value::Value(const Value& v) { memcpy(this, &v, sizeof(Value)); }

    // template <typename T> T Value::as() const { return reinterpret_cast<const
    // Valign<T>*>(this)->t; } template <typename T> T& Value::as() { return
    // reinterpret_cast<Valign<T>*>(this)->t; }

#define MU_AS_FUNC(T)                                          \
    template <> inline T Value::as<T>() const { return _##T; } \
    template <> inline T& Value::as<T>() { return _##T; }

    MU_AS_FUNC(Vector4f)
    MU_AS_FUNC(Vector3f)
    MU_AS_FUNC(Vector2f)
    MU_AS_FUNC(float)
    MU_AS_FUNC(double)
    MU_AS_FUNC(int)
    MU_AS_FUNC(int64)
    MU_AS_FUNC(short)
    MU_AS_FUNC(char)
    MU_AS_FUNC(bool)
    MU_AS_FUNC(Pointer)

    inline void zero(Value& v)
    {
        memset(&v, 0, sizeof(v));
        // assignOp(v._Vector4f, 0.f);
    }

    //
    //  A value with an accociated type
    //

    struct TypedValue
    {
        MU_GC_NEW_DELETE
        TypedValue(const Value& v, const Type* t)
            : _value(v)
            , _type(t)
        {
        }

        TypedValue()
            : _value()
            , _type(0)
        {
        }

        Value _value;
        const Type* _type;
    };

    //
    //  Not the same as a pointer-to-Value. This thing directly points at
    //  a value. This is different than pointing to the Value union which
    //  may have a different start address for each of its members.
    //
    typedef void* ValuePointer;

} // namespace Mu

#endif // __Mu__Value__h__
