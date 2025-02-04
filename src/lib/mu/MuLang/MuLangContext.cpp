//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/BoolType.h>
#include <MuLang/ByteType.h>
#include <MuLang/CharType.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/FloatType.h>
#include <MuLang/DoubleType.h>
#include <MuLang/HalfType.h>
#include <MuLang/Int64Type.h>
#include <MuLang/IntType.h>
#include <MuLang/MathModule.h>
#include <MuLang/MathUtilModule.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/NameType.h>
#include <MuLang/ObjectInterface.h>
#include <MuLang/Parse.h>
#include <MuLang/RegexType.h>
#include <MuLang/RuntimeModule.h>
#include <MuLang/ShortType.h>
#include <MuLang/StringType.h>
#include <MuLang/TypePattern.h>
#include <MuLang/VectorTypeModifier.h>
#include <MuLang/VoidType.h>
#include <Mu/Alias.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Context.h>
#include <Mu/SymbolType.h>
#include <Mu/Exception.h>
#include <Mu/Language.h>
#include <Mu/Module.h>
#include <Mu/NilType.h>
#include <Mu/NodeAssembler.h>
#include <Mu/OpaqueType.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/TypePattern.h>
#include <fstream>
#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <typeinfo>

namespace Mu
{
    using namespace std;

    MuLangContext::MuLangContext(const char* imp, const char* name)
        : Context(imp, name)
        , _typeParsingMode(false)
    {
        PrimaryBit fence(this, false);

        _nilType = new NilType(this);
        _voidType = new VoidType(this);
        _intType = new IntType(this);
        _int64Type = new Int64Type(this);
        _shortType = new ShortType(this);
        _floatType = new FloatType(this);
        _doubleType = new DoubleType(this);
        _halfType = new HalfType(this);
        _boolType = new BoolType(this);
        _objectType = new ObjectInterface(this);
        _charType = new CharType(this);
        _byteType = new ByteType(this);
        _stringType = new StringType(this);
        _regexType = new RegexType(this);
        _exceptionType = new ExceptionType(this);

        Symbol* s = globalScope();

        s->addSymbol(new Alias(this, "__root", s));
        s->addSymbol(new MatchDynamicArray(this));
        s->addSymbol(new MatchFixedArray(this));
        s->addSymbol(new MatchOpaque(this));
        s->addSymbol(new MatchList(this));
        s->addSymbol(new MatchABoolRep(this));

        s->addSymbol(new CaseTest(this));
        s->addSymbol(new PatternTest(this));
        s->addSymbol(new BoolPatternTest(this));

        s->addSymbol(new VarArg(this));
        s->addSymbol(_matchAnyType = new MatchAnyType(this));
        s->addSymbol(new MatchAnyThing(this));
        s->addSymbol(new OneRepeatedArg(this));
        s->addSymbol(new TwoRepeatedArg(this));
        s->addSymbol(new MatchAnyClass(this));
        s->addSymbol(new MatchAnyTuple(this));
        s->addSymbol(new MatchAnyObjectButNotTuple(this));
        s->addSymbol(new MatchAnyClassButNotTuple(this));
        s->addSymbol(new MatchAnyReference(this));
        s->addSymbol(new MatchAnyClassOrInterface(this));
        s->addSymbol(new MatchNonPrimitiveOrNil(this));
        s->addSymbol(new MatchAnyNonPrimitiveReference(this));
        s->addSymbol(new MatchAnyFunction(this));
        s->addSymbol(new MatchAnyVariant(this));

        _noop = new NoOp(this, "__no_op");
        _simpleBlock = new SimpleBlock(this, "__statement_list");
        _patternBlock = new PatternBlock(this, "__pattern_block");
        _fixedFrameBlock = new FixedFrameBlock(this, "__frame");
        _dynamicCast = new DynamicCast(this, "__dynamic_cast");
        _curry = new Curry(this, "__curry");
        _dynamicPartialApply =
            new DynamicPartialApplication(this, "__partial_apply");
        _dynamicPartialEval =
            new DynamicPartialEvaluate(this, "__partial_eval");
        _returnFromFunction = new ReturnFromFunction(this, "__return");
        _returnFromVoidFunction =
            new ReturnFromFunction(this, "__return", false);
        _matchFunction = new VariantMatch(this, "__case");

        s->addSymbol(new NonPrimitiveCondExr(this, "?:"));

        s->addSymbol(_noop);
        s->addSymbol(_simpleBlock);
        s->addSymbol(_patternBlock);
        s->addSymbol(_fixedFrameBlock);
        s->addSymbol(_dynamicCast);
        s->addSymbol(_curry);
        s->addSymbol(_dynamicPartialEval);
        s->addSymbol(_dynamicPartialApply);
        s->addSymbol(_returnFromVoidFunction);
        s->addSymbol(_returnFromFunction);
        s->addSymbol(_matchFunction);

        s->addSymbol(_nilType);
        s->addSymbol(_voidType);
        s->addSymbol(_intType);
        s->addSymbol(_int64Type);
        s->addSymbol(_shortType);
        s->addSymbol(_floatType);
        s->addSymbol(_doubleType);
        s->addSymbol(_halfType);
        s->addSymbol(_boolType);
        s->addSymbol(_objectType);
        s->addSymbol(_stringType);
        s->addSymbol(_regexType);
        s->addSymbol(_charType);
        s->addSymbol(_byteType);
        s->addSymbol(_exceptionType);

        _charArrayType = arrayType(_charType, 1, 0);

        //
        //	This is a round-about way of adding the 3 vector types. This
        //	is done here instead of lazily to make sure that the math
        //	module aliases to these types will be valid.
        //

        TypeModifier* vector = new VectorTypeModifier(this);
        s->addSymbol(vector);

        _v4fType = arrayType(_floatType, 1, 4);
        _v3fType = arrayType(_floatType, 1, 3);
        _v2fType = arrayType(_floatType, 1, 2);

        _vec4fType = vector->transform(_v4fType, this);
        _vec3fType = vector->transform(_v3fType, this);
        _vec2fType = vector->transform(_v2fType, this);

        s->addSymbol(_mathModule = new MathModule(this));
        s->addSymbol(_mathUtilModule = new MathUtilModule(this));

        RuntimeModule* rtm = new RuntimeModule(this, "runtime");
        s->addSymbol(rtm);

        _nameType = rtm->findSymbolOfType<OpaqueType>(internName("name"));
        _symbolType = rtm->findSymbolOfType<OpaqueType>(internName("symbol"));
        _typeSymbolType =
            rtm->findSymbolOfType<OpaqueType>(internName("type_symbol"));
        _functionSymbolType =
            rtm->findSymbolOfType<OpaqueType>(internName("function_symbol"));
    }

    MuLangContext::~MuLangContext() {}

    Type* MuLangContext::arrayType(const Type* elementType,
                                   const size_t* dimensions, size_t nDimensions)
    {
        if (nDimensions == 1)
        {
            if (*dimensions == 0)
            {
                DynamicArrayTypeCache::const_iterator i =
                    _dynArrayCache.find(elementType);
                if (i != _dynArrayCache.end())
                    return i->second;
            }
            else
            {
                Fixed1DArrayTypeCache::key_type p(elementType, *dimensions);
                Fixed1DArrayTypeCache::const_iterator i =
                    _fixed1ArrayCache.find(p);
                if (i != _fixed1ArrayCache.end())
                    return i->second;
            }
        }

        string name;
        // name = elementType->fullyQualifiedName().c_str();
        name = elementType->name().c_str();
        if (name.find(' ') != string::npos)
            name = "(" + name + ")";

        name += "[";

        bool dynamicArray = false;
        bool staticArray = false;

        //
        //	This loop will return from the function if a mixed
        //	dynamic/static array definition is encountered.
        //

        for (int i = 0; i < nDimensions; i++)
        {
            if (i > 0)
                name += ",";

            if (dimensions[i] > 0)
            {
                char temp[20];
                sprintf(temp, "%ld", dimensions[i]);
                name += temp;
                staticArray = true;
            }
            else
            {
                dynamicArray = true;
            }
        }

        if (dynamicArray && nDimensions > 1)
        {
            return 0;
        }

        name += "]";

        //
        //  Lookup or create
        //

        Symbol* scope = const_cast<Symbol*>(elementType->scope());

        if (Name n = lookupName(name.c_str()))
        {
            if (dynamicArray)
            {
                // if (DynamicArrayType* t =
                //     globalScope()->findSymbolOfType<DynamicArrayType>(n))
                if (DynamicArrayType* t =
                        scope->findSymbolOfType<DynamicArrayType>(n))
                {
                    return t;
                }
            }
            else
            {
                if (FixedArrayType* t =
                        scope->findSymbolOfType<FixedArrayType>(n))
                {
                    return t;
                }
            }
        }

        PrimaryBit fence(this, false);
        Type* t = 0;

        if (dynamicArray)
        {
            t = new DynamicArrayType(this, name.c_str(), 0, elementType,
                                     nDimensions);

            _dynArrayCache[elementType] = t;
        }
        else
        {
            t = new FixedArrayType(this, name.c_str(), 0, elementType,
                                   dimensions, nDimensions);

            if (nDimensions == 1)
            {
                Fixed1DArrayTypeCache::key_type d(elementType, dimensions[0]);
                _fixed1ArrayCache[d] = t;
            }
        }

        scope->addSymbol(t);
        return t;
    }

    Type* MuLangContext::arrayType(const Type* elementType,
                                   const SizeVector& dimensions)
    {
        return arrayType(elementType, &dimensions.front(), dimensions.size());
    }

    Type* MuLangContext::arrayType(const Type* elementType,
                                   const STLVector<size_t>::Type& dimensions)
    {
        return arrayType(elementType, &dimensions.front(), dimensions.size());
    }

    Type* MuLangContext::arrayType(const Type* elementType, size_t dimensions,
                                   ...)
    {
        va_list ap;
        va_start(ap, dimensions);
        SizeVector d;

        for (int i = 0; i < dimensions; i++)
        {
            d.push_back(va_arg(ap, int));
        }

        va_end(ap);
        return arrayType(elementType, &d.front(), d.size());
    }

    //----------------------------------------------------------------------

    TypedValue MuLangContext::evalText(const char* text, const char* inputName,
                                       Process* p, const ModuleList& modules)
    {
        if (!p)
            p = new Process(this);

        istream* saved_istream = _istream;

        istringstream str(text);
        PushInputStream pis(this, str);
        NodeAssembler assembler(this, p);
        Thread* thread = assembler.thread();

        for (int i = 0; i < modules.size(); i++)
        {
            assembler.pushScope(const_cast<Module*>(modules[i]), false);
        }

        SourceFileScope(this, internName(inputName));

        if (p = Parse(inputName, &assembler))
        {
            _istream = saved_istream;

            if (p->rootNode())
            {
                Value v = p->evaluate(thread);

                if (thread->uncaughtException())
                {
                    if (Object* exc = thread->exception())
                    {
                        throw TypedValue(Value(exc), exc->type());
                    }
                    else
                    {
                        throw TypedValue();
                    }
                }
                else
                {
                    const Type* type = thread->returnValueType();
                    return TypedValue(v, type);
                }
            }
        }
        else
        {
            _istream = saved_istream;
        }

        return TypedValue();
    }

    TypedValue MuLangContext::evalFile(const char* file, Process* p,
                                       const ModuleList& modules)
    {
        bool deleteProcess = false;

        if (!p)
        {
            p = new Process(this);
            deleteProcess = true;
        }

        istream* saved_istream = _istream;
        ifstream infile(UNICODE_C_STR(file));

        if (!infile)
        {
            throw StreamOpenFailureException();
        }

        PushInputStream pis(this, infile);
        NodeAssembler assembler(this, p);
        Thread* thread = assembler.thread();

        for (int i = 0; i < modules.size(); i++)
        {
            assembler.pushScope(const_cast<Module*>(modules[i]), false);
        }

        SourceFileScope(this, internName(file));

        if (p = Parse(file, &assembler))
        {
            _istream = saved_istream;

            if (p->rootNode())
            {
                Value v = p->evaluate(thread);

                if (thread->uncaughtException())
                {
                    if (Object* exc = thread->exception())
                    {
                        throw TypedValue(Value(exc), exc->type());
                    }
                    else
                    {
                        throw TypedValue();
                    }
                }
                else
                {
                    const Type* type = thread->returnValueType();
                    if (deleteProcess)
                        delete p;
                    return TypedValue(v, type);
                }
            }
        }
        else
        {
            _istream = saved_istream;
        }

        if (deleteProcess)
            delete p;
        return TypedValue();
    }

    Type* MuLangContext::parseType(const char* text, Process* p)
    {
        if (!p)
            p = new Process(this);
        _typeParsingMode = true;
        istream* saved_istream = _istream;

        istringstream str(text);
        PushInputStream pis(this, str);
        NodeAssembler assembler(this, p);

        if (p = Parse("internal type parser", &assembler))
        {
            _istream = saved_istream;

            if (_parsedType)
            {
                _typeParsingMode = false;
                return _parsedType;
            }
        }

        _istream = saved_istream;
        _typeParsingMode = false;
        return 0;
    }

    void MuLangContext::parseStream(Process* process, istream& in,
                                    const char* inputName)
    {
        PushInputStream p(this, in);
        NodeAssembler as(this, process);
        Parse(inputName, &as);
    }

    Object* MuLangContext::exceptionObject(Exception& exc)
    {
        ExceptionType::Exception* e =
            new ExceptionType::Exception(exceptionType());
        e->setBackTrace(exc);

        if (exc.what() != 0 && *exc.what() != 0)
        {
            e->string() = exc.what();
        }
        else
        {
            e->string() = typeid(exc).name();
        }

        return e;
    }

} // namespace Mu
