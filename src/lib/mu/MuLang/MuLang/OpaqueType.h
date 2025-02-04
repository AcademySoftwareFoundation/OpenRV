#ifndef __MuLang__OpaqueType__h__
#define __MuLang__OpaqueType__h__
//
// Copyright (c) 2010, Jim Hourihan
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
    //  class OpaqueType
    //
    //  Used to pass around a C++ void*
    //

    class OpaqueType : public PrimitiveType
    {
    public:
        OpaqueType(Context* c, const char*);
        ~OpaqueType();

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
        static NODE_DECLARATION(conditionalExpr, Pointer);
        static NODE_DECLARATION(assign, Pointer);
    };

} // namespace Mu

#endif // __MuLang__OpaqueType__h__
