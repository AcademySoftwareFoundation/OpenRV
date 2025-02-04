#ifndef __MuLang__VoidType__h__
#define __MuLang__VoidType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/PrimitiveType.h>
#include <Mu/PrimitiveObject.h>
#include <iosfwd>
#include <Mu/Node.h>

namespace Mu
{

    //
    //  class VoidType
    //
    //  Basic integer type: uses the normal machine 32 bit integer type.
    //

    class VoidType : public PrimitiveType
    {
    public:
        VoidType(Context*);
        ~VoidType();

        //
        //	Type API
        //

        virtual PrimitiveObject* newObject() const;
        virtual Value nodeEval(const Node*, Thread&) const;
        virtual void nodeEval(void*, const Node*, Thread&) const;
        virtual void constructInstance(Pointer) const;

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
    };

} // namespace Mu

#endif // __MuLang__VoidType__h__
