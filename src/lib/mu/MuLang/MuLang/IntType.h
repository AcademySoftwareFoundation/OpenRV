#ifndef __MuLang__IntType__h__
#define __MuLang__IntType__h__
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
    //  class IntType
    //
    //  Basic integer type: uses the normal machine 32 bit integer type.
    //

    class IntType : public PrimitiveType
    {
    public:
        IntType(Context*);
        ~IntType();

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

        static NODE_DECLARATION(defaultInt, int);
        static NODE_DECLARATION(dereference, int);
        static NODE_DECLARATION(float2int, int);
        static NODE_DECLARATION(double2int, int);

        //
        //	Operators
        //

        static NODE_DECLARATION(add, int);
        static NODE_DECLARATION(sub, int);
        static NODE_DECLARATION(mult, int);
        static NODE_DECLARATION(div, int);
        static NODE_DECLARATION(mod, int);
        static NODE_DECLARATION(negate, int);
        static NODE_DECLARATION(conditionalExpr, int);

        static NODE_DECLARATION(assign, Pointer);
        static NODE_DECLARATION(assignPlus, Pointer);
        static NODE_DECLARATION(assignSub, Pointer);
        static NODE_DECLARATION(assignMult, Pointer);
        static NODE_DECLARATION(assignDiv, Pointer);
        static NODE_DECLARATION(assignMod, Pointer);

        static NODE_DECLARATION(preInc, int);
        static NODE_DECLARATION(postInc, int);
        static NODE_DECLARATION(preDec, int);
        static NODE_DECLARATION(postDec, int);

        static NODE_DECLARATION(equals, bool);
        static NODE_DECLARATION(notEquals, bool);
        static NODE_DECLARATION(greaterThan, bool);
        static NODE_DECLARATION(lessThan, bool);
        static NODE_DECLARATION(greaterThanEq, bool);
        static NODE_DECLARATION(lessThanEq, bool);

        static NODE_DECLARATION(shiftLeft, int);
        static NODE_DECLARATION(shiftRight, int);
        static NODE_DECLARATION(bitAnd, int);
        static NODE_DECLARATION(bitOr, int);
        static NODE_DECLARATION(bitXor, int);
        static NODE_DECLARATION(bitNot, int);
    };

} // namespace Mu

#endif // __MuLang__IntType__h__
