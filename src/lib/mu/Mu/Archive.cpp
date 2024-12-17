//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Alias.h>
#include <Mu/Archive.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Exception.h>
#include <Mu/FreeVariable.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/GlobalVariable.h>
#include <Mu/ListType.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/Namespace.h>
#include <Mu/NodeAssembler.h>
#include <Mu/NodePatch.h>
#include <Mu/NodePrinter.h>
#include <Mu/ParameterVariable.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/TupleType.h>
#include <Mu/VariantInstance.h>
#include <Mu/VariantTagType.h>
#include <Mu/VariantType.h>
#include <algorithm>
#include <assert.h>

namespace Mu
{
    namespace Archive
    {
        using namespace std;

        U32 fileVersionNumber() { return U32(1); }

        U32 magicNumber() { return 0xa2b10e1f; }

        //----------------------------------------------------------------------

        static bool CompareSymbolType(const Symbol* a, const Symbol* b)
        {
            const Module* ma = dynamic_cast<const Module*>(a);
            const Module* mb = dynamic_cast<const Module*>(b);

            if (ma && !mb)
                return true;
            else if (mb && !ma)
                return false;
            else if (ma && mb)
                return a->fullyQualifiedName() < b->fullyQualifiedName();

            const Namespace* na = dynamic_cast<const Namespace*>(a);
            const Namespace* nb = dynamic_cast<const Namespace*>(b);

            if (na && !nb)
                return true;
            else if (nb && !na)
                return false;
            else if (na && nb)
                return a->fullyQualifiedName() < b->fullyQualifiedName();

            const Class* ca = dynamic_cast<const Class*>(a);

            const Class* cb = dynamic_cast<const Class*>(b);

            if (ca && !cb)
                return true;
            else if (cb && !ca)
                return false;
            else if (ca && cb)
            {
                if (ca->isA(cb))
                    return false;
                else if (cb->isA(ca))
                    return true;
                else
                    return ca->fullyQualifiedName() < cb->fullyQualifiedName();
            }

            const Type* ta = dynamic_cast<const Type*>(a);
            const Type* tb = dynamic_cast<const Type*>(b);

            if (ta && !tb)
                return true;
            else if (tb && !ta)
                return false;

            return a->fullyQualifiedName() < b->fullyQualifiedName();
        }

        void Writer::NodeSymbolCollector::preOrderVisit(Node* n, int depth)
        {
            const Symbol* s = n->symbol();

            if (const Type* t = dynamic_cast<const Type*>(s))
            {
                //
                //  term is a constant
                //

                _writer->internType(t);

                if (!t->isPrimitiveType())
                {
                    const DataNode* dn = static_cast<const DataNode*>(n);
                    _writer->add((const Object*)dn->_data._Pointer);
                }
            }
            else if (const Variable* v = dynamic_cast<const Variable*>(s))
            {
                //
                //  term is a variable reference
                //

                _writer->internType(v->storageClass());
                _writer->internNames(v);
            }
            else if (const Function* f = dynamic_cast<const Function*>(s))
            {
                //
                //  term is a function call
                //

                _writer->internAnnotation(n);
                _writer->internFunction(f);

                for (const Symbol* scope = f->scope(); scope;
                     scope = scope->scope())
                {
                    if (const Module* m = dynamic_cast<const Module*>(scope))
                    {
                        _writer->addModuleRequirement(m);
                    }
                }
            }
        }

        //----------------------------------------------------------------------

        Writer::Writer(Process* p, Context* c)
            : _process(p)
            , _context(c)
            , _frozen(false)
            , _barrier()
            , _debugOutput(false)
            , _currentScope(0)
            , _annLine(0)
            , _annChar(0)
            , _annotationOutput(false)
        {
        }

        Writer::~Writer() {}

        void Writer::internAnnotation(const Node* n)
        {
            if (_annotationOutput && context()->debugging())
            {
                const AnnotatedNode* an = static_cast<const AnnotatedNode*>(n);
                internName(an->sourceFileName());
            }
        }

        void Writer::collectSymbolsFromFile(Name filename, SymbolVector& array,
                                            bool userOnly) const
        {
            const Context::SymbolDefinitionMap& symdefs =
                _context->symbolDefinitions();

            for (Context::SymbolDefinitionMap::const_iterator i =
                     symdefs.begin();
                 i != symdefs.end(); ++i)
            {
                if ((*i).second.filename == filename)
                {
                    const Symbol* s = (*i).first;
                    array.push_back(s);
                }
            }
        }

        void Writer::collectPrimarySymbols(const Symbol* s,
                                           SymbolVector& array) const
        {
            if (s->isPrimary())
            {
                bool ok = true;

                if (const Function* f = dynamic_cast<const Function*>(s))
                {
                    ok = !f->isGenerated();
                }

                if (ok)
                    array.push_back(s);
            }

            if (s->symbolTable())
            {
                for (SymbolTable::Iterator it(s->symbolTable()); it; ++it)
                {
                    for (const Symbol* ss = *it; ss; ss = ss->nextOverload())
                    {
                        collectPrimarySymbols(ss, array);
                    }
                }
            }
        }

        void Writer::collectAllPrimarySymbols(SymbolVector& array) const
        {
            collectPrimarySymbols(_context->globalScope(), array);
        }

        static void addNameToVector(Names& names, const Symbol* s,
                                    bool fully = false)
        {
            Name n = s->fullyQualifiedName();
            if (names.find(n) == names.end())
                names.insert(n);

            if (!fully)
            {
                n = s->name();
                if (names.find(n) == names.end())
                    names.insert(n);
            }
        }

        void Writer::collectNames(const SymbolVector& symbols,
                                  Names& names) const
        {
            for (int i = 0; i < symbols.size(); i++)
            {
                const Symbol* s = symbols[i];

                addNameToVector(names, s);
                Symbol::ConstSymbolVector deps;
                s->symbolDependancies(deps);

                for (int q = 0; q < deps.size(); q++)
                {
                    addNameToVector(names, deps[q], true);
                }
            }
        }

        void Writer::internName(Name n) { _nameMap[n.c_str()] = -1; }

        void Writer::internModule(const Module* t)
        {
            _moduleMap[t] = -1;
            internNames(t);
        }

        void Writer::internFunction(const Function* f)
        {
            if (f->isGenerated())
                return;

            if (_functionMap.count(f) > 0)
                return;
            _functionMap[f] = _functionMap.size() + 1;
            internNames(f);

            if (f->hasParameters())
            {
                for (size_t i = 0; i < f->numArgs() + f->numFreeVariables();
                     i++)
                {
                    internType(f->parameter(i)->storageClass());
                }
            }
            else
            {
                for (size_t i = 0; i < f->numArgs() + f->numFreeVariables();
                     i++)
                {
                    internType(f->argType(i));
                }
            }

            internType(f->returnType());
        }

        void Writer::internNames(const Symbol* s)
        {
            internName(s->name());
            internName(s->fullyQualifiedName());
        }

        void Writer::internType(const Type* t)
        {
            for (const Symbol* s = t->scope(); s; s = s->scope())
            {
                if (const Module* m = dynamic_cast<const Module*>(s))
                {
                    addModuleRequirement(m);
                }
            }

            if (!t->isTypePattern())
            {
                if (t->isSerializable())
                    _typeMap[t] = -1;
                internNames(t);
            }
        }

        void Writer::freeze()
        {
            if (_frozen)
                return;

            //
            //  construct the root symbol set
            //

            for (SymbolSet::const_iterator s = _primarySymbolSet.begin();
                 s != _primarySymbolSet.end(); ++s)
            {
                const Symbol* top = *s;

                for (const Symbol* p = (*s)->scope(); p; p = p->scope())
                {
                    if (_primarySymbolSet.count(p) > 0)
                        top = p;
                }

                _rootSymbolSet.insert(top);
                internName(top->scope()->fullyQualifiedName());
            }

            IDNumber count = 1;

            for (ObjectMap::iterator i = _objects.begin(); i != _objects.end();
                 ++i)
            {
                i->second = count++;
            }

            count = 0;

            for (ModuleMap::iterator i = _moduleMap.begin();
                 i != _moduleMap.end(); ++i)
            {
                i->second = count++;
            }

            count = 0;

            for (TypeMap::iterator i = _typeMap.begin(); i != _typeMap.end();
                 ++i)
            {
                i->second = count++;
            }

            count = 0;

            for (NameMap::iterator i = _nameMap.begin(); i != _nameMap.end();
                 ++i)
            {
                i->second = count++;
            }

            _frozen = true;
        }

        void Writer::add(const SymbolVector& symbols)
        {
            for (int i = 0; i < symbols.size(); i++)
                add(symbols[i]);
        }

        void Writer::add(const Names& names)
        {
            for (Names::const_iterator i = names.begin(); i != names.end(); ++i)
            {
                internName(*i);
            }
        }

        void Writer::add(const Object* o)
        {
            assert(!_frozen);

            if (_rootObjects.find(o) == _rootObjects.end())
            {
                _rootObjects.insert(o);
                collect(o);
            }
        }

        void Writer::add(const Symbol* s)
        {
            _primarySymbols.push_back(s);
            _primarySymbolSet.insert(s);

            if (const Function* f = dynamic_cast<const Function*>(s))
            {
                _functions.push_back(f);
                internFunction(f);
            }
            else if (const Type* t = dynamic_cast<const Type*>(s))
            {
                _types.push_back(t);

                if (const Class* c = dynamic_cast<const Class*>(t))
                {
                    const Class::ClassVector& supers = c->superClasses();

                    for (size_t i = 0; i < supers.size(); i++)
                    {
                        internName(supers[i]->fullyQualifiedName());
                    }
                }
            }
            else if (const Variable* v = dynamic_cast<const Variable*>(s))
            {
                if (const ParameterVariable* p =
                        dynamic_cast<const ParameterVariable*>(v))
                {
                    if (p->hasDefaultValue()
                        && !p->storageClass()->isPrimitiveType())
                    {
                        add((const Object*)(p->defaultValue()._Pointer));
                    }
                }

                _variables.push_back(v);
            }
            else if (const Module* m = dynamic_cast<const Module*>(s))
            {
                _modules.push_back(m);

                if (_requiredModules.count(m) > 0)
                {
                    _requiredModules.erase(m);
                }
            }
            else if (const Alias* a = dynamic_cast<const Alias*>(s))
            {
                _aliases.push_back(a);

                if (const Type* t = dynamic_cast<const Type*>(a->alias()))
                {
                    internType(t);
                }

                internNames(a);
                internNames(a->alias());
            }
            else if (const SymbolicConstant* c =
                         dynamic_cast<const SymbolicConstant*>(s))
            {
                internNames(c);
                internType(c->type());

                if (!c->type()->isPrimitiveType())
                {
                    add((const Object*)(c->value()._Pointer));
                }
            }

            collectRecursive(s);
        }

        void Writer::addModuleRequirement(const Module* m)
        {
            //
            //  Add it if its not the global scope and if the module isn't one
            //  we're going to be writing.
            //

            if (m != _context->globalScope())
            /*&& find(_modules.begin(), _modules.end(), m) == _modules.end())*/
            {
                _requiredModules.insert(m);
                internModule(m);
            }
        }

        IDNumber Writer::objectId(const Object* o) const
        {
            if (!o)
                return 0;
            ObjectMap::const_iterator i = _objects.find(o);

            if (i != _objects.end())
            {
                return i->second;
            }
            else
            {
                abort();
                return 0;
            }
        }

        void Writer::collect(const Object* o)
        {
            if (!o || _objects.find(o) != _objects.end())
                return;

            if (!o->type()->isSerializable())
                throw UnarchivableObjectException();

            if (const FunctionType* ftype =
                    dynamic_cast<const FunctionType*>(o->type()))
            {
                const FunctionObject* fo =
                    static_cast<const FunctionObject*>(o);

                if (fo->function()->native())
                {
                    internFunction(fo->function());
                }
                else
                {
                    collectRecursive(fo->function());
                }

                if (fo->dependent())
                    collect(fo->dependent());
            }

            if (_debugOutput)
            {
                cout << ":: collect ";
                o->type()->outputValue(cout, &o);
                cout << endl;
            }

            _objects[o] = -1;
            internType(o->type());

            const Type* type = o->type();
            const Type* ftype = 0;

            for (int i = 0; ftype = type->fieldType(i); i++)
            {
                //
                //  The fieldPointer may return 0 (end-of-fields)
                //

                if (dynamic_cast<const VariantTagType*>(type))
                {
                    if (const ValuePointer p = type->fieldPointer(o, i))
                    {
                        if (!ftype->isPrimitiveType())
                        {
                            collect(reinterpret_cast<const Object*>(p));
                        }
                    }
                }
                else if (const ValuePointer p = type->fieldPointer(o, i))
                {
                    if (!ftype->isPrimitiveType())
                    {
                        collect(*reinterpret_cast<const Object**>(p));
                    }
                }
                else
                {
                    break;
                }
            }
        }

        void Writer::collectRecursive(const Symbol* s)
        {
            internNames(s);

            if (s->symbolTable())
            {
                for (SymbolTable::Iterator i(s->symbolTable()); i; ++i)
                {
                    collectRecursive(*i);
                }
            }

            if (const Variable* v = dynamic_cast<const Variable*>(s))
            {
                internType(v->storageClass());
            }
            else if (const Function* f = dynamic_cast<const Function*>(s))
            {
                const Signature* S = f->signature();

                for (int i = 0; i < S->size(); i++)
                {
                    const Type* T = static_cast<const Type*>((*S)[i].symbol);
                    internType(T);
                }

                if (f->body())
                    NodeSymbolCollector(f->body(), this);
            }
        }

        void Writer::writeObjectId(ostream& o, const Object* obj)
        {
            IDNumber oid = objectId(obj);
            o.write((char*)&oid, sizeof(IDNumber));
        }

        void Writer::writeSize(ostream& o, SizeType s)
        {
            o.write((const char*)&s, sizeof(SizeType));
        }

        void Writer::writeByte(ostream& o, Byte b)
        {
            o.write((const char*)&b, sizeof(Byte));
        }

        void Writer::writeBool(ostream& o, Boolean b)
        {
            o.write((const char*)&b, sizeof(Boolean));
        }

        void Writer::writeOp(ostream& o, OpCode op)
        {
            unsigned char b = (unsigned char)op;
            writeByte(o, op);
        }

        void Writer::writeName(ostream& o, Name n)
        {
            o << n;
            o.put(0);
        }

        void Writer::writeNameId(ostream& o, Name n)
        {
            SizeType s = _nameMap[n.c_str()];
            assert(s != SizeType(-1));
            writeSize(o, s);
        }

        void Writer::writeU32(ostream& o, U32 u)
        {
            o.write((const char*)&u, sizeof(U32));
        }

        void Writer::writeU16(ostream& o, U16 u)
        {
            o.write((const char*)&u, sizeof(U16));
        }

        //
        //  MAIN ENTRY
        //

        size_t Writer::write(ostream& o)
        {
            freeze();
            writeHeader(o);
            writeNameTable(o);
            writeRequiredModules(o);
            writeSize(o, _rootSymbolSet.size());

            SymbolVector symbols;

            for (SymbolSet::const_iterator i = _rootSymbolSet.begin();
                 i != _rootSymbolSet.end(); ++i)
            {
                symbols.push_back(*i);
            }

            sort(symbols.begin(), symbols.end(), CompareSymbolType);

            _passNum = 0;

            for (size_t i = 0; i < symbols.size(); i++)
            {
                writePartialDeclarations(o, symbols[i], true);
            }

            _passNum = 1;

            for (size_t i = 0; i < symbols.size(); i++)
            {
                writePartialDeclarations(o, symbols[i], true);
            }

            writeDerivedTypes(o);

            for (size_t i = 0; i < symbols.size(); i++)
            {
                writeFullDeclarations(o, symbols[i], true);
            }

            writeObjects(o); // after all declarations

            return 0;
        }

        void Writer::writeHeader(ostream& o)
        {
            writeU32(o, magicNumber());
            writeU32(o, fileVersionNumber());
            writeU32(o, 32);

            U32 f = Header::NoFlags;
            if (_annotationOutput)
                f |= Header::AnnotationFlag;

            writeU32(o, f);
        }

        void Writer::writeNameTable(ostream& o)
        {
            writeSize(o, _nameMap.size());

            for (NameMap::const_iterator i = _nameMap.begin();
                 i != _nameMap.end(); ++i)
            {
                o << i->first;
                o.put(0);
            }
        }

        void Writer::writeRequiredModules(ostream& o)
        {
            size_t ignoreCount = 0;

            for (ModuleSet::const_iterator i = _requiredModules.begin();
                 i != _requiredModules.end(); ++i)
            {
                const Module* m = *i;
                if (_primarySymbolSet.count(m) > 0)
                    ignoreCount++;
            }

            writeSize(o, _requiredModules.size() - ignoreCount);

            for (ModuleSet::const_iterator i = _requiredModules.begin();
                 i != _requiredModules.end(); ++i)
            {
                const Module* m = *i;
                if (_primarySymbolSet.count(m) > 0)
                    continue;
                NameMap::const_iterator ci =
                    _nameMap.find(m->fullyQualifiedName().c_str());
                SizeType n = ci->second;
                writeSize(o, n);
            }
        }

        void Writer::writeDerivedTypes(ostream& o)
        {
            vector<const Type*> types;

            for (TypeMap::iterator i = _typeMap.begin(); i != _typeMap.end();
                 ++i)
            {
                const Type* t = (*i).first;

                if (t->isCollection() || dynamic_cast<const TupleType*>(t)
                    || dynamic_cast<const FunctionType*>(t))
                {
                    types.push_back(t);
                }
            }

            writeSize(o, types.size());

            for (size_t i = 0; i < types.size(); i++)
            {
                writeNameId(o, types[i]->fullyQualifiedName());
                if (_debugOutput)
                    cout << "< derived " << types[i]->fullyQualifiedName()
                         << endl;
            }
        }

        void Writer::writePartialDeclarations(ostream& o, const Symbol* s,
                                              bool root)
        {
            if (root)
            {
                writeOp(o, OpPopScopeToRoot);
                writeNameId(o, s->scope()->fullyQualifiedName());
            }

            if (const Function* F = dynamic_cast<const Function*>(s))
            {
                if (_passNum == 1)
                    writeAnnotationInfo(o, s);

                if ((F->isConstructor() && !F->hasParameters())
                    || !F->hasParameters() || F->isGenerated() || !F->body())
                {
                    writeOp(o, OpPass);
                }
                else if (dynamic_cast<const MemberFunction*>(F))
                {
                    writeOp(o, OpDeclareMemberFunction);
                    writeNameId(o, s->name());
                    writePartialFunctionDeclaration(o, F);
                }
                else
                {
                    writeOp(o, OpDeclareFunction);
                    writeNameId(o, s->name());
                    writePartialFunctionDeclaration(o, F);
                }
            }
            else if (const FunctionType* T =
                         dynamic_cast<const FunctionType*>(s))
            {
                writeOp(o, OpPass);
            }
            else if (const Alias* a = dynamic_cast<const Alias*>(s))
            {
                if (_passNum == 1)
                    writeAnnotationInfo(o, s);
                writeOp(o, OpDeclareAlias);
                writePartialAliasDeclaration(o, a);
            }
            else if (const Class* C = dynamic_cast<const Class*>(s))
            {
                if (_passNum == 0)
                    writeAnnotationInfo(o, s);
                writeOp(o, OpDeclareClass);
                writeNameId(o, s->name());
                writePartialClassDeclaration(o, C);
            }
            else if (const VariantTagType* T =
                         dynamic_cast<const VariantTagType*>(s))
            {
                writeOp(o, OpPass);
            }
            else if (const VariantType* V = dynamic_cast<const VariantType*>(s))
            {
                if (_passNum == 0)
                    writeAnnotationInfo(o, s);
                writeOp(o, OpDeclareVariantType);
                writeNameId(o, s->name());
                writePartialVariantDeclaration(o, V);
            }
            else if (const Namespace* N = dynamic_cast<const Namespace*>(s))
            {
                if (_passNum == 0)
                    writeAnnotationInfo(o, s);
                writeOp(o, OpDeclareNamespace);
                writeNameId(o, s->name());
                writePartialNamespaceDeclaration(o, N);
            }
            else if (const Module* M = dynamic_cast<const Module*>(s))
            {
                if (_passNum == 0)
                    writeAnnotationInfo(o, s);
                writeOp(o, OpDeclareModule);
                writeNameId(o, s->name());
                writePartialModuleDeclaration(o, M);
            }
            else if (const ParameterVariable* v =
                         dynamic_cast<const ParameterVariable*>(s))
            {
                // nothing
                writeOp(o, OpPass);
            }
            else if (const StackVariable* v =
                         dynamic_cast<const StackVariable*>(s))
            {
                if (_passNum == 1)
                    writeAnnotationInfo(o, s);
                writeOp(o, OpDeclareStack);
                writeNameId(o, s->name());
                writePartialStackDeclaration(o, v);
            }
            else if (const GlobalVariable* v =
                         dynamic_cast<const GlobalVariable*>(s))
            {
                if (_passNum == 1)
                    writeAnnotationInfo(o, s);
                writeOp(o, OpDeclareGlobal);
                writeNameId(o, s->name());
                writePartialGlobalDeclaration(o, v);
            }
            else if (const SymbolicConstant* c =
                         dynamic_cast<const SymbolicConstant*>(s))
            {
                if (_passNum == 1)
                    writeAnnotationInfo(o, s);
                writeOp(o, OpDeclareSymbolicConstant);
                writeNameId(o, s->name());
                writePartialSymbolicConstantDeclaration(o, c);
            }
            else
            {
                writeOp(o, OpPass);
            }
        }

        void Writer::writeAnnotationInfo(ostream& o, const Symbol* s)
        {
            if (_context->debugging() && _annotationOutput)
            {
                const Context::SymbolDefinitionMap& symdefs =
                    _context->symbolDefinitions();
                Context::SymbolDefinitionMap::const_iterator i =
                    symdefs.find(s);
                if (i == symdefs.end())
                    return;

                const Context::SourceRecord& r = (*i).second;

                if (r.filename != _annSource)
                {
                    _annSource = r.filename;
                    writeOp(o, OpSourceFile);
                    writeNameId(o, _annSource);
                }

                if (r.linenum != _annLine)
                {
                    _annLine = r.linenum;
                    writeOp(o, OpSourceLine);
                    writeU16(o, _annLine);
                }

                if (r.charnum != _annChar)
                {
                    _annChar = r.charnum;
                    writeOp(o, OpSourceChar);
                    writeU16(o, _annChar);
                }
            }
        }

        void Writer::writePartialChildDeclarations(ostream& o, const Symbol* s)
        {
            if (SymbolTable* st = s->symbolTable())
            {
                SymbolVector symbols;

                for (SymbolTable::Iterator i(st); i; ++i)
                {
                    for (const Symbol* S = *i; S; S = S->nextOverload())
                    {
                        if (_primarySymbolSet.count(S) > 0)
                            symbols.push_back(S);
                    }
                }

                if (!symbols.empty())
                {
                    sort(symbols.begin(), symbols.end(), CompareSymbolType);
                    writeOp(o, OpPushCurrentScope);
                    writeSize(o, symbols.size());

                    for (size_t i = 0; i < symbols.size(); i++)
                    {
                        const Symbol* S = symbols[i];
                        writePartialDeclarations(o, S);
                    }

                    return;
                }
            }

            writeOp(o, OpDone);
        }

        void Writer::writePartialStackDeclaration(ostream& o,
                                                  const StackVariable* v)
        {
            if (_passNum == 1)
            {
                if (_debugOutput)
                {
                    cout << "< declaration of ";
                    v->output(cout);
                    cout << endl;
                }

                // writeNameId(o, v->name());
                writeNameId(o, v->storageClass()->fullyQualifiedName());
                writeU32(o, (U32)v->attributes());

                writePartialChildDeclarations(o, v);
            }
        }

        void Writer::writePartialGlobalDeclaration(ostream& o,
                                                   const GlobalVariable* v)
        {
            if (_passNum == 1)
            {
                if (_debugOutput)
                {
                    cout << "< declaration of ";
                    v->output(cout);
                    cout << endl;
                }

                // writeNameId(o, v->name());
                writeNameId(o, v->storageClass()->fullyQualifiedName());
                writeU32(o, (U32)v->attributes());

                writePartialChildDeclarations(o, v);
            }
        }

        void Writer::writePartialSymbolicConstantDeclaration(
            ostream& o, const SymbolicConstant* s)
        {
            if (_passNum == 1)
            {
                if (_debugOutput)
                {
                    cout << "< declaration of symbolic constant ";
                    s->output(cout);
                    cout << endl;
                }

                const Type* t = s->type();
                writeNameId(o, t->fullyQualifiedName());
                Value v = s->value();

                if (t->isPrimitiveType())
                {
                    t->serialize(o, *this, (ValuePointer)&v);
                }
                else
                {
                    writeObjectId(o, (const Object*)v._Pointer);
                }

                writePartialChildDeclarations(o, s);
            }
        }

        void
        Writer::writePartialMemberVariableDeclaration(ostream& o,
                                                      const MemberVariable* v)
        {
            if (_passNum == 1)
            {
                if (_debugOutput)
                {
                    cout << "< declaration of member var ";
                    v->output(cout);
                    cout << endl;
                }

                // writeNameId(o, v->name());
                writeNameId(o, v->storageClass()->fullyQualifiedName());
                writeU32(o, (U32)v->attributes());

                writePartialChildDeclarations(o, v);
            }
        }

        void Writer::writePartialAliasDeclaration(ostream& o, const Alias* a)
        {
            if (_passNum == 1)
            {
                if (_debugOutput)
                    cout << "< declaration of alias " << a->fullyQualifiedName()
                         << endl;
            }
        }

        void Writer::writePartialClassDeclaration(ostream& o, const Class* c)
        {
            if (_passNum == 0)
            {
                if (_debugOutput)
                    cout << "< declaration of class " << c->fullyQualifiedName()
                         << endl;
                // writeNameId(o, c->name());
            }
            else if (_passNum == 1)
            {

                //
                //  To maintain order we have to write the members here
                //

                const Class::ClassVector& supers = c->superClasses();
                writeSize(o, supers.size());

                for (size_t i = 0; i < supers.size(); i++)
                {
                    writeNameId(o, supers[i]->fullyQualifiedName());
                }

                const Class::MemberVariableVector& vars = c->memberVariables();
                size_t count = 0;

                for (size_t i = 0; i < vars.size(); i++)
                {
                    const MemberVariable* v = vars[i];
                    if (!c->isInBaseClass(v))
                        count++;
                }

                writeSize(o, count);

                for (size_t i = 0; i < vars.size(); i++)
                {
                    const MemberVariable* v = vars[i];

                    if (!c->isInBaseClass(v))
                    {
                        writeNameId(o, v->name());
                        writeNameId(o, v->storageClass()->fullyQualifiedName());
                    }
                }
            }

            writePartialChildDeclarations(o, c);
        }

        void Writer::writePartialVariantDeclaration(ostream& o,
                                                    const VariantType* V)
        {
            if (_passNum == 0)
            {
                if (_debugOutput)
                    cout << "< declaration of variant type "
                         << V->fullyQualifiedName() << endl;
                // writeNameId(o, V->name());
            }

            writePartialChildDeclarations(o, V);
        }

        void Writer::writePartialFunctionDeclaration(ostream& o,
                                                     const Function* f)
        {
            if (_passNum == 0)
                return;
            if (_debugOutput)
                cout << "< declaration of " << f->fullyQualifiedName() << endl;

            _functions.push_back(f);

            writeU32(o, _functionMap[f]);

            writeNameId(o, f->returnType()->fullyQualifiedName());
            writeSize(o, f->numArgs());
            writeSize(o, f->numFreeVariables());
            writeU32(o, (U32)(f->baseAttributes()));

            size_t n = f->numArgs() + f->numFreeVariables();

            bool isMember = dynamic_cast<const MemberFunction*>(f);

            for (size_t i = 0; i < n; i++)
            {
                if (isMember && !i)
                    continue;
                const ParameterVariable* p = f->parameter(i);
                writeNameId(o, p->name());
                writeNameId(o, p->storageClass()->fullyQualifiedName());
                writeBool(o, p->hasDefaultValue());

                if (p->hasDefaultValue())
                {
                    const Type* t = p->storageClass();
                    Value v = p->defaultValue();

                    if (t->isPrimitiveType())
                    {
                        t->serialize(o, *this, (ValuePointer)&v);
                    }
                    else
                    {
                        writeObjectId(o, (const Object*)v._Pointer);
                    }
                }
            }

            // writePartialChildDeclarations(o, f);
        }

        void Writer::writePartialModuleDeclaration(ostream& o, const Module* m)
        {
            if (_passNum == 0)
                if (_debugOutput)
                    cout << "< declaration of module "
                         << m->fullyQualifiedName() << endl;
            // writeNameId(o, m->name());
            writePartialChildDeclarations(o, m);
        }

        void Writer::writePartialNamespaceDeclaration(ostream& o,
                                                      const Namespace* n)
        {
            if (_passNum == 0)
                if (_debugOutput)
                    cout << "< declaration of namespace "
                         << n->fullyQualifiedName() << endl;
            // writeNameId(o, n->name());
            writePartialChildDeclarations(o, n);
        }

        void Writer::writeFullDeclarations(ostream& o, const Symbol* s,
                                           bool root)
        {
            if (root)
            {
                writeOp(o, OpPopScopeToRoot);
                writeNameId(o, s->scope()->fullyQualifiedName());
            }

            if (const Function* F = dynamic_cast<const Function*>(s))
            {
                if ((F->isConstructor() && !F->hasParameters())
                    || !F->hasParameters() || F->isGenerated() || !F->body())
                {
                    writeOp(o, OpPass);
                }
                else if (dynamic_cast<const MemberFunction*>(F))
                {
                    writeOp(o, OpDeclareMemberFunction);
                    writeNameId(o, s->fullyQualifiedName());
                    writeFunctionDeclaration(o, F);
                }
                else
                {
                    writeOp(o, OpDeclareFunction);
                    writeNameId(o, s->fullyQualifiedName());
                    writeFunctionDeclaration(o, F);
                }
            }
            else if (const FunctionType* T =
                         dynamic_cast<const FunctionType*>(s))
            {
                writeOp(o, OpPass);
                // writeOp(o, OpDeclareFunction);
                // writePartialFunctionDeclaration(o, T);
            }
            else if (const VariantTagType* t =
                         dynamic_cast<const VariantTagType*>(s))
            {
                writeOp(o, OpDeclareVariantTagType);
                writeNameId(o, s->fullyQualifiedName());
                writeVariantTagDeclaration(o, t);
            }
            else if (const VariantType* t = dynamic_cast<const VariantType*>(s))
            {
                writeOp(o, OpDeclareVariantType);
                writeNameId(o, s->fullyQualifiedName());
                writeVariantDeclaration(o, t);
            }
            else if (const Class* C = dynamic_cast<const Class*>(s))
            {
                writeOp(o, OpDeclareClass);
                writeNameId(o, s->fullyQualifiedName());
                writeClassDeclaration(o, C);
            }
            else if (const Alias* a = dynamic_cast<const Alias*>(s))
            {
                writeOp(o, OpDeclareAlias);
                writeNameId(o, s->fullyQualifiedName());
                writeAliasDeclaration(o, a);
            }
            else if (const Namespace* N = dynamic_cast<const Namespace*>(s))
            {
                writeOp(o, OpDeclareNamespace);
                writeNameId(o, s->fullyQualifiedName());
                writeNamespaceDeclaration(o, N);
            }
            else if (const Module* M = dynamic_cast<const Module*>(s))
            {
                writeOp(o, OpDeclareModule);
                writeNameId(o, s->fullyQualifiedName());
                writeModuleDeclaration(o, M);
            }
            else if (const ParameterVariable* v =
                         dynamic_cast<const ParameterVariable*>(s))
            {
                writeOp(o, OpPass);
            }
            else if (const StackVariable* v =
                         dynamic_cast<const StackVariable*>(s))
            {
                writeOp(o, OpDeclareStack);
                writeNameId(o, s->fullyQualifiedName());
                writeStackDeclaration(o, v);
            }
            else if (const GlobalVariable* v =
                         dynamic_cast<const GlobalVariable*>(s))
            {
                writeOp(o, OpDeclareGlobal);
                writeNameId(o, s->fullyQualifiedName());
                writeGlobalDeclaration(o, v);
            }
            else
            {
                writeOp(o, OpPass);
            }
        }

        void Writer::writeChildDeclarations(ostream& o, const Symbol* s)
        {
            if (SymbolTable* st = s->symbolTable())
            {
                SymbolVector symbols;

                for (SymbolTable::Iterator i(st); i; ++i)
                {
                    for (const Symbol* S = *i; S; S = S->nextOverload())
                    {
                        if (_primarySymbolSet.count(S) > 0)
                            symbols.push_back(S);
                    }
                }

                if (!symbols.empty())
                {
                    sort(symbols.begin(), symbols.end(), CompareSymbolType);
                    writeOp(o, OpPushCurrentScope);
                    writeSize(o, symbols.size());

                    for (size_t i = 0; i < symbols.size(); i++)
                    {
                        writeFullDeclarations(o, symbols[i]);
                    }

                    return;
                }
            }

            writeOp(o, OpDone);
        }

        void Writer::writeFunctionDeclaration(ostream& o, const Function* f)
        {
            if (_debugOutput)
            {
                cout << "< writing function ";
                f->output(cout);
                cout << endl;
            }

            writeU32(o, _functionMap[f]);

            //_current = f;
            _passNum = 0;
            writePartialChildDeclarations(o, f);
            _passNum = 1;
            writePartialChildDeclarations(o, f);

            writeExpression(o, f->body());
            writeChildDeclarations(o, f);
        }

        void Writer::writeVariantDeclaration(ostream& o, const VariantType* V)
        {
            writeChildDeclarations(o, V);
        }

        void Writer::writeVariantTagDeclaration(ostream& o,
                                                const VariantTagType* V)
        {
            if (_debugOutput)
                cout << "< declaration of variant tag type "
                     << V->fullyQualifiedName() << endl;
            writeNameId(o, V->name());
            writeNameId(o, V->representationType()->fullyQualifiedName());
            // writeChildDeclarations(o, V);
        }

        void Writer::writeAliasDeclaration(ostream& o, const Alias* a)
        {
            if (_debugOutput)
                cout << "< declaration of alias " << a->fullyQualifiedName()
                     << endl;
            writeNameId(o, a->name());
            writeNameId(o, a->alias()->fullyQualifiedName());
        }

        void Writer::writeClassDeclaration(ostream& o, const Class* c)
        {

            writeChildDeclarations(o, c);
        }

        void Writer::writeModuleDeclaration(ostream& o, const Module* m)
        {
            writeChildDeclarations(o, m);
        }

        void Writer::writeNamespaceDeclaration(ostream& o, const Namespace* n)
        {
            writeChildDeclarations(o, n);
        }

        void Writer::writeStackDeclaration(ostream& o, const StackVariable* v)
        {
        }

        void Writer::writeGlobalDeclaration(ostream& o, const GlobalVariable* v)
        {
        }

        size_t Writer::writeObjects(ostream& o)
        {
            //
            //  Write the number of root objects
            //

            writeSize(o, _rootObjects.size());

            //
            //  Write their object ids
            //

            for (ObjectSet::const_iterator i = _rootObjects.begin();
                 i != _rootObjects.end(); ++i)
            {
                writeObjectId(o, *i);
            }

            //
            //  Write the number of objects (altogether)
            //

            writeSize(o, _objects.size());

            //
            //  Write the objects
            //

            for (ObjectMap::const_iterator i = _objects.begin();
                 i != _objects.end(); ++i)
            {
                const Object* obj = i->first;
                const Type* t = obj->type();

                writeNameId(o, t->fullyQualifiedName());
                t->serialize(o, *this, &obj);

                if (_debugOutput)
                {
                    cout << "< object ";
                    Value value;
                    value._Pointer = (Pointer)obj;
                    t->outputValue(cout, value);
                    cout << endl;
                }
            }

            return _objects.size();
        }

        void Writer::writeExpression(ostream& o, const Node* n)
        {
            //
            //  The expression can take one of these general forms:
            //
            //  1) A function call
            //  2) A variable look up
            //  3) A construct call with local stack variables
            //  4) A constant
            //

            const Symbol* s = n->symbol();
            size_t nargs = n->numArgs();

            if (const Function* f = dynamic_cast<const Function*>(s))
            {
                //
                //  Function call
                //

                if (_context->debugging() && _annotationOutput)
                {
                    const AnnotatedNode* an =
                        static_cast<const AnnotatedNode*>(n);

                    if (an->sourceFileName() != _annSource)
                    {
                        _annSource = an->sourceFileName();
                        writeOp(o, OpSourceFile);
                        writeNameId(o, _annSource);
                    }

                    if (an->linenum() != _annLine)
                    {
                        _annLine = an->linenum();
                        writeOp(o, OpSourceLine);
                        writeU16(o, _annLine);
                    }

                    if (an->charnum() != _annChar)
                    {
                        _annChar = an->charnum();
                        writeOp(o, OpSourceChar);
                        writeU16(o, _annChar);
                    }
                }

                if (const NoOp* noop = dynamic_cast<const NoOp*>(f))
                {
                    writeOp(o, OpNoop);
                }
                else if (dynamic_cast<const Curry*>(f)
                         || dynamic_cast<const DynamicPartialEvaluate*>(f)
                         || dynamic_cast<const DynamicPartialApplication*>(f))

                {
                    //
                    //  NOTE: can't do this if the function is
                    //  overloaded. This is for low level calls that are not
                    //  ambiguous and can't be "interpreted" by NodeAssembler
                    //  (and shouldn't be). Examples are partial evaluation
                    //  and application.
                    //

                    assert(f->firstOverload() == f && !f->nextOverload());

                    writeOp(o, OpCallLiteral);
                    writeNameId(o, f->fullyQualifiedName());
                    writeSize(o, nargs);
                }
                else if (dynamic_cast<const MemberFunction*>(f)
                         && n->func()
                                == n->type()->machineRep()->callMethodFunc())
                {
                    writeOp(o, OpCallMethod);
                    writeNameId(o, f->fullyQualifiedName());
                    writeSize(o, nargs);
                }
                else
                {
                    bool exact = true;

                    if (exact && nargs == f->numArgs() + f->numFreeVariables())
                    {
                        for (size_t i = 0; i < nargs; i++)
                        {
                            if (f->argType(i) != n->argNode(i)->type())
                            {
                                exact = false;
                                break;
                            }
                        }
                    }
                    else
                    {
                        exact = false;
                    }

                    // char* n = f->name().c_str();
                    // if (n[0] == '_' && n[1] == '_') exact = false;

                    if (!exact || f->isVariadic() || f->isPolymorphic()
                        || f->isMultiSigniture())
                    {
                        writeOp(o, OpCallBest);
                    }
                    else
                    {
                        writeOp(o, OpCall);
                    }

                    writeNameId(o, f->fullyQualifiedName());
                    writeSize(o, nargs);
                }
            }
            else if (const Variable* v = dynamic_cast<const Variable*>(s))
            {
                //
                //  Variable look up
                //

                if (dynamic_cast<const StackVariable*>(v))
                {
                    if (n->type()->isReferenceType())
                    {
                        writeOp(o, OpReferenceStack);
                    }
                    else
                    {
                        writeOp(o, OpDereferenceStack);
                    }
                }
                else if (dynamic_cast<const MemberVariable*>(v))
                {
                    if (n->type()->isReferenceType())
                    {
                        writeOp(o, OpReferenceClassMember);
                    }
                    else
                    {
                        writeOp(o, OpDereferenceClassMember);
                    }
                }
                else
                {
                    if (n->type()->isReferenceType())
                    {
                        writeOp(o, OpReference);
                    }
                    else
                    {
                        writeOp(o, OpDereference);
                    }
                }

                writeNameId(o, v->fullyQualifiedName());
            }
            else if (const Type* t = dynamic_cast<const Type*>(s))
            {
                //
                //  Constant
                //

                writeOp(o, OpConstant);
                writeNameId(o, t->fullyQualifiedName());
                const DataNode* dn = static_cast<const DataNode*>(n);

                if (t->isPrimitiveType())
                {
                    t->serialize(o, *this, (const ValuePointer)(&dn->_data));
                }
                else
                {
                    writeObjectId(o, (Object*)dn->_data._Pointer);
                }
            }

            for (int i = 0; i < nargs; i++)
            {
                writeExpression(o, n->argNode(i));
            }
        }

        //----------------------------------------------------------------------

        //
        //  Write MUD file
        //

        size_t Writer::writeDocumentation(ostream& o)
        {
            freeze();
            _currentScope = _context->globalScope();
            size_t count = 0;

            if (!_primarySymbols.empty())
            {
                o << "documentation: {" << endl;

                for (size_t i = 0; i < _primarySymbols.size(); i++)
                {
                    const Symbol* s = _primarySymbols[i];

                    if (Object* obj = _process->documentSymbol(s))
                    {
                        count++;
                        writeSymbolDocumentation(o, s, obj);
                    }
                }

                o << endl << "}" << endl;
            }

            return count;
        }

        void Writer::writeSymbolDocumentation(ostream& o, const Symbol* s,
                                              Object* docs)
        {
            o << s->fullyQualifiedName() << " ";
            docs->type()->outputValue(o, &docs, true);
            o << endl;
        }

        //----------------------------------------------------------------------

        Reader::Reader(Process* p, Context* c)
            : _process(p)
            , _context(c)
            , _frozen(false)
            , _barrier()
            , _debugOutput(false)
            , _annotationOutput(false)
            , _sindex(0)
            , _passNum(0)
            , _annLine(0)
            , _annChar(0)
            , _findex(0)
            , _current(0)
        {
        }

        Reader::~Reader() {}

        Object* Reader::objectOfId(IDNumber i) const
        {
            if (i >= _inverseObjectMap.size())
                return 0;
            return _inverseObjectMap[i];
        }

        U32 Reader::readU32(istream& i)
        {
            U32 x;
            i.read((char*)&x, sizeof(U32));
            return x;
        }

        U16 Reader::readU16(istream& i)
        {
            U16 x;
            i.read((char*)&x, sizeof(U16));
            return x;
        }

        string Reader::readString(istream& i)
        {
            string s;
            for (int ch; ch = i.get();)
                s += char(ch);
            return s;
        }

        Name Reader::readName(istream& i)
        {
            string s = readString(i);
            Name n = _context->internName(s.c_str());
            return n;
        }

        Name Reader::readNameId(istream& i)
        {
            IDNumber n = readIDNumber(i);
            assert(n < _nameTable.size());
            return _nameTable[n];
        }

        IDNumber Reader::readIDNumber(istream& i)
        {
            IDNumber x;
            i.read((char*)&x, sizeof(IDNumber));
            return x;
        }

        IDNumber Reader::readObjectId(istream& i) { return readIDNumber(i); }

        SizeType Reader::readSize(istream& i)
        {
            SizeType x;
            i.read((char*)&x, sizeof(SizeType));
            return x;
        }

        Byte Reader::readByte(istream& i)
        {
            Byte x;
            i.read((char*)&x, sizeof(Byte));
            return x;
        }

        OpCode Reader::readOp(istream& i)
        {
            OpCode op = OpCode(readByte(i));
            return op;
        }

        Boolean Reader::readBool(istream& i)
        {
            Boolean x;
            i.read((char*)&x, sizeof(Boolean));
            return x != 0;
        }

        //
        //  MAIN ENTRY
        //

        const Reader::Objects& Reader::read(istream& in)
        {
            _current = 0;
            NodeAssembler as(_context, _process);
            as.reduceConstantExpressions(false);
            as.allowUnresolvedCalls(false);
            as.simplifyExpressions(false);
            as.throwOnError(true);
            _as = &as;
            _sindex = 0;

            readHeader(in);

            if (_header.magicNumber != magicNumber()
                || _header.version > fileVersionNumber())
            {
                throw ArchiveUnknownFormatException();
            }

            readNameTable(in);
            readRequiredModules(in);

            for (size_t i = 0; i < _requiredModules.size(); i++)
            {
                if (_debugOutput)
                    cout << "> loading module " << _requiredModules[i] << endl;
                Module::load(_requiredModules[i], _process, _context);
            }

            SizeType count = readSize(in);
            _passNum = 0;
            for (size_t i = 0; i < count; i++)
                readPartialDeclarations(in);
            _passNum = 1;
            for (size_t i = 0; i < count; i++)
                readPartialDeclarations(in);
            readDerivedTypes(in);
            for (size_t i = 0; i < count; i++)
                readFullDeclarations(in);

#if 0
    for (size_t i = 0; i < _classes.size(); i++)
    {
        //
        //  Can't generate class default constructors until *all*
        //  class members are present in super classes. 
        //

        Class* c = _classes[i];
        _as->generateDefaults(c);
    }
#endif

            readObjects(in);

            for (size_t i = 0; i < _constantNodes.size(); i++)
            {
                DataNode* dn = _constantNodes[i];
                size_t id = dn->_data._int;
                Object* o = objectOfId(id);
                dn->_data._Pointer = o;
            }

            for (size_t i = 0; i < _defaultValueParams.size(); i++)
            {
                const DefaultValuePair& p = _defaultValueParams[i];
                ParameterVariable* pv = const_cast<ParameterVariable*>(p.first);
                pv->_value._Pointer = objectOfId((size_t)p.second._Pointer);
            }

            for (size_t i = 0; i < _symbolicConstants.size(); i++)
            {
                const SymbolicConstant* c = _symbolicConstants[i];
                if (!c->type()->isPrimitiveType())
                {
                    ((SymbolicConstant*)c)
                        ->setValue(Value(
                            Pointer(objectOfId((size_t)c->value()._Pointer))));
                }
            }

            for (size_t i = 0; i < _functions.size(); i++)
            {
                const Function* F = _functions[i];
                if (_debugOutput)
                {
                    cout << "> ";
                    F->output(cout);
                    cout << " -> ";
                    NodePrinter printer(F->body(), cout, NodePrinter::Lispy);
                    printer.traverse();
                    cout << endl;
                }
            }

            //
            //  Call initialization code
            //

            for (size_t i = 0; i < _initFunctions.size(); i++)
            {
                Thread* t = _process->newApplicationThread();
                Function::ArgumentVector args;
                t->call(_initFunctions[i], args, false);
                _process->releaseApplicationThread(t);
            }

            return _inverseObjectMap;
        }

        void Reader::readHeader(istream& in)
        {
            in.read((char*)&_header, sizeof(Header));
        }

        void Reader::readRequiredModules(istream& in)
        {
            SizeType s = readSize(in);

            for (size_t i = 0; i < s; i++)
            {
                _requiredModules.push_back(readNameId(in));
            }
        }

        void Reader::readNameTable(istream& in)
        {
            const SizeType size = readSize(in);
            _nameTable.resize(size);

            for (size_t i = 0; i < size; i++)
            {
                string s = readString(in);
                _nameTable[i] = _context->internName(s.c_str());
            }
        }

        void Reader::readDerivedTypes(istream& in)
        {
            size_t n = readSize(in);

            for (size_t i = 0; i < n; i++)
            {
                Name typeName = readNameId(in);
                const Type* t = findType(typeName);
                if (_debugOutput)
                    cout << "> derived " << t->fullyQualifiedName() << endl;
            }
        }

        void Reader::readPartialDeclarations(istream& in)
        {
            OpCode op = readOp(in);

            if (op == OpPopScopeToRoot)
            {
                Name scopeName = readNameId(in);

                if (scopeName == "")
                {
                    _as->popScopeToRoot();
                }
                else if (Symbol* scope =
                             _context->findSymbolByQualifiedName(scopeName))
                {
                    _as->popScopeToRoot();
                    _as->pushScope(scope);
                }
                else
                {
                    // Fail archive read completely here
                    cout << "ERROR: failed to find scope: " << scopeName
                         << endl;
                }

                op = readOp(in);
            }

            switch (op)
            {
            case OpSourceFile:
                _as->setSourceName(readNameId(in).c_str());
                readPartialDeclarations(in);
                return;
            case OpSourceLine:
                _as->setLine(readU16(in));
                readPartialDeclarations(in);
                return;
            case OpSourceChar:
                _as->setChar(readU16(in));
                readPartialDeclarations(in);
                return;
            case OpDeclareFunction:
                readPartialFunctionDeclaration(in, false);
                return;
            case OpDeclareMemberFunction:
                readPartialFunctionDeclaration(in, true);
                return;
            case OpDeclareClass:
                readPartialClassDeclaration(in);
                return;
            case OpDeclareAlias:
                readPartialAliasDeclaration(in);
                return;
            case OpDeclareNamespace:
                readPartialNamespaceDeclaration(in);
                return;
            case OpDeclareVariantType:
                readPartialVariantDeclaration(in);
                return;
            case OpDeclareVariantTagType:
                readPartialVariantTagDeclaration(in);
                return;
            case OpDeclareModule:
                readPartialModuleDeclaration(in);
                return;
            case OpDeclareStack:
                readPartialStackDeclaration(in);
                return;
            case OpDeclareGlobal:
                readPartialGlobalDeclaration(in);
                return;
            case OpDeclareSymbolicConstant:
                readPartialSymbolicConstantDeclaration(in);
                return;
            case OpDeclareMemberVariable:
                readPartialMemberVariableDeclaration(in);
                return;
            case OpPass:
                //_allSymbols.push_back(0);
                return;
            case OpDone:
                return;
            default:
                return;
            }
        }

        void Reader::readPartialChildDeclarations(istream& in)
        {
            OpCode op = readOp(in);

            if (op == OpPushCurrentScope)
            {
                _as->pushScope(_current);

                SizeType count = readSize(in);

                for (size_t i = 0; i < count; i++)
                {
                    readPartialDeclarations(in);
                }

                _as->popScope();
            }
            else if (op == OpDone)
            {
                // nothing
            }
            else
            {
                cout << "Bad op = " << op << endl;
            }
        }

        void Reader::readPartialStackDeclaration(istream& in)
        {
            Name name = readNameId(in);

            if (_passNum == 1)
            {
                Name typeName = readNameId(in);
                U32 attrs = readU32(in);

                if (_debugOutput)
                    cout << "> declare stack variable " << typeName << " "
                         << name << endl;

                const Type* t = findType(typeName);

                StackVariable* v = _as->declareStackVariable(
                    t, name, (Variable::Attribute)attrs);
                //_allSymbols.push_back(v);
                _symbolMap[v->fullyQualifiedName()] = v;
                readPartialChildDeclarations(in);
            }
        }

        void Reader::readPartialGlobalDeclaration(istream& in)
        {
            Name name = readNameId(in);

            if (_passNum == 1)
            {
                Name typeName = readNameId(in);
                U32 attrs = readU32(in);

                if (_debugOutput)
                    cout << "> declare global variable " << typeName << " "
                         << name << endl;

                const Type* t = findType(typeName);

                GlobalVariable* v = _as->declareGlobalVariable(t, name);
                //_allSymbols.push_back(v);
                _symbolMap[v->fullyQualifiedName()] = v;
                readPartialChildDeclarations(in);
            }
        }

        void Reader::readPartialSymbolicConstantDeclaration(istream& in)
        {
            Name name = readNameId(in);

            if (_passNum == 1)
            {
                Name typeName = readNameId(in);
                const Type* t = findType(typeName);
                SymbolicConstant* c = 0;

                if (t->isPrimitiveType())
                {
                    Value v;
                    t->deserialize(in, *this, (ValuePointer)&v);
                    c = new SymbolicConstant(_context, name.c_str(), t, v);
                }
                else
                {
                    Value v((Pointer)(size_t)readObjectId(in));
                    c = new SymbolicConstant(_context, name.c_str(), t, v);
                }

                _symbolMap[c->fullyQualifiedName()] = c;
                _as->scope()->addSymbol(c);
                _symbolicConstants.push_back(c);
                if (_debugOutput)
                    cout << "> declare symbolic constant "
                         << c->fullyQualifiedName() << endl;

                readPartialChildDeclarations(in);
            }
        }

        void Reader::readPartialMemberVariableDeclaration(istream& in)
        {
            Name name = readNameId(in);
            if (_passNum == 0)
                return;
            Name typeName = readNameId(in);
            U32 attrs = readU32(in);

            if (_debugOutput)
                cout << "> declare variable " << typeName << " " << name
                     << endl;

            const Type* t = findType(typeName);
            MemberVariable* v = new MemberVariable(_context, name.c_str(), t);
            _as->scope()->addSymbol(v);
            //_allSymbols.push_back(v);
            _symbolMap[v->fullyQualifiedName()] = v;

            readPartialChildDeclarations(in);
        }

        void Reader::readPartialModuleDeclaration(istream& in)
        {
            Name name = readNameId(in);

            if (_passNum == 0)
            {
                if (_debugOutput)
                    cout << "> declare module " << name << endl;
                bool exists = _as->scope()->findSymbolOfType<Module>(name) != 0;
                _as->pushModuleScope(name);
                Module* m = static_cast<Module*>(_as->scope());

                if (!exists)
                    _modules.push_back(m);

                _current = m;
                _symbolMap[m->fullyQualifiedName()] = m;
            }
            else
            {
                Module* m = _as->scope()->findSymbolOfType<Module>(name);
                _as->pushScope(m);
                _current = m;
            }

            readPartialChildDeclarations(in);
            _as->popScope();
            _current = _as->scope();
        }

        void Reader::readPartialNamespaceDeclaration(istream& in)
        {
            Name name = readNameId(in);

            if (_passNum == 0)
            {
                Namespace* n = _as->declareNamespace(name);
                if (_debugOutput)
                    cout << "> declare namespace " << n->fullyQualifiedName()
                         << endl;
                //_allSymbols.push_back(n);
                _symbolMap[n->fullyQualifiedName()] = n;
                _as->pushScope(n);
                _current = _as->scope();
            }
            else
            {
                Namespace* n = _as->scope()->findSymbolOfType<Namespace>(name);
                _as->pushScope(n);
                _current = n;
            }

            readPartialChildDeclarations(in);
            _as->popScope();
            _current = _as->scope();
        }

        void Reader::readPartialFunctionDeclaration(istream& in, bool member)
        {
            Name name = readNameId(in);
            if (_passNum == 0)
                return;

            U32 fid = readU32(in);
            Name typeName = readNameId(in);
            const Type* rtype = findType(typeName);
            SizeType nargs = readSize(in);
            SizeType nfree = readSize(in);
            Function::Attributes attrs = (Function::Attributes)readU32(in);
            size_t ntotal = nargs + nfree;

            NodeAssembler::SymbolList l = _as->emptySymbolList();
            ParamVector freeVars;

            for (size_t i = 0; i < nargs; i++)
            {
                if (member && !i)
                    continue;
                Name pname = readNameId(in);
                Name ptypeName = readNameId(in);
                bool hasDefaultValue = readBool(in);
                bool isFreeParam = i >= nargs;

                ParameterVariable* pv = 0;

                const Type* t = findType(ptypeName);

                if (hasDefaultValue)
                {
                    if (t->isPrimitiveType())
                    {
                        Value v;
                        t->deserialize(in, *this, (ValuePointer)&v);
                        pv = new ParameterVariable(_context, pname.c_str(), t,
                                                   v);
                    }
                    else
                    {
                        Value v((Pointer)(size_t)readObjectId(in));
                        pv = new ParameterVariable(_context, pname.c_str(), t,
                                                   Value(Pointer(0)));
                        _defaultValueParams.push_back(DefaultValuePair(pv, v));
                    }
                }
                else
                {
                    pv = new ParameterVariable(_context, pname.c_str(), t);
                }

                l.push_back(pv);
            }

            //_as->newStackFrame();
            Function* F =
                member
                    ? _as->declareMemberFunction(name.c_str(), rtype, l, attrs)
                    : _as->declareFunction(name.c_str(), rtype, l, attrs);

            _functionMap[fid] = F;

            _functions.push_back(F);
            _as->removeSymbolList(l);
            _current = _functions.back();

            //
            //  This part of the NodeAssembler API or Function API is weird
            //  (as if any of it is not!): you have to declare the function
            //  without the free vars first then add them ass parameters
            //  afterwards.
            //

            for (size_t i = nargs; i < nargs + nfree; i++)
            {
                Name pname = readNameId(in);
                Name ptypeName = readNameId(in);
                bool hasDefaultValue = readBool(in); // not used
                const Type* t = findType(ptypeName);

                F->addSymbol(_as->declareFreeVariable(t, pname));
            }

            if (_debugOutput)
            {
                cout << "> declared function: ";
                _functions.back()->output(cout);
                cout << endl;
            }

            //_allSymbols.push_back(F);
            _symbolMap[F->fullyQualifiedName()] = F;

            // readPartialChildDeclarations(in);

            _as->popScope();
            _as->endStackFrame(); // just ignore the return value
        }

        void Reader::readPartialClassDeclaration(istream& in)
        {
            Name name = readNameId(in);

            if (_passNum == 0)
            {
                NodeAssembler::SymbolList l = _as->emptySymbolList();

                Class* c = _as->declareClass(name.c_str(), l);
                _current = c;

                if (_debugOutput)
                    cout << "> declared class " << c->fullyQualifiedName()
                         << endl;

                _as->removeSymbolList(l);
                //_allSymbols.push_back(_current);
                _symbolMap[c->fullyQualifiedName()] = c;
            }
            else
            {
                Class* c = _as->scope()->findSymbolOfType<Class>(name);
                SizeType s = readSize(in);

                for (size_t i = 0; i < s; i++)
                {
                    Name superName = readNameId(in);

                    if (Class* super =
                            _context->findSymbolOfTypeByQualifiedName<Class>(
                                superName, false))
                    {
                        c->addSuperClass(super);
                    }
                    else
                    {
                        // fail
                    }
                }

                SizeType n = readSize(in);

                for (size_t i = 0; i < n; i++)
                {
                    Name mname = readNameId(in);
                    Name tname = readNameId(in);
                    const Type* t = findType(tname);

                    MemberVariable* m =
                        new MemberVariable(_context, mname.c_str(), t);
                    c->addSymbol(m);

                    if (_debugOutput)
                    {
                        cout << "> read ";
                        m->output(cout);
                        cout << endl;
                    }
                }

                _as->pushScope(c);
                _current = c;
            }

            readPartialChildDeclarations(in);
            _as->popScope();
            _current = _as->scope();
        }

        void Reader::readPartialVariantDeclaration(istream& in)
        {
            Name name = readNameId(in);

            if (_passNum == 0)
            {
                VariantType* v = _as->declareVariantType(name.c_str());
                _current = v;

                if (_debugOutput)
                    cout << "> declared variant " << v->fullyQualifiedName()
                         << endl;

                //_allSymbols.push_back(_current);
                _symbolMap[v->fullyQualifiedName()] = v;
            }
            else
            {
                VariantType* t =
                    _as->scope()->findSymbolOfType<VariantType>(name);
                _as->pushScope(t);
                _current = t;
            }

            readPartialChildDeclarations(in);
            _as->popScope();
            _current = _as->scope();
        }

        void Reader::readPartialVariantTagDeclaration(istream& in)
        {
            //_allSymbols.push_back(0);
        }

        void Reader::readPartialAliasDeclaration(istream& in)
        {
            //_allSymbols.push_back(0);
        }

        void Reader::readFullDeclarations(istream& in)
        {
            OpCode op = readOp(in);

            if (op == OpPopScopeToRoot)
            {
                Name scopeName = readNameId(in);

                if (scopeName == "")
                {
                    _as->popScopeToRoot();
                }
                else if (Symbol* scope =
                             _context->findSymbolByQualifiedName(scopeName))
                {
                    _as->popScopeToRoot();
                    _as->pushScope(scope);
                }
                else
                {
                    // Fail archive read completely here
                    cout << "ERROR: failed to find scope: " << scopeName
                         << endl;
                }

                op = readOp(in);
            }

            // Symbol* s = _allSymbols[_sindex++];
            Symbol* s = 0;

            if (op != OpPass)
            {
                Name name = readNameId(in);
                s = _symbolMap[name]; // can be 0
            }

            switch (op)
            {
            case OpDeclareFunction:
                if (Function* F = dynamic_cast<Function*>(s))
                {
                    readFunctionDeclaration(in, F);
                }
                else
                {
                    abort();
                }
                break;
            case OpDeclareMemberFunction:
                if (MemberFunction* F = dynamic_cast<MemberFunction*>(s))
                {
                    readFunctionDeclaration(in, F);
                }
                else
                {
                    abort();
                }
                break;
            case OpDeclareVariantType:
                if (VariantType* v = dynamic_cast<VariantType*>(s))
                {
                    readVariantDeclaration(in, v);
                }
                else
                {
                    abort();
                }
                break;
            case OpDeclareVariantTagType:
            {
                assert(s == 0);
                readVariantTagDeclaration(in);
            }
            break;
            case OpDeclareAlias:
            {
                assert(s == 0);
                readAliasDeclaration(in);
            }
            break;
            case OpDeclareClass:
                if (Class* C = dynamic_cast<Class*>(s))
                {
                    readClassDeclaration(in, C);
                }
                else
                {
                    abort();
                }
                break;
            case OpDeclareNamespace:
                if (Namespace* N = dynamic_cast<Namespace*>(s))
                {
                    readNamespaceDeclaration(in, N);
                }
                else
                {
                    abort();
                }
                break;
            case OpDeclareModule:
                if (Module* M = dynamic_cast<Module*>(s))
                {
                    readModuleDeclaration(in, M);
                }
                else
                {
                    abort();
                }
                break;
            case OpDeclareStack:
                if (StackVariable* v = dynamic_cast<StackVariable*>(s))
                {
                    readStackDeclaration(in, v);
                }
                else
                {
                    abort();
                }
                break;
            case OpDeclareGlobal:
                if (GlobalVariable* v = dynamic_cast<GlobalVariable*>(s))
                {
                    readGlobalDeclaration(in, v);
                }
                else
                {
                    abort();
                }
                break;
            case OpPass:
                assert(s == 0);
            case OpDone:
                break;
            default:
                break;
            }
        }

        void Reader::readChildDeclarations(istream& in)
        {
            OpCode op = readOp(in);

            if (op == OpPushCurrentScope)
            {
                _as->pushScope(_current);

                SizeType count = readSize(in);

                for (size_t i = 0; i < count; i++)
                {
                    readFullDeclarations(in);
                }

                _as->popScope();
            }
            else if (op == OpDone)
            {
                // nothing
            }
            else
            {
                cout << "Bad op = " << op << endl;
            }
        }

        void Reader::readStackDeclaration(istream& in, StackVariable* v)
        {
            // nothing
        }

        void Reader::readGlobalDeclaration(istream& in, GlobalVariable* v)
        {
            // nothing
        }

        void Reader::readFunctionDeclaration(istream& in, Function* F)
        {
            if (_debugOutput)
            {
                cout << ">> func: ";
                F->output(cout);
                cout << endl;
            }

            IDNumber fid = readU32(in);
            Function* Fcached = _functionMap[fid];
            assert(Fcached);

            F = Fcached;

            _as->pushScope(F);
            _current = F;

            _passNum = 0;
            readPartialChildDeclarations(in);

            _as->newStackFrame();

            NodeAssembler::SymbolList l = _as->emptySymbolList();

            for (size_t i = 0; F->parameter(i); i++)
            {
                l.push_back(F->parameter(i));
            }

            _as->declareParameters(l);
            _as->removeSymbolList(l);

            _passNum = 1;
            readPartialChildDeclarations(in);

            size_t stackSize = _as->endStackFrame();
            F->stackSize(stackSize);
            _current = F;

            Node* node = readExpression(in, *_as);

            F->setBody(node);

            if (_debugOutput)
            {
                cout << "> read body of ";
                F->output(cout);
                cout << endl;
            }

            readChildDeclarations(in);
            _as->popScope();
            _current = F;

            string fname = F->name();

            if (fname.size() > 6 && !fname.compare(0, 6, "__init"))
            {
                _initFunctions.push_back(F);
            }
        }

        void Reader::readVariantDeclaration(istream& in, VariantType* v)
        {
            _current = v;
            readChildDeclarations(in);
        }

        void Reader::readVariantTagDeclaration(istream& in)
        {
            Name name = readNameId(in);
            Name typeName = readNameId(in);
            const Type* t = findType(typeName);

            VariantTagType* tt = _as->declareVariantTagType(name.c_str(), t);
            _current = tt;
            if (_debugOutput)
                cout << "> declared variant tag " << tt->fullyQualifiedName()
                     << endl;

            // readChildDeclarations(in);
        }

        void Reader::readAliasDeclaration(istream& in)
        {
            Name name = readNameId(in);
            Name symbolName = readNameId(in);

            Alias* a = new Alias(_context, name.c_str(), symbolName.c_str());
            _as->scope()->addSymbol(a);

            if (_debugOutput)
            {
                cout << "> declared alias ";
                a->output(cout);
                cout << endl;
            }
            // readChildDeclarations(in);
        }

        void Reader::readClassDeclaration(istream& in, Class* C)
        {
            _current = C;
            assert(!C->isFrozen());
            C->freeze();
            readChildDeclarations(in);
            _classes.push_back(C);
        }

        void Reader::readModuleDeclaration(istream& in, Module* M)
        {
            _as->pushScope(M);
            _current = M;
            readChildDeclarations(in);
            _as->popScope();
        }

        void Reader::readNamespaceDeclaration(istream& in, Namespace* N)
        {
            _as->pushScope(N);
            _current = N;
            readChildDeclarations(in);
            _as->popScope();
        }

        void Reader::readObjects(istream& in)
        {
            const Symbol* globalScope = _context->globalScope();

            //
            //  Read the number of roots.
            //

            size_t nRoots = readSize(in);

            //
            //  Read the root ids
            //

            vector<int> rootIds(nRoots);

            for (int i = 0; i < nRoots; i++)
            {
                rootIds[i] = readIDNumber(in);
            }

            //
            //  Read the number of objects
            //

            size_t nObjects = readSize(in);

            if (_debugOutput)
                cout << "> " << nObjects << " objects serialized in file"
                     << endl;

            //
            //  Read the objects
            //

            _inverseObjectMap.resize(1);
            _inverseObjectMap.front() = 0;

            for (int i = 0; i < nObjects; i++)
            {
                Name typeName = readNameId(in);
                const Type* t = findType(typeName);

                Object* obj = t->newObject();
                t->deserialize(in, *this, &obj);

                _inverseObjectMap.push_back(obj);
            }

            for (int i = 0; i < _inverseObjectMap.size(); i++)
            {
                Object* obj = _inverseObjectMap[i];
                if (obj)
                    obj->type()->reconstitute(*this, obj);
            }

            InverseObjectMap roots;

            for (int i = 0; i < rootIds.size(); i++)
            {
                roots.push_back(objectOfId(rootIds[i]));
            }

            //_inverseObjectMap = roots;
        }

        Node* Reader::readExpression(istream& in, NodeAssembler& as)
        {
            OpCode op = readOp(in);

            if (op == OpSourceFile)
            {
                _annSource = readNameId(in);
                _as->setSourceName(_annSource);
                op = readOp(in);
            }

            if (op == OpSourceLine)
            {
                _annLine = readU16(in);
                _as->setLine(_annLine);
                op = readOp(in);
            }

            if (op == OpSourceChar)
            {
                _annChar = readU16(in);
                _as->setChar(_annChar);
                op = readOp(in);
            }

            switch (op)
            {
            case OpCall:
            case OpCallBest:
            case OpCallMethod:
            {
                QualifiedName n = readNameId(in);
                Function* f =
                    as.context()->findSymbolOfTypeByQualifiedName<Function>(
                        n, false);

                if (!f)
                {
                    cout << "ABORT: Failed to find function " << n << endl;

                    cout << "scope is: " << as.scope()->fullyQualifiedName()
                         << endl;

                    cout << "source location is: " << as.sourceName()
                         << ", line " << as.lineNum() << ", char "
                         << as.charNum() << endl;

                    Symbol* ec =
                        as.context()->findSymbolOfTypeByQualifiedName<Module>(
                            as.context()->internName("extra_commands"));
                    if (ec)
                        cout << "found " << ec->fullyQualifiedName() << endl;
                    else
                        cout << "no extra_commands found" << endl;

                    if (SymbolTable* t = ec->symbolTable())
                    {
                        for (SymbolTable::Iterator i(t); i; ++i)
                        {
                            const Symbol* s = *i;
                            s->output(cout);
                            cout << endl;
                        }
                    }
                    else
                    {
                        cout << "no symbol table" << endl;
                    }

                    abort();
                }

                // if (!f) f =
                // as.context()->globalScope()->findSymbolOfType<Function>(n);
                // Symbol* s = as.context()->globalScope()->findSymbol(n);
                assert(f);
                int nargs = readSize(in);
                NodeAssembler::NodeList nl = as.emptyNodeList();

                for (int i = 0; i < nargs; i++)
                {
                    nl.push_back(readExpression(in, as));
                }

                Node* rn = 0;

                if (op == OpCall)
                {
                    rn = as.callExactOverloadedFunction(f, nl);
                }
                else if (op == OpCallMethod)
                {
                    rn = as.callMethod(dynamic_cast<const MemberFunction*>(f),
                                       nl);
                }

                //
                //    Fall back if for some reason the above failed
                //

                if (!rn)
                {
                    rn = as.callBestOverloadedFunction(f, nl);
                }

                as.removeNodeList(nl);
                assert(rn);
                return rn;
            }

            case OpCallLiteral:
            {
                QualifiedName n = readNameId(in);
                Function* f =
                    as.context()->findSymbolOfTypeByQualifiedName<Function>(
                        n, false);
                int nargs = readSize(in);
                Node* node = as.newNode(f, nargs);

                for (int i = 0; i < nargs; i++)
                {
                    node->setArg(readExpression(in, as), i);
                }

                return node;
            }

            case OpReference:
            {
                QualifiedName n = readNameId(in);
                // cout << "OpReference: " << n << endl;
                Variable* v =
                    _context->findSymbolOfTypeByQualifiedName<Variable>(n,
                                                                        false);
                assert(v);
                return as.referenceVariable(v);
            }

            case OpDereference:
            {
                QualifiedName n = readNameId(in);
                // cout << "OpDereference: " << n << endl;
                Variable* v =
                    _context->findSymbolOfTypeByQualifiedName<Variable>(n,
                                                                        false);
                assert(v);
                return as.dereferenceVariable(v);
            }

            case OpReferenceStack:
            {
                QualifiedName n = readNameId(in);
                // cout << "OpReferenceStack: " << n << endl;
                Variable* v =
                    _context->findSymbolOfTypeByQualifiedName<Variable>(n,
                                                                        false);
                assert(v);
                return as.referenceVariable(v);
            }

            case OpDereferenceStack:
            {
                QualifiedName n = readNameId(in);
                // cout << "OpDereferenceStack: " << n << endl;
                Variable* v = findStackVariableInScope(n, as);
                assert(v);
                return as.dereferenceVariable(v);
            }

            case OpReferenceClassMember:
            case OpDereferenceClassMember:
            {
                QualifiedName n = readNameId(in);
                // cout << "OpReferenceClassMember: " << n << endl;
                MemberVariable* v =
                    _context->findSymbolOfTypeByQualifiedName<MemberVariable>(
                        n, false);
                assert(v);
                Node* onode = readExpression(in, as);
                Node* rnode = as.referenceMemberVariable(v, onode);
                if (op == OpDereferenceClassMember)
                    rnode = as.dereferenceLValue(rnode);
                return rnode;
            }

            case OpConstant:
            {
                //
                //    Is it true that you can make a type object here? I
                //    don't think so: the partial declaration of the type
                //    would have to have enough information.
                //
                Name typeName = readNameId(in);
                // cout << "OpConstant:" << typeName << endl;
                const Type* t = findType(typeName);
                DataNode* dn = as.constant(t);

                if (t->isPrimitiveType())
                {
                    t->deserialize(in, *this, (const ValuePointer)(&dn->_data));
                }
                else
                {
                    dn->_data._int = (int)readIDNumber(in);
                    _constantNodes.push_back(dn);
                }

                return dn;
            }

            case OpNoop:
            {
                return as.callBestOverloadedFunction(_context->noop(),
                                                     as.emptyNodeList());
            }

            default:
                throw ArchiveReadFailureException();
                break;
            }
        }

        Variable* Reader::findStackVariableInScope(QualifiedName n,
                                                   NodeAssembler& as)
        {
            String scopeName = as.scope()->fullyQualifiedName();
            String name = n;

            if (name.size() <= scopeName.size())
            {
                return _context->findSymbolOfTypeByQualifiedName<Variable>(
                    n, false);
            }

            if (name.find(scopeName) == 0)
            {
                name = name.substr(scopeName.size() + 1);
                Name nn = _context->internName(name.c_str());
                return as.scope()->findSymbolOfTypeByQualifiedName<Variable>(
                    nn, false);
            }
            else
            {
                abort();
            }

            return 0;
        }

        const Type* Reader::findType(Name name)
        {
            const Type* t =
                _context->findSymbolOfTypeByQualifiedName<Type>(name, false);
            if (!t)
                t = _context->parseType(name.c_str(), _process);
            assert(t);

            if (!t)
            {
                cout << "ERROR: failed to find type " << name << endl;
                abort();
            }

            return t;
        }

    } // namespace Archive
} // namespace Mu
