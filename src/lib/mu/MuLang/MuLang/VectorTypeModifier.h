#ifndef __MuLang__VectorTypeModifier__h__
#define __MuLang__VectorTypeModifier__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/TypeModifier.h>

namespace Mu
{

    class VectorTypeModifier : public TypeModifier
    {
    public:
        VectorTypeModifier(Context*);
        virtual ~VectorTypeModifier();

        virtual const Type* transform(const Type*, Context* c) const;

    private:
        mutable Type* _v4fType;
        mutable Type* _v3fType;
        mutable Type* _v2fType;
    };

} // namespace Mu

#endif // __MuLang__VectorTypeModifier__h__
