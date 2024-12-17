#ifndef __MuLang__ShortType__h__
#define __MuLang__ShortType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Node.h>
#include <Mu/PrimitiveObject.h>
#include <Mu/PrimitiveType.h>
#include <iosfwd>

namespace Mu
{

    //
    //  class ShortType
    //
    //  Basic integer type: uses the normal machine 32 bit integer type.
    //

    class ShortType : public PrimitiveType
    {
    public:
        ShortType(Context* c);
        ~ShortType();

        //
        //	Type API
        //

        virtual PrimitiveObject* newObject() const;
        virtual Value nodeEval(const Node*, Thread&) const;
        virtual void nodeEval(void*, const Node*, Thread&) const;

        //
        //	Output the symbol name
        //	Output the appropriate Value in human readable form
        //

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;

        //
        //	Load function is called when the symbol is added to the
        //	context.
        //

        virtual void load();

        //
        //	Casts
        //

        static NODE_DECLARATION(defaultShort, short);
        static NODE_DECLARATION(dereference, short);
        static NODE_DECLARATION(int2short, short);
        static NODE_DECLARATION(float2short, short);
        static NODE_DECLARATION(fromShort, int);

        //
        //	Operators
        //

        static NODE_DECLARATION(add, short);
        static NODE_DECLARATION(sub, short);
        static NODE_DECLARATION(mult, short);
        static NODE_DECLARATION(div, short);
        static NODE_DECLARATION(mod, short);
        static NODE_DECLARATION(negate, short);
        static NODE_DECLARATION(conditionalExpr, short);

        static NODE_DECLARATION(assign, Pointer);
        static NODE_DECLARATION(assignPlus, Pointer);
        static NODE_DECLARATION(assignSub, Pointer);
        static NODE_DECLARATION(assignMult, Pointer);
        static NODE_DECLARATION(assignDiv, Pointer);
        static NODE_DECLARATION(assignMod, Pointer);

        static NODE_DECLARATION(preInc, short);
        static NODE_DECLARATION(postInc, short);
        static NODE_DECLARATION(preDec, short);
        static NODE_DECLARATION(postDec, short);

        static NODE_DECLARATION(equals, bool);
        static NODE_DECLARATION(notEquals, bool);
        static NODE_DECLARATION(greaterThan, bool);
        static NODE_DECLARATION(lessThan, bool);
        static NODE_DECLARATION(greaterThanEq, bool);
        static NODE_DECLARATION(lessThanEq, bool);

        static NODE_DECLARATION(shiftLeft, short);
        static NODE_DECLARATION(shiftRight, short);
        static NODE_DECLARATION(bitAnd, short);
        static NODE_DECLARATION(bitOr, short);
        static NODE_DECLARATION(bitXor, short);
        static NODE_DECLARATION(bitNot, short);
    };

} // namespace Mu

#endif // __MuLang__ShortType__h__
