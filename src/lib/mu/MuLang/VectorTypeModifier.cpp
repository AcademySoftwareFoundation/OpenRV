//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/VectorTypeModifier.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/VectorType.h>
#include <Mu/Type.h>

namespace Mu
{
    using namespace std;

    VectorTypeModifier::VectorTypeModifier(Context* c)
        : TypeModifier(c, "vector")
        , _v4fType(0)
        , _v3fType(0)
        , _v2fType(0)
    {
    }

    VectorTypeModifier::~VectorTypeModifier() {}

    const Type* VectorTypeModifier::transform(const Type* type,
                                              Context* context) const
    {
        MuLangContext* c = static_cast<MuLangContext*>(context);
        Type* float4 = c->arrayType(c->floatType(), 1, 4);

        Context::PrimaryBit fence(context, false);

        if (type == float4)
        {
            if (!_v4fType)
            {
                _v4fType =
                    new Vector4fType(c, "vector float[4]", c->floatType(),
                                     Vector4FloatRep::rep());

                c->globalScope()->addSymbol(_v4fType);
            }

            return _v4fType;
        }

        Type* float3 = c->arrayType(c->floatType(), 1, 3);

        if (type == float3)
        {
            if (!_v3fType)
            {
                _v3fType =
                    new Vector3fType(c, "vector float[3]", c->floatType(),
                                     Vector3FloatRep::rep());

                c->globalScope()->addSymbol(_v3fType);
            }

            return _v3fType;
        }

        Type* float2 = c->arrayType(c->floatType(), 1, 2);

        if (type == float2)
        {
            if (!_v2fType)
            {
                _v2fType =
                    new Vector2fType(c, "vector float[2]", c->floatType(),
                                     Vector2FloatRep::rep());

                c->globalScope()->addSymbol(_v2fType);
            }

            return _v2fType;
        }

        return 0;
    }

} // namespace Mu
