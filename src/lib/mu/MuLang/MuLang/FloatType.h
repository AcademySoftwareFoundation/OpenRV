#ifndef __MuLang__FloatType__h__
#define __MuLang__FloatType__h__
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
    //  class FloatType
    //
    //  Basic integer type: uses the normal machine 32 bit integer type.
    //

    class FloatType : public PrimitiveType
    {
    public:
        FloatType(Context*);
        ~FloatType();

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

        static NODE_DECLARATION(defaultFloat, float);
        static NODE_DECLARATION(dereference, float);
        static NODE_DECLARATION(int2float, float);
        static NODE_DECLARATION(double2float, float);
        static NODE_DECLARATION(int642float, float);

        //
        //	Operators
        //

        static NODE_DECLARATION(add, float);
        static NODE_DECLARATION(sub, float);
        static NODE_DECLARATION(mult, float);
        static NODE_DECLARATION(div, float);
        static NODE_DECLARATION(mod, float);
        static NODE_DECLARATION(negate, float);
        static NODE_DECLARATION(conditionalExpr, float);

        static NODE_DECLARATION(assign, Pointer);
        static NODE_DECLARATION(assignPlus, Pointer);
        static NODE_DECLARATION(assignSub, Pointer);
        static NODE_DECLARATION(assignMult, Pointer);
        static NODE_DECLARATION(assignDiv, Pointer);
        static NODE_DECLARATION(assignMod, Pointer);

        static NODE_DECLARATION(preInc, float);
        static NODE_DECLARATION(postInc, float);
        static NODE_DECLARATION(preDec, float);
        static NODE_DECLARATION(postDec, float);

        static NODE_DECLARATION(equals, bool);
        static NODE_DECLARATION(notEquals, bool);
        static NODE_DECLARATION(greaterThan, bool);
        static NODE_DECLARATION(lessThan, bool);
        static NODE_DECLARATION(greaterThanEq, bool);
        static NODE_DECLARATION(lessThanEq, bool);

        //
        //	Basic Math functions
        //

        static NODE_DECLARATION(print, void);
    };

} // namespace Mu

#endif // __MuLang__FloatType__h__
