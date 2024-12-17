#ifndef __MuLang__CharType__h__
#define __MuLang__CharType__h__
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
    //  class CharType
    //
    //  Character type. The class is almost entirely based off the IntType.
    //

    class CharType : public PrimitiveType
    {
    public:
        CharType(Context*);
        ~CharType();

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
        //  Cast
        //

        static NODE_DECLARATION(fromInt, int);
        static NODE_DECLARATION(toString, Pointer);
    };

} // namespace Mu

#endif // __MuLang__CharType__h__
