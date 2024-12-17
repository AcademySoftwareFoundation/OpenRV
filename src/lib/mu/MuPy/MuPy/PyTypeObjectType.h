#ifndef __MuPy__PyTypeObjectType__h__
#define __MuPy__PyTypeObjectType__h__
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
    //  class PyTypeObjectType
    //
    //  An opaque type that wraps a PyObject*
    //  Functions in PyModule operate on this type
    //

    class PyTypeObjectType : public OpaqueType
    {
    public:
        PyTypeObjectType(Context*, const char*);
        ~PyTypeObjectType();

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void load();
    };

} // namespace Mu

#endif // __MuPy__PyTypeObjectType__h__
