#ifndef __Mu__VariantType__h__
#define __Mu__VariantType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>

namespace Mu
{
    class VariantTagType;

    //
    //  class VariantType
    //
    //  A disjoint (tagged) union type. No value is of VariantType. A
    //  value must be a VariantTagType. A variable may be a VariantType
    //  but not a VariantTagType (although this restriction could be
    //  lifted).
    //

    class VariantType : public Type
    {
    public:
        //
        //  Types
        //

        //
        //  Construction
        //
        //  You can create a VariantType in two steps:
        //
        //  1) Create the type and add it to the scope you want:
        //
        //      VariantType* t = new VariantType("Foo");
        //      scope->addSymbol(t);
        //
        //  2) Create (and add) the VariantTagTypes
        //
        //      t->addSymbols( VariantTagType("A", "int"),
        //                     VariantTagType("B", "float"),
        //                     VariantTagType("C", "string"),
        //                     EndArguments );
        //

        //
        //  When using this constructor, ancillary functions are created
        //  for you. There should be an even number of arguments to the
        //  function.
        //

        VariantType(Context* context, const char* name);

        virtual ~VariantType();

        virtual void addSymbol(Symbol*);

        size_t numTagTypes() const { return _numTags; }

        //
        //  Type and Symbol API
        //

        virtual MatchResult match(const Type*, Bindings&) const;
        virtual Object* newObject() const;
        virtual size_t objectSize() const;
        virtual Value nodeEval(const Node*, Thread& t) const;
        virtual void nodeEval(void*, const Node*, Thread& t) const;
        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void constructInstance(Pointer) const;
        virtual void copyInstance(Pointer, Pointer) const;

    protected:
        virtual void load();

    private:
        static NODE_DECLARATION(match, void);

    private:
        size_t _numTags;
    };

} // namespace Mu

#endif // __Mu__VariantType__h__
