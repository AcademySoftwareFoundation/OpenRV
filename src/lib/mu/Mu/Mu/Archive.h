#ifndef __Mu__Archive__h__
#define __Mu__Archive__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Context.h>
#include <Mu/SymbolTable.h>
#include <Mu/GarbageCollector.h>
#include <Mu/NodeVisitor.h>
#include <Mu/Object.h>
#include <Mu/MuProcess.h>
#include <Mu/Name.h>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace Mu
{
    class Variable;
    class StackVariable;
    class Signature;
    class Namespace;
    class NodeAssembler;
    class Alias;
    class Class;
    class ParameterVariable;
    class MemberVariable;
    class VariantType;
    class VariantTagType;
    class GlobalVariable;
    class SymbolicConstant;

    //
    //  Namespace Archive
    //
    //  An Archive is a byte-stream which represents objects, functions,
    //  and types. The byte-stream can be read using the Archive::Reader
    //  class and created with Archive::Writer.
    //
    //  NOTE: Garbage collection should be disabled while reading an
    //  Archive (see GarbageCollector::Barrier). This is necessary because
    //  the Archiver violates the principle that objects may never exist
    //  in a "partial" form. For example, some objects have "ids" in their
    //  reference fields during extraction that do not lead to valid
    //  objects. Once reconstituted, the objects are valid.
    //
    //  An Archive Reader or Writer should only be created on the stack
    //  and should exist only long enough to get the job done. When
    //  reading, make sure the objects have pointers on the stack or are
    //  referenced by another object before the Archive goes out of scope.
    //

    namespace Archive
    {

        //
        //  Types
        //

        typedef unsigned int IDNumber;
        typedef unsigned int SizeType;
        typedef unsigned char Byte;
        typedef unsigned char Boolean;
        typedef unsigned int U32;
        typedef unsigned short U16;
        typedef STLMap<const Object*, IDNumber>::Type ObjectMap;
        typedef STLSet<const Object*>::Type ObjectSet;
        typedef STLSet<const Module*>::Type ModuleSet;
        typedef STLSet<const Symbol*>::Type SymbolSet;
        typedef STLMap<const Type*, IDNumber>::Type TypeMap;
        typedef STLMap<const Function*, IDNumber>::Type FunctionMap;
        typedef STLMap<const Module*, IDNumber>::Type ModuleMap;
        typedef STLVector<Function*>::Type FunctionVector;
        typedef STLMap<IDNumber, Function*>::Type InverseFunctionMap;
        typedef STLVector<Module*>::Type InverseModuleMap;
        typedef STLVector<Type*>::Type InverseTypeMap;
        typedef STLVector<Object*>::Type InverseObjectMap;
        typedef STLVector<ParameterVariable*>::Type ParamVector;
        typedef STLVector<const Function*>::Type Functions;
        typedef STLVector<const Variable*>::Type Variables;
        typedef STLVector<const Module*>::Type Modules;
        typedef STLVector<const Type*>::Type Types;
        typedef STLVector<const Symbol*>::Type Symbols;
        typedef STLVector<const SymbolicConstant*>::Type SymbolicConstants;
        typedef STLVector<const Alias*>::Type Aliases;
        typedef STLMap<Name, Symbol*>::Type SymbolMap;
        typedef std::map<std::string, IDNumber> NameMap;
        typedef STLMap<int, Name::Ref>::Type UnresolvedFunctionMap;
        typedef SymbolTable::SymbolHashTable HT;
        typedef STLSet<Name>::Type Names;
        typedef STLVector<const Symbol*>::Type SymbolVector;
        typedef STLVector<Name>::Type NameVector;
        typedef STLVector<DataNode*>::Type DataNodeVector;
        typedef STLVector<Node*>::Type NodeVector;
        typedef STLVector<Class*>::Type ClassVector;
        typedef std::pair<ParameterVariable*, Value> DefaultValuePair;
        typedef STLVector<DefaultValuePair>::Type ParameterVector;

        struct Header
        {
            enum Flags
            {
                NoFlags,
                AnnotationFlag
            };

            U32 magicNumber;
            U32 version;
            U32 sizeType;
            U32 flags;
        };

        enum OpCode
        {
            OpNil,
            OpDeclareModule,
            OpDeclareClass,
            OpDeclareVariantType,
            OpDeclareVariantTagType,
            OpDeclareNamespace,
            OpDeclareStack,
            OpDeclareGlobal,
            OpDeclareMemberVariable,
            OpDeclareFunction,
            OpDeclareMemberFunction,
            OpDeclareAlias,
            OpDeclareSymbolicConstant,

            OpPushCurrentScope,
            OpPushScope,
            OpPopScope,
            OpPopScopeToRoot,
            OpDone,
            OpPass,

            OpExpression,
            OpCall,
            OpCallBest,
            OpCallLiteral,
            OpCallMethod,
            OpReference,
            OpDereference,
            OpReferenceStack,
            OpDereferenceStack,
            OpReferenceClassMember,
            OpDereferenceClassMember,
            OpConstant,

            OpSourceFile,
            OpSourceLine,
            OpSourceChar,

            OpNoop
        };

        //
        //  class Archive::Writer
        //
        //  Collect all necessary information to write an archive. Can write
        //  the archive file, but its not mandatory. The archive writer should
        //  be able to collect all necessary information with which e.g. a
        //  native compiler would need.
        //

        class Writer
        {
        public:
            class NodeSymbolCollector : public NodeVisitor
            {
            public:
                NodeSymbolCollector(Node* n, Archive::Writer* a)
                    : NodeVisitor(n)
                    , _writer(a)
                {
                    traverse();
                }

            protected:
                virtual void preOrderVisit(Node*, int depth);
                Archive::Writer* _writer;
            };

            //
            //  Constructor
            //

            Writer(Process*, Context*);
            ~Writer();

            //
            //  Related
            //

            Context* context() const { return _context; }

            //
            //  debugging
            //

            void setDebugOutput(bool b) { _debugOutput = b; }

            void setAnnotationOutput(bool b) { _annotationOutput = b; }

            bool annotationOutput() const { return _annotationOutput; }

            //
            //  Creation API. You can add as many objects as you want. The
            //  objects and everything they reference will be included in the
            //  archive. Once you're done you can call write(). The archive is
            //  no longer usable after writing.
            //
            //

            void add(const Object*);
            void add(const Symbol*);
            void add(const SymbolVector&);
            void add(const Names&);

            //
            //  Output the archive to the stream using write(). Output a .mud
            //  file using writeDocumentation(). The archive does not contain
            //  any documentation information; it can only be kept in a
            //  seperate file. The archive DOES contain debug information.
            //

            size_t write(std::ostream&);
            size_t writeDocumentation(std::ostream&);

            //
            //  Primary symbols are those marked with the "primary bit". See
            //  context.h
            //

            void collectSymbolsFromFile(Name filename, SymbolVector&,
                                        bool userOnly = true) const;

            void collectPrimarySymbols(const Symbol* root, SymbolVector&) const;
            void collectAllPrimarySymbols(SymbolVector&) const;

            void collectNames(const SymbolVector&, Names& names) const;

            //
            //  Object and Type to ID. Used by the type classes during
            //  creation and extraction.
            //

            IDNumber objectId(const Object*) const;
            Object* objectOfId(IDNumber) const;

            //
            //  Write bits
            //

            void writeU32(std::ostream&, U32);
            void writeU16(std::ostream&, U16);
            void writeObjectId(std::ostream&, const Object*);
            void writeByte(std::ostream&, Byte);
            void writeBool(std::ostream&, Boolean);
            void writeOp(std::ostream&, OpCode);
            void writeName(std::ostream&, Name);
            void writeSize(std::ostream&, SizeType);
            void writeNameId(std::ostream&, Name);

            //
            //  Symbol internment
            //

            void internModule(const Module*);
            void internType(const Type*);
            void internFunction(const Function*);
            void internName(Name);
            void internNames(const Symbol*);
            void internAnnotation(const Node*);

            //
            //  Add a module requirement
            //

            void addModuleRequirement(const Module*);

            const ModuleSet requiredModules() const { return _requiredModules; }

            //
            //  Access to internals
            //

            const SymbolSet rootSymbolSet() const { return _rootSymbolSet; }

        private:
            void freeze();
            void collect(const Object*);
            void writeHeader(std::ostream&);
            void writeNameTable(std::ostream&);
            void writeRequiredModules(std::ostream&);
            void writePartialDeclarations(std::ostream&, const Symbol*,
                                          bool root = false);
            void writePartialChildDeclarations(std::ostream&, const Symbol*);
            void writeFullDeclarations(std::ostream&, const Symbol*,
                                       bool root = false);
            void writeChildDeclarations(std::ostream&, const Symbol*);
            void writeDerivedTypes(std::ostream&);

            void writePartialVariantDeclaration(std::ostream&,
                                                const VariantType*);
            void writePartialVariantTagDeclaration(std::ostream&,
                                                   const VariantTagType*);
            void writePartialClassDeclaration(std::ostream&, const Class*);
            void writePartialAliasDeclaration(std::ostream&, const Alias*);
            void writePartialModuleDeclaration(std::ostream&, const Module*);
            void writePartialFunctionDeclaration(std::ostream&,
                                                 const Function*);
            void writePartialNamespaceDeclaration(std::ostream&,
                                                  const Namespace*);
            void writePartialStackDeclaration(std::ostream&,
                                              const StackVariable*);
            void writePartialGlobalDeclaration(std::ostream&,
                                               const GlobalVariable*);
            void writePartialMemberVariableDeclaration(std::ostream&,
                                                       const MemberVariable*);
            void
            writePartialSymbolicConstantDeclaration(std::ostream&,
                                                    const SymbolicConstant*);

            void writeClassDeclaration(std::ostream&, const Class*);
            void writeAliasDeclaration(std::ostream&, const Alias*);
            void writeModuleDeclaration(std::ostream&, const Module*);
            void writeFunctionDeclaration(std::ostream&, const Function*);
            void writeNamespaceDeclaration(std::ostream&, const Namespace*);
            void writeStackDeclaration(std::ostream&, const StackVariable*);
            void writeGlobalDeclaration(std::ostream&, const GlobalVariable*);
            void writeVariantDeclaration(std::ostream&, const VariantType*);
            void writeVariantTagDeclaration(std::ostream&,
                                            const VariantTagType*);

            size_t writeObjects(std::ostream&);
            void writeExpression(std::ostream&, const Node*);
            void writeAnnotationInfo(std::ostream&, const Symbol*);

            void collectRecursive(const Symbol*);

            void writeSymbolDocumentation(std::ostream&, const Symbol*,
                                          Object*);

        private:
            Process* _process;
            Context* _context;
            bool _debugOutput;
            bool _annotationOutput;
            GarbageCollector::Barrier _barrier;
            bool _frozen;
            ObjectMap _objects;
            ObjectSet _rootObjects;
            ModuleMap _moduleMap;
            TypeMap _typeMap;
            FunctionMap _functionMap;
            NameMap _nameMap;

            const Symbol* _currentScope;

            Functions _functions;
            Types _types;
            Variables _variables;
            Modules _modules;
            Aliases _aliases;

            size_t _passNum;

            ModuleSet _requiredModules;

            Symbols _primarySymbols;
            SymbolSet _rootSymbolSet;
            SymbolSet _primarySymbolSet;

            Name _annSource;
            unsigned int _annLine;
            unsigned int _annChar;

            friend class NodeSymbolCollectoror;
        };

        //
        //  class Archive::Reader
        //

        class Reader
        {
        public:
            typedef STLVector<Object*>::Type Objects;
            typedef STLVector<Function*>::Type Functions;

            //
            //  Constructor
            //

            Reader(Process*, Context*);
            ~Reader();

            NodeAssembler* as() const { return _as; }

            Context* context() const { return _context; }

            Process* process() const { return _process; }

            //
            //  Extraction API. The read function will return a vector of
            //  object pointers. These should be put somewhere immediately if
            //  you don't want the garbage collector to nuke them.
            //

            void setDebugOutput(bool b) { _debugOutput = b; }

            bool hasAnnotation() const
            {
                return _header.flags & Header::AnnotationFlag;
            }

            const Objects& read(std::istream&);

            //
            //  Access to internal state after read completes
            //

            const Header& header() const { return _header; }

            const NameVector& nameTable() const { return _nameTable; }

            const NameVector& requiredModules() const
            {
                return _requiredModules;
            }

            const Modules& modules() const { return _modules; }

            //
            //  Object and Type to ID. Used by the type classes during
            //  creation and extraction.
            //

            Object* objectOfId(IDNumber) const;

            //
            //  Read bits
            //

            U32 readU32(std::istream&);
            U16 readU16(std::istream&);
            std::string readString(std::istream&);
            Name readName(std::istream&);
            Name readNameId(std::istream&);
            SizeType readSize(std::istream&);
            IDNumber readIDNumber(std::istream&);
            IDNumber readObjectId(std::istream&);
            Boolean readBool(std::istream&);
            Byte readByte(std::istream&);
            OpCode readOp(std::istream&);

        private:
            const Type* findType(Name);

            void readHeader(std::istream&);
            void readNameTable(std::istream&);

            void readDerivedTypes(std::istream&);

            void readPartialDeclarations(std::istream&);
            void readPartialChildDeclarations(std::istream&);
            void readFullDeclarations(std::istream&);
            void readRequiredModules(std::istream&);

            void readPartialAliasDeclaration(std::istream&);
            void readPartialClassDeclaration(std::istream&);
            void readPartialVariantDeclaration(std::istream&);
            void readPartialVariantTagDeclaration(std::istream&);
            void readPartialNamespaceDeclaration(std::istream&);
            void readPartialModuleDeclaration(std::istream&);
            void readPartialFunctionDeclaration(std::istream&, bool);
            void readPartialStackDeclaration(std::istream&);
            void readPartialGlobalDeclaration(std::istream&);
            void readPartialSymbolicConstantDeclaration(std::istream&);
            void readPartialMemberVariableDeclaration(std::istream&);

            void readChildDeclarations(std::istream&);
            void readAliasDeclaration(std::istream&);
            void readClassDeclaration(std::istream&, Class*);
            void readNamespaceDeclaration(std::istream&, Namespace*);
            void readModuleDeclaration(std::istream&, Module*);
            void readFunctionDeclaration(std::istream&, Function*);
            void readStackDeclaration(std::istream&, StackVariable*);
            void readGlobalDeclaration(std::istream&, GlobalVariable*);
            void readVariantDeclaration(std::istream&, VariantType*);
            void readVariantTagDeclaration(std::istream&);

            OpCode readAnnotationInfo(std::istream&);

            void readObjects(std::istream&);
            Node* readExpression(std::istream&, NodeAssembler&);

            Variable* findStackVariableInScope(QualifiedName, NodeAssembler&);

        private:
            Process* _process;
            Context* _context;
            NodeAssembler* _as;

            NameVector _nameTable;
            NameVector _requiredModules;

            GarbageCollector::Barrier _barrier;
            bool _frozen;
            ObjectSet _rootObjects;
            InverseModuleMap _inverseModuleMap;
            InverseTypeMap _inverseTypeMap;
            InverseObjectMap _inverseObjectMap;
            FunctionVector _inverseFunctionMap;
            UnresolvedFunctionMap _unresolvedFuncs;

            size_t _sindex;
            SymbolMap _symbolMap;

            size_t _passNum;

            size_t _findex;
            Functions _functions;
            InverseFunctionMap _functionMap;
            Symbols _externalSymbols;

            DataNodeVector _constantNodes;
            ParameterVector _defaultValueParams;
            ClassVector _classes;
            Modules _modules;
            FunctionVector _initFunctions;
            SymbolicConstants _symbolicConstants;

            Symbol* _current;

            Header _header;

            Name _annSource;
            unsigned int _annLine;
            unsigned int _annChar;

            bool _debugOutput;
            bool _annotationOutput;

            friend class NodeSymbolCollectoror;
        };

        U32 fileVersionNumber();
        U32 magicNumber();

    } // namespace Archive
} // namespace Mu

#endif // __Mu__Archive__h__
