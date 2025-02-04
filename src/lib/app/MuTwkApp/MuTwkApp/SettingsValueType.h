//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkApp__SettingsValueType__h__
#define __TwkApp__SettingsValueType__h__
#include <iostream>
#include <Mu/VariantType.h>

namespace Mu
{
    class VariantInstance;
}

namespace TwkApp
{

    class SettingsValueType : public Mu::VariantType
    {
    public:
        enum ValueType
        {
            NoType,
            FloatType,
            IntType,
            BoolType,
            StringType,
            StringArrayType,
            FloatArrayType,
            IntArrayType
        };

        SettingsValueType(Mu::Context*);
        virtual ~SettingsValueType();

        virtual void load();

        static NODE_DECLARATION(defaultConstructor, Mu::Pointer);

        ValueType valueType(Mu::VariantInstance*) const;

        const Mu::VariantTagType* boolType() const { return _boolType; }

        const Mu::VariantTagType* intType() const { return _intType; }

        const Mu::VariantTagType* floatType() const { return _floatType; }

        const Mu::VariantTagType* stringType() const { return _stringType; }

        const Mu::VariantTagType* stringArrayType() const
        {
            return _stringArrayType;
        }

        const Mu::VariantTagType* floatArrayType() const
        {
            return _floatArrayType;
        }

        const Mu::VariantTagType* intArrayType() const { return _intArrayType; }

    private:
        Mu::VariantTagType* _boolType;
        Mu::VariantTagType* _intType;
        Mu::VariantTagType* _floatType;
        Mu::VariantTagType* _stringType;
        Mu::VariantTagType* _intArrayType;
        Mu::VariantTagType* _stringArrayType;
        Mu::VariantTagType* _floatArrayType;
    };

} // namespace TwkApp

#endif // __TwkApp__SettingsValueType__h__
