#ifndef __MuImage__PixelsType__h__
#define __MuImage__PixelsType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/VariantType.h>

namespace Mu
{

    class PixelsType : public VariantType
    {
    public:
        PixelsType(Context* c);
        virtual ~PixelsType();

        virtual void load();

        static NODE_DECLARATION(defaultPixelConstructor, Pointer);
    };

} // namespace Mu

#endif // __MuImage__PixelsType__h__
