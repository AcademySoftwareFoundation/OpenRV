#ifndef __Mu__InterfaceImp__h__
#define __Mu__InterfaceImp__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/NodeFunc.h>
#include <vector>

namespace Mu
{
    class Class;
    class Interface;

    //
    //  class InterfaceImp
    //
    //  There is one of these attached to any Class that implements an
    //  interface
    //

    class InterfaceImp
    {
    public:
        InterfaceImp(const Class*, const Interface*);
        ~InterfaceImp();
        typedef STLVector<NodeFunc>::Type VTable;

        const Class* implementor() const { return _class; }

        const Interface* iface() const { return _interface; }

        NodeFunc func(int i) const { return _vtable[i]; }

    protected:
        const Class* _class;
        const Interface* _interface;
        VTable _vtable;

        friend class Interface;
    };

} // namespace Mu

#endif // __Mu__InterfaceImp__h__
