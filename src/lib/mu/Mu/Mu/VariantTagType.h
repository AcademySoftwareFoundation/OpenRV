#ifndef __Mu__VariantTagType__h__
#define __Mu__VariantTagType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/VariantType.h>

namespace Mu
{

    //
    //  A VariantTagType is a *subclass* of its VariantType
    //

    class VariantTagType : public Type
    {
    public:
        //
        //  You can make a VariantTagType with either the type or the name
        //  of the type.
        //

        VariantTagType(Context* context, const char* name, const Type* repType);
        VariantTagType(Context* context, const char* name, const char* repType);
        virtual ~VariantTagType();

        const Type* representationType() const;

        const VariantType* variantType() const
        {
            return static_cast<const VariantType*>(scope());
        }

        size_t index() const { return _index; }

        //
        //  Type API
        //

        virtual void serialize(std::ostream&, Archive::Writer&,
                               const ValuePointer) const;
        virtual void deserialize(std::istream&, Archive::Reader&,
                                 ValuePointer) const;
        virtual void reconstitute(Archive::Reader&, Object*) const;

        virtual MatchResult match(const Type*, Bindings&) const;
        virtual Object* newObject() const;
        virtual size_t objectSize() const;
        virtual Value nodeEval(const Node*, Thread& t) const;
        virtual void nodeEval(void*, const Node*, Thread& t) const;
        virtual const Type* nodeReturnType(const Node*) const;

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual const Type* fieldType(size_t) const;
        virtual ValuePointer fieldPointer(Object*, size_t) const;
        virtual const ValuePointer fieldPointer(const Object*, size_t) const;
        virtual void constructInstance(Pointer) const;
        virtual void copyInstance(Pointer, Pointer) const;

        //
        //  Nodes
        //

        static NODE_DECLARATION(upcast, Pointer);

    protected:
        virtual void load();
        virtual bool resolveSymbols() const;

    private:
        mutable SymbolRef _repType;
        size_t _index;

        friend class VariantType;
    };

} // namespace Mu

#endif // __Mu__VariantTagType__h__
