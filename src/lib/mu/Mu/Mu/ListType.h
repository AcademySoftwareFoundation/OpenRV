#ifndef __MuLang__ListType__h__
#define __MuLang__ListType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>

namespace Mu
{

    //
    //  class ListType
    //
    //  While it makes sense to make ListType a VariantType, it would have
    //  different semantics from the other collection classes. Namely,
    //  non-primitive types would be copied into the list cell. By making
    //  it a Class instead, the List will have the same semantics as array
    //  and map.
    //

    class ListType : public Class
    {
    public:
        ListType(Context* context, const char* name, const Type* elementType);
        virtual ~ListType();

        virtual MatchResult match(const Type*, Bindings&) const;

        const Type* elementType() const { return _elementType; }

        size_t nextOffset() const { return _nextOffset; }

        size_t valueOffset() const { return _valueOffset; }

        //
        //	Symbol API
        //

        virtual void freeze();

        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void load();

        static NODE_DECLARATION(construct_aggregate, Pointer);
        static NODE_DECLARATION(cons, Pointer);
        static NODE_DECLARATION(tail, Pointer);

        static NODE_DECLARATION(head_int, int);
        static NODE_DECLARATION(head_int64, int64);
        static NODE_DECLARATION(head_float, float);
        static NODE_DECLARATION(head_bool, bool);
        static NODE_DECLARATION(head_char, char);
        static NODE_DECLARATION(head_short, short);
        static NODE_DECLARATION(head_Pointer, Pointer);
        static NODE_DECLARATION(head_Vector4f, Vector4f);
        static NODE_DECLARATION(head_Vector3f, Vector3f);
        static NODE_DECLARATION(head_Vector2f, Vector2f);

    private:
        const Type* _elementType;
        size_t _nextOffset;
        size_t _valueOffset;
    };

} // namespace Mu

#endif // __MuLang__ListType__h__
