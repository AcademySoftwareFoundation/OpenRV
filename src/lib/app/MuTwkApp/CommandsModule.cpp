//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MuTwkApp/CommandsModule.h>
#include <MuTwkApp/FunctionAction.h>
#include <MuTwkApp/MenuItem.h>
#include <MuTwkApp/MenuState.h>
#include <MuTwkApp/MuInterface.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/List.h>
#include <Mu/ListType.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/TupleType.h>
#include <Mu/Thread.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/FixedArray.h>
#include <MuLang/FixedArrayType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <TwkApp/Command.h>
#include <TwkApp/Document.h>
#include <TwkApp/EventTable.h>
#include <TwkApp/Mode.h>
#include <TwkUtil/FrameUtils.h>
#include <stl_ext/string_algo.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

namespace TwkApp
{
    using namespace TwkApp;
    using namespace Mu;
    using namespace std;
    using namespace TwkUtil;
    typedef StringType::String String;

    CommandsModule::CommandsModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    CommandsModule::~CommandsModule() {}

    void CommandsModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;
        typedef ParameterVariable Param;

        //
        //  Force the existance of the types we're going to use
        //

        MuLangContext* context = (MuLangContext*)globalModule()->context();
        const Type* stringType = context->stringType();
        const Type* floatType = context->floatType();
        const Type* intType = context->intType();
        const Type* vec2fType = context->vec2fType();
        Context* c = context;

        Context::TypeVector types;
        types.push_back(context->stringType());
        types.push_back(context->intType());
        context->tupleType(types);

        context->arrayType(floatType, 1, 0);    // float[]
        context->arrayType(floatType, 2, 4, 4); // float[4,4]
        context->arrayType(intType, 1, 0);      // int[]
        context->arrayType(stringType, 1, 0);   // string[]
        context->arrayType(vec2fType, 1, 0);
        context->functionType("(void;Event)");

        types.clear();
        types.push_back(stringType);
        types.push_back(stringType);

        context->listType(stringType);
        context->arrayType(context->tupleType(types), 1, 0);

        globalScope()->addSymbols(new MenuItem(c, "MenuItem", 0), EndArguments);

        addSymbols(
            new SymbolicConstant(c, "NeutralMenuState", "int", Value(0)),
            new SymbolicConstant(c, "UncheckedMenuState", "int", Value(1)),
            new SymbolicConstant(c, "CheckedMenuState", "int", Value(2)),
            new SymbolicConstant(c, "MixedStateMenuState", "int", Value(3)),
            new SymbolicConstant(c, "DisabledMenuState", "int", Value(-1)),

            new Function(c, "eval", CommandsModule::eval, None, Parameters,
                         new Param(c, "text", "string"), Return, "string", End),

            new Function(c, "undo", CommandsModule::undo, None, Return, "void",
                         End),

            new Function(c, "redo", CommandsModule::redo, None, Return, "void",
                         End),

            new Function(c, "clearHistory", CommandsModule::clearHistory, None,
                         Return, "void", End),

            new Function(c, "beginCompoundCommand",
                         CommandsModule::beginCompoundCommand, None, Parameters,
                         new ParameterVariable(c, "name", "string"), Return,
                         "void", End),

            new Function(c, "endCompoundCommand",
                         CommandsModule::endCompoundCommand, None, Return,
                         "void", End),

            new Function(c, "undoHistory", CommandsModule::undoHistory, None,
                         Return, "string[]", End),

            new Function(c, "redoHistory", CommandsModule::redoHistory, None,
                         Return, "string[]", End),

            new Function(c, "activateMode", CommandsModule::activateMode, None,
                         Return, "void", Parameters,
                         new Param(c, "name", "string"), End),

            new Function(c, "isModeActive", CommandsModule::isModeActive, None,
                         Return, "bool", Parameters,
                         new Param(c, "name", "string"), End),

            new Function(c, "activeModes", CommandsModule::activeModes, None,
                         Return, "[string]", End),

            new Function(c, "deactivateMode", CommandsModule::deactivateMode,
                         None, Return, "void", Parameters,
                         new Param(c, "name", "string"), End),

            new Function(c, "defineMinorMode", CommandsModule::defineMinorMode,
                         None, Return, "void", Parameters,
                         new Param(c, "name", "string"),
                         new Param(c, "sortKey", "string", Value(Pointer(0))),
                         new Param(c, "order", "int", Value(0)), End),

            new Function(c, "bindingDocumentation", CommandsModule::bindDoc,
                         None, Return, "string", Parameters,
                         new Param(c, "eventName", "string"),
                         new Param(c, "modeName", "string", Value()),
                         new Param(c, "tableName", "string", Value()), End),

            new Function(c, "bindings", CommandsModule::bindings, None, Return,
                         "(string,string)[]", End),

            new Function(c, "bind", CommandsModule::bind, None, Return, "void",
                         Parameters, new Param(c, "modeName", "string"),
                         new Param(c, "tableName", "string"),
                         new Param(c, "eventName", "string"),
                         new Param(c, "func", "(void;Event)"),
                         new Param(c, "description", "string", Value()), End),

            new Function(c, "bindRegex", CommandsModule::bindRegex, None,
                         Return, "void", Parameters,
                         new Param(c, "modeName", "string"),
                         new Param(c, "tableName", "string"),
                         new Param(c, "eventPattern", "string"),
                         new Param(c, "func", "(void;Event)"),
                         new Param(c, "description", "string", Value()), End),

            new Function(c, "unbind", CommandsModule::unbind, None, Return,
                         "void", Parameters, new Param(c, "modeName", "string"),
                         new Param(c, "tableName", "string"),
                         new Param(c, "eventName", "string"), End),

            new Function(c, "unbindRegex", CommandsModule::unbindRegex, None,
                         Return, "void", Parameters,
                         new Param(c, "modeName", "string"),
                         new Param(c, "tableName", "string"),
                         new Param(c, "eventName", "string"), End),

            new Function(c, "setEventTableBBox", CommandsModule::setTableBBox,
                         None, Return, "void", Parameters,
                         new Param(c, "modeName", "string"),
                         new Param(c, "tableName", "string"),
                         new Param(c, "min", "vector float[2]"),
                         new Param(c, "max", "vector float[2]"), End),

            new Function(c, "defineModeMenu", CommandsModule::defineModeMenu,
                         None, Return, "void", Parameters,
                         new Param(c, "mode", "string"),
                         new Param(c, "menu", "MenuItem[]"),
                         new Param(c, "strict", "bool", Value(false)), End),

            // new Function(c, "isComputationInProgress",
            //              CommandsModule::isComputationInProgress, None,
            //              Return, "bool",
            //              End),

            // new Function(c, "computationMessage",
            //              CommandsModule::computationMessage, None,
            //              Return, "string",
            //              End),

            // new Function(c, "computationProgress",
            //              CommandsModule::computationProgress, None,
            //              Return, "float",
            //              End),

            // new Function(c, "computationElapsedTime",
            //              CommandsModule::computationElapsedTime, None,
            //              Return, "float",
            //              End),

            new Function(c, "pushEventTable", CommandsModule::pushEventTable,
                         None, Return, "void", Parameters,
                         new Param(c, "table", "string"), End),

            new Function(c, "popEventTable", CommandsModule::popEventTable,
                         None, Return, "void", End),

            new Function(c, "popEventTable", CommandsModule::popNamedEventTable,
                         None, Return, "void", Parameters,
                         new Param(c, "table", "string"), End),

            new Function(c, "activeEventTables",
                         CommandsModule::activeEventTables, None, Return,
                         "[string]", End),

            new Function(c, "contractSequences", CommandsModule::contractSeq,
                         None, Return, "string[]", Parameters,
                         new Param(c, "files", "string[]"), End),

            new Function(c, "sequenceOfFile", CommandsModule::sequenceOfFile,
                         None, Return, "(string,int)", Parameters,
                         new Param(c, "file", "string"), End),

            EndArguments);
    }

    static Document* currentDocument()
    {
        if (Document* d = Document::eventDocument())
        {
            return d;
        }
        else
        {
            return Document::activeDocument();
        }
    }

    static void throwBadArgumentException(const Mu::Node& node,
                                          Mu::Thread& thread, std::string msg)
    {
        ostringstream str;
        const Mu::MuLangContext* context =
            static_cast<const Mu::MuLangContext*>(thread.context());
        ExceptionType::Exception* e =
            new ExceptionType::Exception(context->exceptionType());
        str << "in " << node.symbol()->fullyQualifiedName() << ": " << msg;
        e->string() += str.str().c_str();
        thread.setException(e);
        BadArgumentException exc;
        exc.message() = e->string();
        throw exc;
    }

    NODE_IMPLEMENTATION(CommandsModule::eval, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const String* text = NODE_ARG_OBJECT(0, StringType::String);
        const StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        Context::ModuleList modules;
        String* s = stype->allocate(
            muEval(c, p, modules, text->c_str(), "commands.eval"));
        NODE_RETURN(s);
    }

    NODE_IMPLEMENTATION(CommandsModule::undo, void)
    {
        Document* d = currentDocument();
        d->undoCommand();
    }

    NODE_IMPLEMENTATION(CommandsModule::redo, void)
    {
        Document* d = currentDocument();
        d->redoCommand();
    }

    NODE_IMPLEMENTATION(CommandsModule::clearHistory, void)
    {
        Document* d = currentDocument();
        d->clearHistory();
    }

    NODE_IMPLEMENTATION(CommandsModule::beginCompoundCommand, void)
    {
        String* name = NODE_ARG_OBJECT(0, StringType::String);
        Document* d = currentDocument();
        d->beginCompoundCommand(name->c_str());
    }

    NODE_IMPLEMENTATION(CommandsModule::endCompoundCommand, void)
    {
        Document* d = currentDocument();
        d->endCompoundCommand();
    }

    NODE_IMPLEMENTATION(CommandsModule::readFile, void)
    {
        const String* name = NODE_ARG_OBJECT(0, StringType::String);
        bool import = NODE_ARG(1, bool);

        if (Document* d = currentDocument())
        {
            Document::ReadRequest request;
            request.setOption("append", import);
            d->read(name->c_str(), request);
        }
    }

    NODE_IMPLEMENTATION(CommandsModule::writeFile, void)
    {
        String* name = NODE_ARG_OBJECT(0, StringType::String);
        const bool partial = NODE_ARG(1, bool);

        if (Document* d = currentDocument())
        {
            Document::WriteRequest request;
            request.setOption("partial", partial);
            d->write(name->c_str(), request);
        }
    }

    namespace
    {

        void compileHistoryDescriptions(const TwkApp::CommandHistory* history,
                                        bool undo, vector<string>& array,
                                        size_t indent = 0)
        {
            const TwkApp::CommandStack& stack =
                undo ? history->undoStack() : history->redoStack();

            for (int i = 0; i < stack.size(); i++)
            {
                const TwkApp::Command* c = stack[i];

                ostringstream str;

                for (size_t q = 0; q < indent; q++)
                    str << ".";
                str << c->description() << " (" << c->name() << ")";

                array.push_back(str.str());

                if (const TwkApp::CompoundCommand* cc =
                        dynamic_cast<const TwkApp::CompoundCommand*>(stack[i]))
                {
                    compileHistoryDescriptions(cc, undo, array, indent + 1);
                }
            }
        }

    } // namespace

    NODE_IMPLEMENTATION(CommandsModule::undoHistory, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();

        const Mu::DynamicArrayType* atype =
            static_cast<const Mu::DynamicArrayType*>(NODE_THIS.type());
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(atype->elementType());

        DynamicArray* array = new DynamicArray(atype, 1);

        vector<string> desc;
        compileHistoryDescriptions(d, true, desc);
        array->resize(desc.size());

        for (int i = 0; i < desc.size(); i++)
        {
            array->element<String*>(i) = stype->allocate(desc[i]);
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(CommandsModule::redoHistory, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();

        const Mu::DynamicArrayType* atype =
            static_cast<const Mu::DynamicArrayType*>(NODE_THIS.type());
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(atype->elementType());

        DynamicArray* array = new DynamicArray(atype, 1);
        const TwkApp::CommandStack& redoStack = d->redoStack();
        array->resize(redoStack.size());

        for (int i = 0; i < redoStack.size(); i++)
        {
            const TwkApp::Command* c = redoStack[i];
            array->element<String*>(i) = stype->allocate(c->description());
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(CommandsModule::defineMinorMode, void)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();
        String* modeName = NODE_ARG_OBJECT(0, StringType::String);
        String* sortKey = NODE_ARG_OBJECT(1, StringType::String);
        int order = NODE_ARG(2, int);

        MinorMode* mode = new MinorMode(modeName->c_str(), d);
        mode->setOrder(order);
        if (sortKey)
            mode->setSortKey(sortKey->c_str());
        d->addMode(mode);
    }

    NODE_IMPLEMENTATION(CommandsModule::activateMode, void)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();
        String* modeName = NODE_ARG_OBJECT(0, StringType::String);

        if (!modeName)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "Nil argument to function");

        d->activateMode(modeName->c_str());
    }

    NODE_IMPLEMENTATION(CommandsModule::isModeActive, bool)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();
        bool b = false;

        if (String* modeName = NODE_ARG_OBJECT(0, StringType::String))
        {
            b = d->isModeActive(modeName->c_str());
        }
        else
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "Nil argument to function");
        }

        NODE_RETURN(b);
    }

    NODE_IMPLEMENTATION(CommandsModule::deactivateMode, void)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();
        String* modeName = NODE_ARG_OBJECT(0, StringType::String);
        d->deactivateMode(modeName->c_str());
    }

    NODE_IMPLEMENTATION(CommandsModule::bind, void)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();

        String* modeName = NODE_ARG_OBJECT(0, StringType::String);
        String* tableName = NODE_ARG_OBJECT(1, StringType::String);
        String* eventName = NODE_ARG_OBJECT(2, StringType::String);
        FunctionObject* obj = NODE_ARG_OBJECT(3, FunctionObject);
        String* docstring = NODE_ARG_OBJECT(4, StringType::String);

        string doc = docstring ? docstring->c_str() : "";

        if (Mode* mode = d->findModeByName(modeName->c_str()))
        {
            EventTable* table = mode->findTableByName(tableName->c_str());

            if (!table)
            {
                table = new EventTable(tableName->c_str());
                mode->addEventTable(table);
            }

            MuFuncAction* action = new MuFuncAction(obj, doc);
            table->bind(eventName->c_str(), action);
            //
            //  currentDocument has a copy of this table, so invalidate
            //  that copy.
            //
            d->invalidateEventTables();
        }
        else
        {
            string msg = "No mode named " + string(modeName->c_str());
            throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
        }
    }

    NODE_IMPLEMENTATION(CommandsModule::bindRegex, void)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();

        String* modeName = NODE_ARG_OBJECT(0, StringType::String);
        String* tableName = NODE_ARG_OBJECT(1, StringType::String);
        String* eventRegex = NODE_ARG_OBJECT(2, StringType::String);
        FunctionObject* obj = NODE_ARG_OBJECT(3, FunctionObject);
        String* docstring = NODE_ARG_OBJECT(4, StringType::String);

        string doc = docstring ? docstring->c_str() : "";

        if (Mode* mode = d->findModeByName(modeName->c_str()))
        {
            EventTable* table = mode->findTableByName(tableName->c_str());

            if (!table)
            {
                table = new EventTable(tableName->c_str());
                mode->addEventTable(table);
            }

            MuFuncAction* action = new MuFuncAction(obj, doc);
            table->bindRegex(eventRegex->c_str(), action);
            //
            //  currentDocument has a copy of this table, so invalidate
            //  that copy.
            //
            d->invalidateEventTables();
        }
        else
        {
            string msg = "No mode named " + string(modeName->c_str());
            throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
        }
    }

    NODE_IMPLEMENTATION(CommandsModule::unbind, void)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();

        String* modeName = NODE_ARG_OBJECT(0, StringType::String);
        String* tableName = NODE_ARG_OBJECT(1, StringType::String);
        String* eventName = NODE_ARG_OBJECT(2, StringType::String);

        if (!modeName || !tableName || !eventName)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "Nil argument to function");
        }

        if (Mode* mode = d->findModeByName(modeName->c_str()))
        {
            if (EventTable* table = mode->findTableByName(tableName->c_str()))
            {
                table->unbind(eventName->c_str());
                //
                //  currentDocument has a copy of this table, so invalidate
                //  that copy.
                //
                d->invalidateEventTables();
            }
            else
            {
                string msg = "No table named " + string(tableName->c_str());
                throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
            }
        }
        else
        {
            string msg = "No mode named " + string(modeName->c_str());
            throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
        }
    }

    NODE_IMPLEMENTATION(CommandsModule::unbindRegex, void)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();

        String* modeName = NODE_ARG_OBJECT(0, StringType::String);
        String* tableName = NODE_ARG_OBJECT(1, StringType::String);
        String* eventName = NODE_ARG_OBJECT(2, StringType::String);

        if (!modeName || !tableName || !eventName)
        {
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "Nil argument to function");
        }

        if (Mode* mode = d->findModeByName(modeName->c_str()))
        {
            if (EventTable* table = mode->findTableByName(tableName->c_str()))
            {
                table->unbindRegex(eventName->c_str());
                //
                //  currentDocument has a copy of this table, so invalidate
                //  that copy.
                //
                d->invalidateEventTables();
            }
            else
            {
                string msg = "No table named " + string(tableName->c_str());
                throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
            }
        }
        else
        {
            string msg = "No mode named " + string(modeName->c_str());
            throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
        }
    }

    NODE_IMPLEMENTATION(CommandsModule::bindings, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();
        const Mu::MuLangContext* c =
            static_cast<const Mu::MuLangContext*>(NODE_THREAD.context());

        const DynamicArrayType* atype =
            (const DynamicArrayType*)NODE_THIS.type();
        const Class* ttype = (const Class*)atype->elementType();
        DynamicArray* array = new DynamicArray(atype, 1);

        struct StringTuple
        {
            String* a;
            String* b;
        };

        const Document::EventTableStack& stack = d->eventTableStack();

        for (int i = stack.size() - 1; i >= 0; i--)
        {
            const EventTable* table = stack[i];
            typedef EventTable::BindingMap::const_iterator iter;
            typedef EventTable::BindingMap::value_type vtype;

            for (iter i = table->begin(); i != table->end(); ++i)
            {

                String* name = c->stringType()->allocate(i->first);
                String* doc = c->stringType()->allocate(i->second->docString());

                ClassInstance* tuple = ClassInstance::allocate(ttype);
                StringTuple* st =
                    reinterpret_cast<StringTuple*>(tuple->structure());

                st->a = name;
                st->b = doc;

                array->resize(array->size() + 1);
                array->element<ClassInstance*>(array->size() - 1) = tuple;
            }
        }

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(CommandsModule::bindDoc, Mu::Pointer)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();
        const Mu::MuLangContext* c =
            static_cast<const Mu::MuLangContext*>(NODE_THREAD.context());

        String* eventName = NODE_ARG_OBJECT(0, StringType::String);
        String* modeName = NODE_ARG_OBJECT(1, StringType::String);
        String* tableName = NODE_ARG_OBJECT(2, StringType::String);

        if (modeName && tableName)
        {
            if (Mode* mode = d->findModeByName(modeName->c_str()))
            {
                const EventTable* table =
                    mode->findTableByName(tableName->c_str());

                if (!table)
                {
                    string msg = "No table named " + string(tableName->c_str());
                    throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
                }

                if (const Action* action = table->query(eventName->c_str()))
                {
                    String* doc =
                        c->stringType()->allocate(action->docString());
                    NODE_RETURN(doc);
                }
                else
                {
                    string msg = "Not bound: " + string(eventName->c_str());
                    throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
                }
            }
            else
            {
                string msg = "No mode named " + string(modeName->c_str());
                throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
            }
        }
        else
        {
            const Document::EventTableStack& stack = d->eventTableStack();

            for (int i = stack.size() - 1; i >= 0; i--)
            {
                const EventTable* table = stack[i];

                if (const Action* action = table->query(eventName->c_str()))
                {
                    String* doc =
                        c->stringType()->allocate(action->docString());
                    NODE_RETURN(doc);
                }
            }
        }

        NODE_RETURN(0);
    }

    NODE_IMPLEMENTATION(CommandsModule::setTableBBox, void)
    {
        Process* p = NODE_THREAD.process();
        Document* d = currentDocument();
        String* modeName = NODE_ARG_OBJECT(0, StringType::String);
        String* tableName = NODE_ARG_OBJECT(1, StringType::String);
        Vector2f min = NODE_ARG(2, Vector2f);
        Vector2f max = NODE_ARG(3, Vector2f);

        TwkMath::Box2i bbox;
        bbox.min.x = int(min[0]);
        bbox.min.y = int(min[1]);
        bbox.max.x = int(max[0]);
        bbox.max.y = int(max[1]);

        if (Mode* mode = d->findModeByName(modeName->c_str()))
        {
            EventTable* table = mode->findTableByName(tableName->c_str());

            if (!table)
            {
                table = new EventTable(tableName->c_str());
                mode->addEventTable(table);
            }

            TwkMath::Box2i old = table->bbox();
            if (old != bbox)
            {
                table->setBBox(bbox);
                //
                //  currentDocument has a copy of this table, so invalidate
                //  that copy.
                //
                d->invalidateEventTables();
            }
        }
        else
        {
            string msg = "No mode named " + string(modeName->c_str());
            throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
        }
    }

    NODE_IMPLEMENTATION(CommandsModule::defineModeMenu, void)
    {
        StringType::String* modeName = NODE_ARG_OBJECT(0, StringType::String);
        DynamicArray* array = NODE_ARG_OBJECT(1, DynamicArray);
        bool strict = NODE_ARG(2, bool);

        if (array)
        {
            Document* d = currentDocument();
            Menu* menu = createTwkAppMenu("Main", array);

            if (Mode* m = d->findModeByName(modeName->c_str()))
            {
                if (strict)
                    m->setMenu(menu);
                else
                    m->merge(menu);

                //
                //  If we are re-defining an existing menu, we need to
                //  invalidate the old menu.
                //
                d->invalidateMenu();
            }
            else
            {
                string msg = "No mode named " + string(modeName->c_str());
                throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
            }
        }
    }

    // NODE_IMPLEMENTATION(CommandsModule::isComputationInProgress, bool)
    // {
    //     NODE_RETURN(Interrupt::isWorking());
    // }

    // NODE_IMPLEMENTATION(CommandsModule::computationMessage, Mu::Pointer)
    // {
    //     Process *p   = NODE_THREAD.process();
    //     const Mu::StringType* stype =
    //         static_cast<const Mu::StringType*>(NODE_THIS.type());

    //     if (!Interrupt::isWorking())
    //     {
    //         string msg = "computation not in progress: invalid call";
    //         throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
    //     }

    //     String* s = stype->allocate(Interrupt::currentComputation().message);
    //     NODE_RETURN(s);
    // }

    // NODE_IMPLEMENTATION(CommandsModule::computationProgress, float)
    // {
    //     if (!Interrupt::isWorking())
    //     {
    //         string msg = "computation not in progress: invalid call";
    //         throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
    //     }

    //     NODE_RETURN(Interrupt::currentComputation().percentDone);
    // }

    // NODE_IMPLEMENTATION(CommandsModule::computationElapsedTime, float)
    // {
    //     if (!Interrupt::isWorking())
    //     {
    //         string msg = "computation not in progress: invalid call";
    //         throwBadArgumentException(NODE_THIS, NODE_THREAD, msg);
    //     }

    //     NODE_RETURN(Interrupt::currentComputation().timer.elapsed());
    // }

    NODE_IMPLEMENTATION(CommandsModule::pushEventTable, void)
    {
        String* name = NODE_ARG_OBJECT(0, StringType::String);
        Document* d = currentDocument();
        d->pushTable(name->c_str());
    }

    NODE_IMPLEMENTATION(CommandsModule::popEventTable, void)
    {
        Document* d = currentDocument();
        d->popTable();
    }

    NODE_IMPLEMENTATION(CommandsModule::popNamedEventTable, void)
    {
        String* name = NODE_ARG_OBJECT(0, StringType::String);
        Document* d = currentDocument();
        d->popTable(name->c_str());
    }

    NODE_IMPLEMENTATION(CommandsModule::activeEventTables, Mu::Pointer)
    {
        Document* d = currentDocument();
        const Document::EventTableStack& tables = d->eventTableStack();

        Process* p = NODE_THREAD.process();
        const ListType* ltype = static_cast<const ListType*>(NODE_THIS.type());
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(ltype->elementType());
        Mu::List list(p, ltype);

        for (int i = 0; i < tables.size(); i++)
        {
            StringType::String* str = stype->allocate(tables[i]->name());
            list.append(str);
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(CommandsModule::contractSeq, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        const StringType* stype =
            static_cast<const StringType*>(atype->elementType());
        DynamicArray* inArray = NODE_ARG_OBJECT(0, DynamicArray);

        if (!inArray)
            throwBadArgumentException(NODE_THIS, NODE_THREAD,
                                      "Nil argument to function");

        if (!inArray->size())
            NODE_RETURN(inArray);

        DynamicArray* outArray = new DynamicArray(atype, 1);
        FileNameList files(inArray->size());

        for (int i = 0; i < files.size(); i++)
        {
            files[i] = inArray->element<StringType::String*>(i)->c_str();
        }

        SequenceNameList seqlist =
            sequencesInFileList(files, GlobalExtensionPredicate);

        outArray->resize(seqlist.size());

        for (int i = 0; i < seqlist.size(); i++)
        {
            outArray->element<StringType::String*>(i) =
                stype->allocate(seqlist[i]);
        }

        NODE_RETURN(outArray);
    }

    NODE_IMPLEMENTATION(CommandsModule::activeModes, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Mu::MuLangContext* c =
            static_cast<const Mu::MuLangContext*>(NODE_THREAD.context());

        const Mu::ListType* listType =
            static_cast<const Mu::ListType*>(NODE_THIS.type());
        const StringType* stype =
            static_cast<const StringType*>(listType->elementType());
        Document* d = currentDocument();

        List list(p, listType);
        list.append(stype->allocate(d->majorMode()->name()));

        const Document::MinorModes& minorModes = d->minorModes();

        for (Document::MinorModes::const_iterator i = minorModes.begin();
             i != minorModes.end(); ++i)
        {
            const MinorMode* m = *i;
            list.append(stype->allocate(m->name()));
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(CommandsModule::sequenceOfFile, Pointer)
    {
        Process* p = NODE_THREAD.process();
        String* file = NODE_ARG_OBJECT(0, StringType::String);
        const StringType* stype = static_cast<const StringType*>(file->type());
        const Class* ttype = static_cast<const Class*>(NODE_THIS.type());

        struct PatTuple
        {
            StringType::String* file;
            int frame;
        };

        string filename = file->c_str();
        PatternFramePair pp =
            TwkUtil::sequenceOfFile(filename, GlobalExtensionPredicate);

        ClassInstance* tuple = ClassInstance::allocate(ttype);
        PatTuple* pt = reinterpret_cast<PatTuple*>(tuple->structure());
        pt->file = stype->allocate(pp.first);
        pt->frame = pp.second;

        NODE_RETURN(tuple);
    }

} // namespace TwkApp
