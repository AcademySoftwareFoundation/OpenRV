#ifndef __MuLang__NilType__h__
#define __MuLang__NilType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/Node.h>
#include <iosfwd>

namespace Mu
{

    //
    //  class NilType
    //
    //  This type is of "nebulous" ancestry. This makes it possible to
    //  cast NilType objects to any other class. So, nil inherts from all
    //  types.
    //
    //  Has a single value => Pointer(0).
    //

    class NilType : public Class
    {
    public:
        NilType(Context* context);
        ~NilType();

        //
        //	Type API
        //

        virtual Object* newObject() const;
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

    protected:
        //
        //  This is part of the Class API relevant to NilType
        //

        virtual bool nebulousIsA(const Class*) const;
    };

} // namespace Mu

#endif // __MuLang__NilType__h__
