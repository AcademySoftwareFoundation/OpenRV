#ifndef __MuPy__PyObjectType__h__
#define __MuPy__PyObjectType__h__
//
// Copyright (c) 2011, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/OpaqueType.h>
#include <iosfwd>

namespace Mu
{

    //
    //  class PyObjectType
    //
    //  An opaque type that wraps a PyObject*
    //  Functions in PyModule operate on this type
    //

    class PyObjectType : public OpaqueType
    {
    public:
        PyObjectType(Context*, const char*);
        ~PyObjectType();

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void load();
    };

} // namespace Mu

#endif // __MuPy__PyObjectType__h__
