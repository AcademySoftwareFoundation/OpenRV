#ifndef __MuLang__ObjectInterface__h__
#define __MuLang__ObjectInterface__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Interface.h>

namespace Mu
{

    //
    //  class ObjectInterface
    //
    //  The "universal" interface. Anything can be cast to "object". The
    //  interface gives access to general characteristics of any
    //  non-primitive type object.
    //

    class ObjectInterface : public Interface
    {
    public:
        ObjectInterface(Context*);
        virtual ~ObjectInterface();

        virtual void load();
        static NODE_DECLARATION(identity, Pointer);
    };

} // namespace Mu

#endif // __MuLang__ObjectInterface__h__
