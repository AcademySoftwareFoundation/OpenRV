//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <MuTwkApp/SettingsValueType.h>
#include <Mu/Function.h>
#include <MuLang/MuLangContext.h>
#include <Mu/Module.h>
#include <Mu/Function.h>
#include <Mu/ReferenceType.h>
#include <Mu/VariantTagType.h>
#include <Mu/VariantInstance.h>
#include <Mu/BaseFunctions.h>
#include <MuLang/StringType.h>

namespace TwkApp
{
    using namespace std;
    using namespace Mu;

    SettingsValueType::SettingsValueType(Context* c)
        : VariantType(c, "SettingsValue")
    {
    }

    SettingsValueType::~SettingsValueType() {}

    void SettingsValueType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;
        context->arrayType(context->floatType(), 1, 0);
        context->arrayType(context->intType(), 1, 0);
        context->arrayType(context->stringType(), 1, 0);

        _floatType = new VariantTagType(c, "Float", "float");
        _intType = new VariantTagType(c, "Int", "int");
        _stringType = new VariantTagType(c, "String", "string");
        _boolType = new VariantTagType(c, "Bool", "bool");
        _floatArrayType = new VariantTagType(c, "FloatArray", "float[]");
        _intArrayType = new VariantTagType(c, "IntArray", "int[]");
        _stringArrayType = new VariantTagType(c, "StringArray", "string[]");

        addSymbols(_floatType, _intType, _stringType, _boolType,
                   _floatArrayType, _intArrayType, _stringArrayType,
                   EndArguments);

        String rtname = name().c_str();
        rtname += "&";
        ReferenceType* rt = new ReferenceType(c, rtname.c_str(), this);
        scope()->addSymbol(rt);

        const char* tn = fullyQualifiedName().c_str();

#if 0
    for (int i=0 i < constructors.size(); i++)
    {
        //
        //  Add default constructors (w/o arguments). 
        //

        VariantTagType* t = constructors[i];
        
        t->addSymbol( new Function(c, t->name().c_str(), 
                                   SettingsValueType::defaultConstructor, None,
                                   Return, tn,
                                   End) );
    }
#endif

        Function* OpAs = new Function(
            c, "=", BaseFunctions::assign,
            Function::MemberOperator | Function::Operator, Function::Return,
            rt->fullyQualifiedName().c_str(), Function::Args,
            rt->fullyQualifiedName().c_str(), tn, Function::End);

        globalScope()->addSymbol(OpAs);
    }

    NODE_IMPLEMENTATION(SettingsValueType::defaultConstructor, Pointer)
    {
        //
        //  Tricky -- get the scope of the constructor symbol -- that's
        //  the tag type.
        //

        const VariantTagType* t =
            static_cast<const VariantTagType*>(NODE_THIS.symbol()->scope());

        NODE_RETURN(VariantInstance::allocate(t));
    }

    SettingsValueType::ValueType
    SettingsValueType::valueType(VariantInstance* vobj) const
    {
        const VariantTagType* tt =
            static_cast<const VariantTagType*>(vobj->type());

        if (tt == _stringType)
            return StringType;
        else if (tt == _intType)
            return IntType;
        else if (tt == _boolType)
            return BoolType;
        else if (tt == _floatType)
            return FloatType;
        else if (tt == _stringArrayType)
            return StringArrayType;
        else if (tt == _floatArrayType)
            return FloatArrayType;
        else if (tt == _intArrayType)
            return IntArrayType;
        return NoType;
    }

} // namespace TwkApp
