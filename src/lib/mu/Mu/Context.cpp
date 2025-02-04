//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/BaseFunctions.h>
#include <Mu/Class.h>
#include <Mu/Context.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/FunctionSpecializer.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/Interface.h>
#include <Mu/Language.h>
#include <Mu/ListType.h>
#include <Mu/Module.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/StructType.h>
#include <Mu/Symbol.h>
#include <Mu/TupleType.h>
#include <Mu/Type.h>
#include <Mu/TypePattern.h>
#include <Mu/Unresolved.h>
#include <Mu/config.h>
#include <Mu/UTF8.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdarg.h>
#include <stdio.h>
#include <stl_ext/string_algo.h>
#include <string.h>
#include <vector>
#include <sstream>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <codecvt>
#include <wchar.h>
#endif

namespace Mu
{
    using namespace std;

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
#ifdef _MSC_VER
    std::wstring to_wstring(const char* c)
    {
        wstring_convert<codecvt_utf8<wchar_t>> wstr;
        return wstr.from_bytes(c);
    }
#endif

    Context::Context(const char* impName, const char* name)
        : _primaryState(false)
        , _uniqueID(0)
    {
        pthread_mutex_init(&_lock, 0);

        GarbageCollector::init();
        PrimaryBit fence(this, false);
        USING_MU_FUNCTION_SYMBOLS;

        _name = internName(name);
        _impName = internName(impName);
        _globalScope = new Module(this, "");
        _istream = &cin;
        _ostream = &cout;
        _estream = &cerr;
        _ambiguousFunctionType = new FunctionType(this, "(;)");
        _unresolvedCall = new UnresolvedCall(this);
        _unresolvedCast = new UnresolvedCast(this);
        _unresolvedConstructor = new UnresolvedConstructor(this);
        _unresolvedReference = new UnresolvedReference(this);
        _unresolvedDereference = new UnresolvedDereference(this);
        _unresolvedStackReference = new UnresolvedStackReference(this);
        _unresolvedStackDereference = new UnresolvedStackDereference(this);
        _unresolvedMemberReference = new UnresolvedMemberReference(this);
        _unresolvedMemberCall = new UnresolvedMemberCall(this);
        _unresolvedType = new UnresolvedType(this);
        _unresolvedDeclaration = new UnresolvedDeclaration(this);
        _unresolvedAssignment = new UnresolvedAssignment(this);
        _assignNonPrimitive = new AssignAsReference(this);
        _verbose = false;
        _debugging = false;

        _globalScope->addSymbol(_unresolvedCall);
        _globalScope->addSymbol(_unresolvedCast);
        _globalScope->addSymbol(_unresolvedConstructor);
        _globalScope->addSymbol(_unresolvedReference);
        _globalScope->addSymbol(_unresolvedStackReference);
        _globalScope->addSymbol(_unresolvedStackDereference);
        _globalScope->addSymbol(_unresolvedMemberReference);
        _globalScope->addSymbol(_unresolvedMemberCall);
        _globalScope->addSymbol(_unresolvedDeclaration);
        _globalScope->addSymbol(_unresolvedAssignment);
        _globalScope->addSymbol(_ambiguousFunctionType);
        _globalScope->addSymbol(_assignNonPrimitive);
    }

    Context::~Context()
    {
        pthread_mutex_destroy(&_lock);
        delete _globalScope;
    }

    void Context::setInput(istream& stream) { _istream = &stream; }

    void Context::setOutput(ostream& stream) { _ostream = &stream; }

    void Context::setError(ostream& stream) { _estream = &stream; }

    void Context::error(const char* fmt, ...)
    {
        char p[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(p, 1024, fmt, ap);
        errorStream() << "ERROR: " << p;
        va_end(ap);
    }

    void Context::warning(const char* fmt, ...)
    {
        char p[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(p, 1024, fmt, ap);
        errorStream() << "WARNING: " << p;
        va_end(ap);
    }

    void Context::output(const char* fmt, ...)
    {
        char p[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(p, 1024, fmt, ap);
        outputStream() << "INFO: " << p;
        va_end(ap);
    }

    const Signature* Context::internSignature(Signature* s) const
    {
        if (const SignatureHashTable::Item* i = _signatureTable.find(s))
        {
            delete s;
            return i->data();
        }
        else
        {
            if (!s->resolved())
            {
                s->resolve(this);

                if (!s->resolved())
                {
                    throw UnresolvedSignatureException();
                }
            }

            return _signatureTable.add(s)->data();
        }
    }

    FunctionType* Context::functionType(const char* name)
    {
        Signature* sig = new Signature();
        vector<string> tokens;
        stl_ext::tokenize(tokens, name, "(,;) \t\r\n");

        for (int i = 0; i < tokens.size(); i++)
        {
            if (Name n = lookupName(tokens[i].c_str()))
            {
                sig->push_back(n);
            }
            else
            {
                throw InconsistantSignatureException();
            }
        }

        sig->resolve(this);
        return functionType(internSignature(sig));
    }

    FunctionType* Context::functionType(const Signature* s)
    {
        if (s->resolved())
        {
            String name = s->functionTypeName();

            if (Name n = lookupName(name.c_str()))
            {
                if (FunctionType* ftype =
                        globalScope()->findSymbolOfType<FunctionType>(n))
                {
                    return ftype;
                }
            }

            FunctionType* type = new FunctionType(this, name.c_str(), s);
            globalScope()->addSymbol(type);
            return type;
        }
        else
        {
            return 0;
        }
    }

    //----------------------------------------------------------------------

    bool operator<(const Context::ScoredFunction& a,
                   const Context::ScoredFunction& b)
    {
        return a.totalScore > b.totalScore;
    }

    void Context::ScoredFunction::computeTotalScore()
    {
        int sum = accumulate(scores.begin(), scores.end(), 0);
        int n = count(scores.begin(), scores.end(), 0);
        totalScore = sum + (scores.size() - n);
    }

    const Function* Context::findCast(const Type* from, const Type* to) const
    {
        for (const Symbol* s = to->firstOverload(); s; s = s->nextOverload())
        {
            if (const Function* f = dynamic_cast<const Function*>(s))
            {
                if (f->isCast() && f->argType(0) == from)
                {
                    return f;
                }
            }
        }

        if (const Class* fromClass = dynamic_cast<const Class*>(from))
        {
            if (const Class* toClass = dynamic_cast<const Class*>(to))
            {
                if (fromClass->isA(toClass) || toClass->isA(fromClass))
                {
                    return dynamicCast();
                }
            }
            else if (const Interface* toInter =
                         dynamic_cast<const Interface*>(to))
            {
                return dynamicCast();
            }
        }

        return 0;
    }

    int Context::score(const Function* F, TypeBindings& bindings,
                       const Type* ftype, const Type* type) const
    {
        if (!type || !ftype || ftype == type)
            return 0;

        if (ftype->isTypePattern())
        {
            return ftype->match(type, bindings) == Type::Match ? 4 : -1;
        }

        if (type->isTypePattern())
        {
            return type->match(ftype, bindings) == Type::Match ? 4 : -1;
        }

        if (ftype->match(type, bindings) == Type::Match)
        {
            return 1;
        }

        if (const Function* f = findCast(type, ftype))
        {
            return f->isLossy() ? 2 : 1;
        }

        return -1;
    }

    bool Context::scoreArgs(Scores& scores, TypeBindings& bindings,
                            const Function* F, const TypeVector& args) const
    {
        scores.resize(F->numArgs());

        if (F->isVariadic())
        {
            //
            //  For variadic functions, loop over the given arguments and
            //  allow the typePatterns to potentially loop back until a
            //  match occurs.
            //

            for (int i = 0, j = 0; i < args.size(); i++, j++)
            {
                const Type* givenType = args[i];
                const Type* funcType = F->argType(j);

                int s = score(F, bindings, funcType, givenType);
                if (s < 0)
                    return false;
                if (i < F->numArgs())
                    scores[i] = s;

                if (funcType->isTypePattern())
                {
                    const TypePattern* ptype =
                        static_cast<const TypePattern*>(funcType);
                    ptype->argumentAdjust(i, j);
                }
            }
        }
        else
        {
            //
            //  Non-variadic functions loop over the function parameters
            //  and match them to the given arguments.
            //

            for (int i = 0; i < F->numArgs(); i++)
            {
                if (i >= args.size())
                {
                    scores[i] = 0;
                }
                else
                {
                    const Type* givenType = args[i];
                    const Type* funcType = F->argType(i);

                    int s = score(F, bindings, funcType, givenType);
                    if (s < 0)
                        return false;
                    scores[i] = s;
                }
            }
        }

        return true;
    }

    const Function* Context::matchFunction(const FunctionVector& functions,
                                           const TypeVector& argTypes,
                                           TypeBindings& bindings,
                                           MatchType& matchType) const
    {
        ScoredFunctions possibles;
        Scores scores;

        for (int i = 0; i < functions.size(); i++)
        {
            const Function* F = functions[i];

            if (F->symbolState() != Symbol::ResolvedState)
            {
                F->resolve();
                if (F->symbolState() != Symbol::ResolvedState)
                    continue;
            }

            int numArgs = F->numArgs();
            int minArgs = F->minimumArgs();
            int maxArgs = F->maximumArgs();
            int reqArgs = F->requiredArgs();

            if (argTypes.size() < minArgs || argTypes.size() > maxArgs)
                continue;

            TypeBindings bindings;

            if (numArgs == 0)
            {
                scores.clear();
                possibles.push_back(ScoredFunction(F, bindings, scores));
            }
            else if (scoreArgs(scores, bindings, F, argTypes))
            {
                possibles.push_back(ScoredFunction(F, bindings, scores));
            }
            else if (F->isCommutative())
            {
                //
                //  Only deals with 2 arg permutation
                //

                if (numArgs == 2)
                {
                    bindings.clear();
                    TypeVector flippedArgs = argTypes;
                    swap(flippedArgs.front(), flippedArgs.back());

                    if (scoreArgs(scores, bindings, F, flippedArgs))
                    {
                        Permutation p(2);
                        p[0] = 1;
                        p[1] = 0;

                        possibles.push_back(
                            ScoredFunction(F, bindings, scores, p));
                    }
                }
            }
        }

        if (possibles.empty())
            return 0;

        //
        //  Reverse the possibles array. This is necessary becasue the
        //  order of the incoming functions matters. In the case of
        //  multiple or even single inheritance the functions will be
        //  order of "distance" from the object class. So identical
        //  signature functions in the derived class will be before the
        //  base classes. Since we're sorting to get the result as the
        //  last in the array reverse the order so that the "closest"
        //  function appears last in the last.
        //

        reverse(possibles.begin(), possibles.end());
        stable_sort(possibles.begin(), possibles.end());

        //
        //  Check here for functions with the same name that have the same
        //  args but differ in return type only. This is ambiguous in Mu
        //  since we don't allow overloading on return type (at least not
        //  right now).
        //

        // check here!

        if (_verbose)
        {
            cout << ">>> MU: Choices in order for "
                 << possibles.back().func->fullyQualifiedName() << " (";

            for (int i = 0; i < argTypes.size(); i++)
            {
                if (i)
                    cout << ", ";
                cout << argTypes[i]->fullyQualifiedName();
            }

            cout << ")" << endl;

            for (int i = possibles.size(); i--;)
            {
                ScoredFunction& f = possibles[i];

                cout << ">>> Mu:    ";
                f.func->output(cout);
                cout << endl << ">>> Mu:      " << f.totalScore << " = ( ";

                copy(f.scores.begin(), f.scores.end(),
                     ostream_iterator<int>(cout, " "));

                cout << ")";

                if (f.permutation.size())
                {
                    cout << "   [ ";
                    copy(f.permutation.begin(), f.permutation.end(),
                         ostream_iterator<int>(cout, " "));

                    cout << "]";
                }

                cout << endl;
            }
        }

        ScoredFunction& choice = possibles.back();
        const Function* F = choice.func;

        if (choice.func->isPolymorphic())
        {
            matchType = PolymorphicMatch;
            bindings = choice.bindings;
            return choice.func;
        }

        if (choice.totalScore == 0)
        {
            if (F->numArgs() != argTypes.size())
                matchType = BestPartialMatch;
            else if (choice.permutation.empty())
                matchType = ExactMatch;
            else
                matchType = TypeMatch;
        }
        else if (F->numArgs() != argTypes.size())
            matchType = BestPartialMatch;
        else if (choice.permutation.empty())
            matchType = BestMatch;
        else
            matchType = TypeMatch;

        return choice.func;
    }

    const Function* Context::matchSpecializedFunction(
        Process* p, Thread* thread, const FunctionVector& fvec,
        const TypeVector& args, MatchType& matchType)
    {
        TypeBindings bindings;
        const Function* F = matchFunction(fvec, args, bindings, matchType);

        if (matchType == PolymorphicMatch)
        {
            matchType = ExactMatch;
            return specializeFunction(p, thread, bindings, F, args);
        }
        else
        {
            return F;
        }
    }

    //----------------------------------------------------------------------

    //
    //  specialize the function F for the given parameter types.
    //

    const Function* Context::specializeFunction(Process* p, Thread* thread,
                                                const TypeBindings& bindings,
                                                const Function* F,
                                                const TypeVector& types)
    {
#if 0
    for (TypeMap::const_iterator i = typeMap.begin();
         i != typeMap.end();
         ++i)
    {
        cout << (*i).first->fullyQualifiedName()
             << " => "
             << (*i).second->fullyQualifiedName()
             << endl;
    }
#endif

        FunctionSpecializer evaluator(F, p, thread);

        if (Function* S = evaluator.specialize(bindings))
        {
            return S;
        }
        else
        {
            return F;
        }
    }

    //----------------------------------------------------------------------
    TypedValue Context::evalFunction(Process* p, Function* F,
                                     ArgumentVector& args)
    {
        Thread* t = p->newApplicationThread();
        TypedValue v(t->call(F, args, true), F->returnType());
        p->releaseApplicationThread(t);
        return v;
    }

    //----------------------------------------------------------------------

    void Context::parseFile(Process* process, const char* filename)
    {
        ifstream in(UNICODE_C_STR(filename));

        if (in)
        {
            parseStream(process, in, filename);
        }
        else
        {
            throw FileOpenErrorException();
        }
    }

    void Context::parseStdin(Process* process, const char* inputName)
    {
        if (!inputName)
            inputName = "Standard Input";
        parseStream(process, cin, inputName);
    }

    void Context::parseStream(Process*, istream&, const char* inputName)
    {
        abort();
    }

    //----------------------------------------------------------------------

    TupleType* Context::tupleType(const TypeVector& types)
    {
        PrimaryBit fence(this, false);
        String tupleName("(");

        for (int i = 0; i < types.size(); i++)
        {
            if (i)
                tupleName += ",";
            tupleName += types[i]->fullyQualifiedName().c_str();
        }

        tupleName += ")";
        Name name = internName(tupleName);

        if (TupleType* type = findSymbolOfType<TupleType>(name))
        {
            return type;
        }
        else
        {
            TupleType* tt = new TupleType(this, tupleName.c_str(), types);
            globalScope()->addSymbol(tt);
            return tt;
        }
    }

    //----------------------------------------------------------------------

    StructType* Context::structType(Symbol* root, const char* n,
                                    const NameValuePairs& fields)
    {
        PrimaryBit fence(this, false);
        Name name = internName(n);

        if (!root)
            root = globalScope();

        if (StructType* type = root->findSymbolOfType<StructType>(name))
        {
            return type;
        }
        else
        {
            StructType* t = new StructType(this, n, fields);
            root->addSymbol(t);
            return t;
        }
    }

    //----------------------------------------------------------------------

    ListType* Context::listType(const Type* elementType)
    {
        PrimaryBit fence(this, false);
        String name = "[";
        name += elementType->fullyQualifiedName().c_str();
        name += "]";

        if (Name n = lookupName(name.c_str()))
        {
            if (ListType* t = globalScope()->findSymbolOfType<ListType>(n))
            {
                return t;
            }
        }

        ListType* t = new ListType(this, name.c_str(), elementType);
        globalScope()->addSymbol(t);
        return t;
    }

    //----------------------------------------------------------------------

    Name Context::internName(const char* n) { return namePool().intern(n); }

    Name Context::internName(const String& n) { return namePool().intern(n); }

    Name Context::internName(const char* n) const
    {
        return namePool().intern(n);
    }

    Name Context::internName(const String& n) const
    {
        return namePool().intern(n);
    }

    void Context::separateName(Name name, NameVector& names) const
    {
        STLVector<String>::Type tokens;

        const Symbol* s = globalScope()->findSymbol(name);

        if (s && s->scope() == globalScope())
        {
            names.push_back(name);
        }
        else
        {
            string n = name;

            UTF8tokenize(tokens, name, ".");
            names.resize(tokens.size());

            for (int i = 0; i < tokens.size(); i++)
            {
                names[i] = internName(tokens[i]);
            }
        }
    }

    String Context::encodeName(Name n)
    {
        const String& s = n;
        String o;

        for (int i = 0; i < s.size(); i++)
        {
            switch (s[i])
            {
            case ' ':
                o += "_";
                break;
            case '.':
                o += "__";
                break;
            case '~':
                o += "Tilde_";
                break;
            case '`':
                o += "Tick_";
                break;
            case '+':
                o += "Plus_";
                break;
            case '-':
                o += "Minus_";
                break;
            case '!':
                o += "Bang_";
                break;
            case '%':
                o += "PCent_";
                break;
            case '@':
                o += "At_";
                break;
            case '#':
                o += "Pound_";
                break;
            case '$':
                o += "Dollar_";
                break;
            case '^':
                o += "Caret_";
                break;
            case '&':
                o += "Amp_";
                break;
            case '*':
                o += "Star_";
                break;
            case '(':
                o += "BParen_";
                break;
            case ')':
                o += "EParen_";
                break;
            case '=':
                o += "EQ_";
                break;
            case '{':
                o += "BCB_";
                break;
            case '}':
                o += "ECB_";
                break;
            case '[':
                o += "BSB_";
                break;
            case ']':
                o += "ESB_";
                break;
            case '|':
                o += "Pipe_";
                break;
            case '\\':
                o += "BSlash_";
                break;
            case ':':
                o += "Colon_";
                break;
            case ';':
                o += "SColon_";
                break;
            case '"':
                o += "Quote_";
                break;
            case '\'':
                o += "SQuote_";
                break;
            case '<':
                o += "LT_";
                break;
            case '>':
                o += "GT_";
                break;
            case ',':
                o += "Comma_";
                break;
            case '?':
                o += "QMark_";
                break;
            case '/':
                o += "Slash_";
                break;

            default:
                o += s[i];
                break;
            }
        }

        return o;
    }

    Name Context::uniqueName(const Symbol* scope, const char* prefix)
    {
        bool found = true;
        Name name;

        while (found)
        {
            ostringstream str;
            _uniqueID++;
            str << prefix << hex << _uniqueID;
            string s = str.str();
            const char* n = s.c_str();
            found = _namePool.exists(n);
            if (!found)
                name = internName(n);
        }

        return name;
    }

    const Context::SourceRecord*
    Context::sourceRecordOfNode(const Node* n) const
    {
        NodeDefinitionMap::const_iterator i = _nodeDefinitionMap.find(n);

        if (i != _nodeDefinitionMap.end())
        {
            return &(*i).second;
        }

        return 0;
    }

    const Context::SourceRecord*
    Context::sourceRecordOfSymbol(const Symbol* s) const
    {
        SymbolDefinitionMap::const_iterator i = _symbolDefinitionMap.find(s);

        if (i != _symbolDefinitionMap.end())
        {
            return &(*i).second;
        }

        return 0;
    }

    static bool showConstructed = false;

    void Context::setShowConstructed(bool b) { showConstructed = b; }

    void Context::symbolConstructed(const Symbol* s)
    {
        if ((_debugging && s && _primaryState) || showConstructed)
        {
            _symbolDefinitionMap[s] = _sourceRecord;

#if 0
        if (showConstructed)
        {
            cout << "DEFINED: ";
            s->output(cout);
            cout << endl;
            cout << "AT: " << _sourceRecord.filename 
                 << ", line " << _sourceRecord.linenum 
                 << ", char "<< _sourceRecord.charnum 
                 << endl;
        }
#endif
        }
    }

    void Context::symbolDestroyed(const Symbol* s)
    {
        _symbolDefinitionMap.erase(s);
    }

} // namespace Mu
