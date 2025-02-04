#ifndef __MuLang__NameType__h__
#define __MuLang__NameType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/PrimitiveType.h>
#include <Mu/PrimitiveObject.h>

namespace Mu
{

    class NameType : public PrimitiveType
    {
    public:
        NameType(Context*);
        virtual ~NameType();

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

        static NODE_DECLARATION(dereference, Pointer);
        static NODE_DECLARATION(assign, Pointer);
        static NODE_DECLARATION(to_string, Pointer);
    };

} // namespace Mu

#endif // __MuLang__NameType__h__
