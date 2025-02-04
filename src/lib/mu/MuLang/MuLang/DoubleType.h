#ifndef __MuLang__DoubleType__h__
#define __MuLang__DoubleType__h__
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
    //  class DoubleType
    //
    //  Basic integer type: uses the normal machine 32 bit integer type.
    //

    class DoubleType : public PrimitiveType
    {
    public:
        DoubleType(Context*);
        ~DoubleType();

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

        static NODE_DECLARATION(defaultDouble, double);
        static NODE_DECLARATION(dereference, double);
        static NODE_DECLARATION(int2double, double);
        static NODE_DECLARATION(float2double, double);
        static NODE_DECLARATION(int642double, double);

        //
        //	Operators
        //

        static NODE_DECLARATION(add, double);
        static NODE_DECLARATION(sub, double);
        static NODE_DECLARATION(mult, double);
        static NODE_DECLARATION(div, double);
        static NODE_DECLARATION(mod, double);
        static NODE_DECLARATION(negate, double);
        static NODE_DECLARATION(conditionalExpr, double);

        static NODE_DECLARATION(assign, Pointer);
        static NODE_DECLARATION(assignPlus, Pointer);
        static NODE_DECLARATION(assignSub, Pointer);
        static NODE_DECLARATION(assignMult, Pointer);
        static NODE_DECLARATION(assignDiv, Pointer);
        static NODE_DECLARATION(assignMod, Pointer);

        static NODE_DECLARATION(preInc, double);
        static NODE_DECLARATION(postInc, double);
        static NODE_DECLARATION(preDec, double);
        static NODE_DECLARATION(postDec, double);

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

#endif // __MuLang__DoubleType__h__
