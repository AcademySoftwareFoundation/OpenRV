#ifndef __MuLang__Context__h__
#define __MuLang__Context__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
#include <Mu/Context.h>
#include <vector>

namespace Mu
{
    class Type;
    class Function;
    class Interface;
    class Class;
    class DynamicArrayType;
    class FixedArrayType;
    class Module;
    class StringType;

    //
    //  class Context
    //
    //  A Context with the base MuLang language installed by default.
    //

    class MuLangContext : public Context
    {
    public:
        //
        //  types
        //

        typedef std::vector<size_t> SizeVector; // should not require being GC'd
        typedef STLVector<String>::Type StringVector;
        typedef STLMap<const Type*, Type*>::Type DynamicArrayTypeCache;
        typedef STLMap<std::pair<const Type*, size_t>, Type*>::Type
            Fixed1DArrayTypeCache;

        MuLangContext(const char* imp, const char* name);
        ~MuLangContext();

        bool typeParsingMode() const { return _typeParsingMode; }

        void setParsedType(Type* t) { _parsedType = t; }

        //
        //  Single Entry API
        //

        virtual TypedValue evalText(const char* text, const char* inputName,
                                    Process*, const ModuleList&);

        virtual TypedValue evalFile(const char* file, Process*,
                                    const ModuleList&);

        virtual void parseStream(Process*, std::istream&,
                                 const char* inputName);

        virtual Type* parseType(const char* text, Process*);

        const Type* intType() const { return _intType; }

        const Type* int64Type() const { return _int64Type; }

        const Type* shortType() const { return _shortType; }

        const Type* doubleType() const { return _doubleType; }

        const Type* floatType() const { return _floatType; }

        const Type* halfType() const { return _halfType; }

        const Type* charType() const { return _charType; }

        const Type* byteType() const { return _byteType; }

        const Interface* objectType() const { return _objectType; }

        const StringType* stringType() const { return _stringType; }

        const Type* charArrayType() const { return _charArrayType; }

        const Class* regexType() const { return _regexType; }

        const Class* exceptionType() const { return _exceptionType; }

        const Type* nameType() const { return _nameType; }

        const Type* v4fType() const { return _v4fType; }

        const Type* v3fType() const { return _v3fType; }

        const Type* v2fType() const { return _v2fType; }

        const Type* vec4fType() const { return _vec4fType; }

        const Type* vec3fType() const { return _vec3fType; }

        const Type* vec2fType() const { return _vec2fType; }

        const Module* mathModule() const { return _mathModule; }

        const Module* mathUtilModule() const { return _mathUtilModule; }

        virtual Object* exceptionObject(Exception&);

        //
        //	Find or create the array type given a dimension vector.
        //

        Type* arrayType(const Type* elementType, const SizeVector& dimensions);

        Type* arrayType(const Type* elementType,
                        const STLVector<size_t>::Type& dimensions);

        Type* arrayType(const Type* elementType, const size_t* dimensions,
                        size_t nDimensions);

        //
        //	Same as above except uses stdargs:
        //	for example, to find a dynamic float array:
        //
        //	    arrayType(floatType, 1, 0);
        //

        Type* arrayType(const Type* elementType, size_t nDimensions, ...);

    private:
        Type* _intType;
        Type* _int64Type;
        Type* _shortType;
        Type* _floatType;
        Type* _doubleType;
        Type* _halfType;
        Type* _charType;
        Type* _byteType;
        const Type* _v4fType;
        const Type* _v3fType;
        const Type* _v2fType;
        const Type* _vec4fType;
        const Type* _vec3fType;
        const Type* _vec2fType;
        const Type* _nameType;
        Interface* _objectType;
        StringType* _stringType;
        Type* _charArrayType;
        Class* _regexType;
        Class* _exceptionType;
        Module* _mathModule;
        Module* _mathUtilModule;
        bool _typeParsingMode;
        Type* _parsedType;
        DynamicArrayTypeCache _dynArrayCache;
        Fixed1DArrayTypeCache _fixed1ArrayCache;
    };

} // namespace Mu

#endif // __MuLang__Context__h__
