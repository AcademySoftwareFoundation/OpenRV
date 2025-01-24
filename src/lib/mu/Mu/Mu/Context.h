#ifndef __Mu__Context__h__
#define __Mu__Context__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Name.h>
#include <Mu/Signature.h>
#include <Mu/Value.h>
#include <pthread.h>
#include <iosfwd>
#include <vector>
#include <iostream>

//
// Use macro UNICODE_C_STR to convert "const char*" arg to unicode
// versions. i.e. on Windows it returns "const wchar_t*" while
// on linux/osx it returns what was passed in.
// e.g. ifstream mystream(UNICODE_C_STR(filename));
//
// Use macro UNICODE_STR to convert std::string arg to unicode
// string version. i.e. on Windows it returns "std::wstring" while
// on linux/osx it returns what was passed in.
//
#ifndef UNICODE_C_STR
#ifdef _MSC_VER
#define UNICODE_STR(_str) Mu::to_wstring(_str.c_str())
#define UNICODE_C_STR(_cstr) Mu::to_wstring(_cstr).c_str()
#else
#define UNICODE_STR(_str) _str
#define UNICODE_C_STR(_cstr) _cstr
#endif
#endif

namespace Mu
{

    class Type;
    class ListType;
    class Function;
    class Symbol;
    class Process;
    class Signature;
    class Object;
    class FunctionType;
    class ProtoSignature;
    class TupleType;
    class StructType;
    class Module;
    class Exception;

#ifdef _MSC_VER
    std::wstring to_wstring(const char* c);
#endif

    //
    //  class Context
    //
    //  A Context owns a SymbolTable and a hierarchy of symbols. A Process
    //  can be spawned from a Context.
    //
    //  Only one thread should being building a Context at a time. Its ok
    //  to have multiple threads each with building its own context in
    //  parallel, or sharing a single context at runtime between multiple
    //  threads.
    //

    class Context
    {
    public:
        MU_GC_NEW_DELETE

        //
        //  Types
        //

        typedef int Score;
        typedef STLVector<Score>::Type Scores;
        typedef STLVector<size_t>::Type Permutation;
        typedef STLVector<const Type*>::Type TypeVector;
        typedef STLVector<const Function*>::Type FunctionVector;
        typedef Symbol::SymbolVector SymbolVector;
        typedef Symbol::ConstSymbolVector ConstSymbolVector;
        typedef STLVector<const Module*>::Type ModuleList;
        typedef STLMap<const TypeVariable*, const Type*>::Type TypeBindings;
        typedef APIAllocatable::STLVector<Value>::Type ArgumentVector;
        typedef std::pair<std::string, const Type*> NameValuePair;
        typedef STLVector<NameValuePair>::Type NameValuePairs;
        typedef STLVector<Name>::Type NameVector;

        struct ScoredFunction
        {
            ScoredFunction(const Function* f, const TypeBindings& b,
                           const Scores& s)
                : func(f)
                , bindings(b)
                , scores(s)
                , permutation()
                , totalScore(-1)
            {
                computeTotalScore();
            }

            ScoredFunction(const Function* f, const TypeBindings& b,
                           const Scores& s, const Permutation& pt)
                : func(f)
                , bindings(b)
                , scores(s)
                , permutation(pt)
                , totalScore(-1)
            {
                computeTotalScore();
            }

            void computeTotalScore();

            MU_GC_NEW_DELETE

            const Function* func;
            Scores scores;
            Context::TypeBindings bindings;
            Permutation permutation;
            int totalScore;
        };

        struct SourceRecord
        {
            SourceRecord()
                : filename()
                , linenum(0)
                , charnum(0)
            {
            }

            SourceRecord(Name n, unsigned short l, unsigned short c)
                : filename(n)
                , linenum(l)
                , charnum(c)
            {
            }

            Name filename;
            unsigned short linenum;
            unsigned short charnum;
        };

        typedef STLVector<ScoredFunction>::Type ScoredFunctions;
        typedef STLMap<const Symbol*, SourceRecord>::Type SymbolDefinitionMap;
        typedef STLMap<const Node*, SourceRecord>::Type NodeDefinitionMap;
        typedef STLVector<SourceRecord>::Type SourceRecordVector;

        //
        //  Constructors
        //

        Context(const char* imp, const char* name);
        virtual ~Context();

        const Name impName() const { return _impName; }

        const Name name() const { return _name; }

        //
        //	I/O - these must remain valid for the lifetime of this object
        //

        struct PushInputStream
        {
            PushInputStream(Context* c, std::istream& i)
                : context(c)
                , current(&c->inputStream())
            {
                context->setInput(i);
            }

            ~PushInputStream() { context->setInput(*current); }

            Context* context;
            std::istream* current;
        };

        void setInput(std::istream&);
        void setOutput(std::ostream&);
        void setError(std::ostream&);

        std::istream& inputStream() const { return *_istream; }

        std::ostream& outputStream() const { return *_ostream; }

        std::ostream& errorStream() const { return *_estream; }

        //
        //	By default, these functions output ERROR: or WARNING: or INFO:
        //	followed by the formated string to the appropriate output
        //	stream defined above.
        //

        virtual void error(const char* fmt, ...);
        virtual void warning(const char* fmt, ...);
        virtual void output(const char* fmt, ...);

        //
        //  Instances of this object are used on the stack to constrol
        //  marking of symbols as "primary" or not. This is used by the
        //  compiler to determine which symbols were defined in the source
        //  file being compiled.
        //

        void setPrimaryState(bool b) { _primaryState = b; }

        bool primaryState() const { return _primaryState; }

        struct PrimaryBit
        {
            PrimaryBit(Context* c, bool val)
                : _context(c)
                , _previous(c->primaryState())
            {
                c->setPrimaryState(val);
            }

            ~PrimaryBit() { _context->setPrimaryState(_previous); }

        private:
            Context* _context;
            bool _previous;
        };

        friend class PrimaryBit;

        //
        //  Works in conjuection with primary state above
        //

        struct SourceFileScope
        {
            SourceFileScope(Context* c, Name n)
                : context(c)
            {
                c->pushSourceRecordStack();
                c->setSourceFileName(n);
                c->setSourceLine(0);
                c->setSourceChar(0);
            }

            ~SourceFileScope() { context->popSourceRecordStack(); }

            Context* context;
        };

        void setSourceLine(unsigned short i) { _sourceRecord.linenum = i; }

        void setSourceChar(unsigned short i) { _sourceRecord.charnum = i; }

        void setSourceFileName(Name n) { _sourceRecord.filename = n; }

        const SourceRecord& sourceRecord() const { return _sourceRecord; }

        const SourceRecord* sourceRecordOfNode(const Node*) const;
        const SourceRecord* sourceRecordOfSymbol(const Symbol*) const;

        void pushSourceRecordStack()
        {
            _sourceRecordStack.push_back(_sourceRecord);
        }

        void popSourceRecordStack()
        {
            _sourceRecord = _sourceRecordStack.back();
            _sourceRecordStack.pop_back();
        }

        const SymbolDefinitionMap& symbolDefinitions() const
        {
            return _symbolDefinitionMap;
        }

        //
        //  Names
        //

        NamePool& namePool() { return _namePool; }

        NamePool& namePool() const { return _namePool; }

        Name lookupName(const char* n) const { return namePool().find(n); }

        Name internName(const char*);
        Name internName(const String&);
        Name internName(const char*) const;
        Name internName(const String&) const;

        void separateName(Name name, NameVector& names) const;
        static String encodeName(Name n);

        Name uniqueName(const Symbol* scope, const char* prefix);

        //
        //  Basic Evaluate API
        //

        TypedValue evalFunction(Process*, Function*, ArgumentVector&);

        //
        //  Parsing API.
        //
        //  Evaluate source text and return value. Either expect a type or
        //  a real return value.
        //

        virtual TypedValue evalText(const char* text, const char* inputName,
                                    Process*, const ModuleList&) = 0;

        virtual TypedValue evalFile(const char* file, Process*,
                                    const ModuleList&) = 0;

        virtual void parseStream(Process*, std::istream&,
                                 const char* inputName);

        void parseFile(Process*, const char* file);

        void parseStdin(Process*, const char* intputName = 0);

        virtual Type* parseType(const char* text, Process*) = 0;

        //
        //	This is the root namespace
        //

        Symbol* globalScope() { return _globalScope; }

        const Symbol* globalScope() const { return _globalScope; }

        //
        //  Symbol API.
        //
        //  Note that this differs from the API in Symbol in that the
        //  asked for symbol *might* get created in the process.  For that
        //  reason, specializations are needed for Type.
        //

        void findSymbols(QualifiedName n, SymbolVector& v)
        {
            return globalScope()->findSymbols(n, v);
        }

        void findSymbols(QualifiedName n, ConstSymbolVector& v) const
        {
            return globalScope()->findSymbols(n, v);
        }

        Symbol* findSymbolByQualifiedName(QualifiedName n,
                                          bool restricted = true)
        {
            return globalScope()->findSymbolByQualifiedName(n, restricted);
        }

        const Symbol* findSymbolByQualifiedName(QualifiedName n,
                                                bool restricted = true) const
        {
            return globalScope()->findSymbolByQualifiedName(n, restricted);
        }

        template <class T> T* findSymbolOfType(Name);

        template <class T> const T* findSymbolOfType(Name) const;

        template <class T>
        T* findSymbolOfTypeByQualifiedName(Name, bool restricted = true);

        template <class T>
        const T* findSymbolOfTypeByQualifiedName(Name,
                                                 bool restricted = true) const;

        template <class T> SymbolVector findSymbolsOfType(QualifiedName);

        template <class T>
        ConstSymbolVector findSymbolsOfType(QualifiedName) const;

        template <class T>
        const T* findSymbolOfTypeByMangledName(const String&,
                                               QualifiedName) const;

        //
        //  Signature table. If you call an intern function, the context owns
        //  your interned object -- so basically its deleted.
        //

        const Signature* internSignature(Signature*) const;

        SignatureHashTable& signatureHashTable() { return _signatureTable; }

        //
        //  Function types are created based on Signatures
        //

        FunctionType* functionType(const char*);
        FunctionType* functionType(const Signature*);

        const FunctionType* ambiguousFunctionType() const
        {
            return _ambiguousFunctionType;
        }

        //
        //  Find or create a Tuple Type
        //

        TupleType* tupleType(const TypeVector& types);

        //
        //  Find or create a Struct Type
        //

        StructType* structType(Symbol* root, const char* name,
                               const NameValuePairs& fields);

        //
        //  List of T  (like above but for lists)
        //

        ListType* listType(const Type* elementType);

        //
        //  Exception object created from Mu::Exeception class
        //

        virtual Object* exceptionObject(Mu::Exception&) = 0;

        //
        //	Matching functions, both expect a 0 terminated array of Type*
        //	for the args argument. matchFunction will call
        //	bestMatchFunction in order to find a match.
        //
        //  matchSpecializedFunction() is just like calling matchFunction,
        //  but it will create a specialized version of a PolymorphicMatch
        //  if necessary. Because it requires its own NodeAssembler, you
        //  need to pass in a process and thread.
        //
        //	Both functions return the type of match by filling in the
        //	matchType parameter passed in.
        //

        enum MatchType
        {
            NoMatch,
            ExactMatch,       // no ambiguity
            TypeMatch,        // matches if argument order changes
            BestMatch,        // ambiguous: matched according to scoring
            BestPartialMatch, // best match with default arguments
            PolymorphicMatch  // matches a generic function
        };

        virtual const Function* matchFunction(const FunctionVector&,
                                              const TypeVector& args,
                                              TypeBindings&,
                                              MatchType& matchType) const;

        virtual const Function* matchSpecializedFunction(Process*, Thread*,
                                                         const FunctionVector&,
                                                         const TypeVector& args,
                                                         MatchType& matchType);

        void verbose(bool b) { _verbose = b; }

        bool verbose() const { return _verbose; }

        void debugging(bool b) { _debugging = b; }

        bool debugging() const { return _debugging; }

        const Function* specializeFunction(Process*, Thread*,
                                           const TypeBindings&, const Function*,
                                           const TypeVector&);

        //
        //  Symbols
        //

        const Type* nilType() const { return _nilType; }

        const Type* voidType() const { return _voidType; }

        const Type* boolType() const { return _boolType; }

        const Type* matchAnyType() const { return _matchAnyType; }

        const Type* unresolvedType() const { return _unresolvedType; }

        const Type* symbolType() const { return _symbolType; }

        const Type* typeSymbolType() const { return _typeSymbolType; }

        const Type* functionSymbolType() const { return _functionSymbolType; }

        const Function* noop() const { return _noop; }

        const Function* simpleBlock() const { return _simpleBlock; }

        const Function* patternBlock() const { return _patternBlock; }

        const Function* fixedFrameBlock() const { return _fixedFrameBlock; }

        const Function* dynamicCast() const { return _dynamicCast; }

        const Function* curry() const { return _curry; }

        const Function* dynamicPartialApply() const
        {
            return _dynamicPartialApply;
        }

        const Function* dynamicPartialEval() const
        {
            return _dynamicPartialEval;
        }

        const Function* returnFromVoidFunction() const
        {
            return _returnFromVoidFunction;
        }

        const Function* returnFromFunction() const
        {
            return _returnFromFunction;
        }

        const Function* matchFunction() const { return _matchFunction; }

        const Function* assignNonPrimitive() const
        {
            return _assignNonPrimitive;
        }

        const Symbol* unresolvedCall() const { return _unresolvedCall; }

        const Symbol* unresolvedCast() const { return _unresolvedCast; }

        const Symbol* unresolvedConstructor() const
        {
            return _unresolvedConstructor;
        }

        const Symbol* unresolvedReference() const
        {
            return _unresolvedReference;
        }

        const Symbol* unresolvedDereference() const
        {
            return _unresolvedDereference;
        }

        const Symbol* unresolvedStackReference() const
        {
            return _unresolvedStackReference;
        }

        const Symbol* unresolvedStackDereference() const
        {
            return _unresolvedStackDereference;
        }

        const Symbol* unresolvedMemberReference() const
        {
            return _unresolvedMemberReference;
        }

        const Symbol* unresolvedMemberCall() const
        {
            return _unresolvedMemberCall;
        }

        const Symbol* unresolvedDeclaration() const
        {
            return _unresolvedDeclaration;
        }

        const Symbol* unresolvedAssignment() const
        {
            return _unresolvedAssignment;
        }

        static void setShowConstructed(bool);

    private:
        bool scoreArgs(Scores& scores, TypeBindings& bindings,
                       const Function* F, const TypeVector& args) const;

        int score(const Function* F, TypeBindings& bindings, const Type* ftype,
                  const Type* type) const;

        const Function* findCast(const Type* from, const Type* to) const;

        void symbolConstructed(const Symbol*);
        void symbolDestroyed(const Symbol*);

    protected:
        std::istream* _istream;
        std::ostream* _ostream;
        std::ostream* _estream;
        Name _name;
        Name _impName;
        mutable NamePool _namePool;
        Symbol* _globalScope;
        FunctionType* _ambiguousFunctionType;
        bool _verbose;
        bool _debugging;
        bool _primaryState;
        mutable SignatureHashTable _signatureTable;

        Function* _noop;
        Function* _simpleBlock;
        Function* _patternBlock;
        Function* _fixedFrameBlock;
        Function* _dynamicCast;
        Function* _curry;
        Function* _dynamicPartialApply;
        Function* _dynamicPartialEval;
        Function* _returnFromFunction;
        Function* _returnFromVoidFunction;
        Function* _matchFunction;
        Function* _assignNonPrimitive;
        Symbol* _unresolvedCall;
        Symbol* _unresolvedCast;
        Symbol* _unresolvedConstructor;
        Symbol* _unresolvedReference;
        Symbol* _unresolvedDereference;
        Symbol* _unresolvedStackReference;
        Symbol* _unresolvedStackDereference;
        Symbol* _unresolvedMemberReference;
        Symbol* _unresolvedMemberDereference;
        Symbol* _unresolvedMemberCall;
        Symbol* _unresolvedDeclaration;
        Symbol* _unresolvedAssignment;

        Type* _nilType;
        Type* _voidType;
        Type* _boolType;
        Type* _matchAnyType;
        Type* _unresolvedType;
        Type* _symbolType;
        Type* _typeSymbolType;
        Type* _functionSymbolType;

        SymbolDefinitionMap _symbolDefinitionMap;
        NodeDefinitionMap _nodeDefinitionMap;
        SourceRecord _sourceRecord;
        SourceRecordVector _sourceRecordStack;

        pthread_mutex_t _lock;

        size_t _uniqueID;

        friend class Symbol;
    };

    template <class T> T* Context::findSymbolOfType(Name name)
    {
        if (Symbol* s = globalScope()->findSymbol(name))
        {
            for (s = s->firstOverload(); s; s = s->nextOverload())
            {
                if (T* typePointer = dynamic_cast<T*>(s))
                {
                    return typePointer;
                }
            }
        }

        return 0;
    }

    template <class T> const T* Context::findSymbolOfType(Name name) const
    {
        if (const Symbol* symbol = globalScope()->findSymbol(name))
        {
            while (symbol)
            {
                if (const T* typePointer = dynamic_cast<const T*>(symbol))
                {
                    return typePointer;
                }

                symbol = symbol->nextOverload();
            }
        }

        return 0;
    }

    template <class T>
    T* Context::findSymbolOfTypeByQualifiedName(QualifiedName name,
                                                bool restricted)
    {
        if (Symbol* s =
                globalScope()->findSymbolByQualifiedName(name, restricted))
        {
            for (s = s->firstOverload(); s; s = s->nextOverload())
            {
                if (T* typePointer = dynamic_cast<T*>(s))
                {
                    return typePointer;
                }
            }
        }

        return 0;
    }

    template <class T>
    const T* Context::findSymbolOfTypeByQualifiedName(QualifiedName name,
                                                      bool restricted) const
    {
        if (const Symbol* symbol =
                globalScope()->findSymbolByQualifiedName(name, restricted))
        {
            while (symbol)
            {
                if (const T* typePointer = dynamic_cast<const T*>(symbol))
                {
                    return typePointer;
                }

                symbol = symbol->nextOverload();
            }
        }

        return 0;
    }

    template <class T>
    Context::SymbolVector Context::findSymbolsOfType(QualifiedName name)
    {
        SymbolVector s;
        findSymbols(name, s);
        s.erase(
            std::remove_if(s.begin(), s.end(), stl_ext::IsNotA_p<Symbol, T>()),
            s.end());
        return s;
    }

    template <class T>
    Context::ConstSymbolVector
    Context::findSymbolsOfType(QualifiedName name) const
    {
        ConstSymbolVector s;
        findSymbols(name, s);
        s.erase(std::remove_if(s.begin(), s.end(),
                               stl_ext::IsNotA_p<const Symbol, const T>()),
                s.end());
        return s;
    }

    template <class T>
    const T*
    Context::findSymbolOfTypeByMangledName(const Mu::String& mangledName,
                                           QualifiedName name) const
    {
        ConstSymbolVector s = findSymbolsOfType<T>(name);

        for (int i = 0; i < s.size(); i++)
        {
            if (s[i]->mangledName() == mangledName)
                return static_cast<const T*>(s[i]);
        }

        return 0;
    }

} // namespace Mu

#endif // __Mu__Context__h__
