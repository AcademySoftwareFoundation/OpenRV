//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MuTwkApp/MuInterface.h>
#include <MuTwkApp/CommandsModule.h>
#include <MuTwkApp/SettingsValueType.h>
#include <MuTwkApp/EventType.h>
#include <MuTwkApp/FunctionAction.h>
#include <MuTwkApp/MenuItem.h>
#include <MuTwkApp/MenuState.h>
#include <Mu/Function.h>
#include <Mu/MachineRep.h>
#include <Mu/NodeAssembler.h>
#include <Mu/NodePrinter.h>
#include <Mu/MuProcess.h>
#include <Mu/GarbageCollector.h>
#include <Mu/SymbolTable.h>
#include <Mu/Thread.h>
#include <Mu/Type.h>
#include <Mu/Value.h>
// #include <MuAutoDoc/AutoDocModule.h>
// #include <MuIO/IOModule.h>
// #include <MuImage/ImageModule.h>
// #include <MuEncoding/EncodingModule.h>
// #include <MuMathLinear/MathLinearModule.h>
// #include <MuSystem/SystemModule.h>
#include <MuLang/InteractiveSession.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/MuLangLanguage.h>
#include <MuLang/Parse.h>
#include <MuLang/StringType.h>
#include <TwkApp/Menu.h>
#include <TwkApp/Document.h>
#include <TwkUtil/Timer.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace Mu;
using namespace std;
using namespace TwkUtil;

//
//  This will intialize the garbage collector among other things.
//

GenericMachine* g_machine = 0;

//
//  For complete saftey (because we don't know the order in which
//  static initialization occurs), these are allocated just before
//  first use. We don't want to call an GC functions before its
//  initialized.
//

static Context::ModuleList g_modules;
static MuLangContext* g_context = 0;
static MuLangLanguage* g_language = 0;
static Process* g_process = 0;
static Thread* g_appThread = 0;

//
//  This should to be a container searchable by the GC
//

struct ThreadMapEntry
{
    pthread_t posixThread;
    Thread* muThread;
};

typedef Mu::STLVector<ThreadMapEntry>::Type ThreadMap;
static ThreadMap threadmap;

extern const char* mu_commands;

namespace TwkApp
{

    static bool debugging = false;

    CallEnv::CallEnv(Document* d)
        : Mu::CallEnvironment()
        , _doc(d)
    {
    }

    CallEnv::~CallEnv() {}

    const Value CallEnv::call(const Function* F,
                              Function::ArgumentVector& args) const
    {
        if (_doc)
        {
            Document* current = Document::activeDocument();
            if (current != _doc)
                _doc->makeActive();
            const Value v = muProcess()->call(muAppThread(), F, args);
            if (current != _doc)
                current->makeActive();
            return v;
        }
        else
        {
            return Value(Pointer(0));
        }
    }

    const Value CallEnv::callMethodByName(const char* Fname,
                                          Function::ArgumentVector& args) const
    {
        if (_doc)
        {
            Document* current = Document::activeDocument();
            if (current != _doc)
                _doc->makeActive();
            const Value v = muAppThread()->callMethodByName(Fname, args, false);
            if (current != _doc)
                current->makeActive();
            return v;
        }
        else
        {
            return Value(Pointer(0));
        }
    }

    const Context* CallEnv::context() const { return muContext(); }

    //----------------------------------------------------------------------

    void setDebugging(bool b)
    {
        debugging = true;
        if (g_context)
            g_context->debugging(true);
    }

    void setCompileOnDemand(bool b)
    {
        Module::setCompileOnDemand(b, b); // sets both mud and muc output
    }

    void setDebugMUC(bool b) { Module::setDebugArchive(b); }

    bool isDebuggingOn()
    {
        return g_context ? g_context->debugging() : debugging;
    }

    Context::ModuleList& muModuleList() { return g_modules; }

    Mu::MuLangContext* muContext() { return g_context; }

    Mu::Process* muProcess() { return g_process; }

    Mu::Thread* muAppThread()
    {
        for (int i = 0; i < threadmap.size(); i++)
        {
            if (pthread_equal(threadmap[i].posixThread, pthread_self()))
            {
                return threadmap[i].muThread;
            }
        }

        ThreadMapEntry entry;
        entry.posixThread = pthread_self();
        entry.muThread = muProcess()->newApplicationThread();
        threadmap.push_back(entry);
        return entry.muThread;
    }

    // alternative to readline

    const char* getline(const char* prompt)
    {
        static char buffer[1024];
        size_t max_size = 1024;

        cout << prompt << flush;

        int i = 0;

        for (; i < max_size; i++)
        {
            if (!cin.eof() && !cin.fail())
            {
                char c;
                cin.get(c);
                if (c == '\n')
                {
                    buffer[i] = 0;
                    return buffer;
                }
                else
                {
                    buffer[i] = c;
                }
            }
            else
            {
                break;
            }
        }

        if (i)
        {
            if (i != max_size)
                i--;
            return buffer;
        }
        else
        {
            return 0;
        }
    }

    //----------------------------------------------------------------------

    std::string muEval(MuLangContext* context, Process* process,
                       const Context::ModuleList& modules, const char* line,
                       const char* contextName, bool showType)
    {
        ostringstream str;

        if (*line)
        {
            try
            {
                Mu::TypedValue value =
                    context->evalText(line, contextName, process, modules);

                if (value._type && value._type != context->voidType())
                {
                    if (showType)
                    {
                        value._type->output(str);
                        str << " => ";
                    }

                    value._type->outputValue(str, value._value);
                }
            }
            catch (Mu::TypedValue value)
            {
                if (Mu::ExceptionType::Exception* e =
                        (Mu::ExceptionType::Exception*)value._value._Pointer)
                {
                    str << "ERROR: " << e->string() << endl;
                    cerr << e->backtraceAsString() << endl;
                }
            }
            catch (std::exception& e)
            {
                cout << "ERROR: uncaught exception: " << e.what() << endl;
            }
        }

        return str.str();
    }

    std::string muEvalStringExpr(MuLangContext* context, Process* process,
                                 const Context::ModuleList& modules,
                                 const char* line, const char* contextName)
    {
        ostringstream str;

        if (*line)
        {
            try
            {
                Mu::TypedValue value =
                    context->evalText(line, contextName, process, modules);

                if (value._type && value._type == context->stringType())
                {
                    StringType::String* s =
                        reinterpret_cast<StringType::String*>(
                            value._value._Pointer);
                    str << s->c_str();
                }
            }
            catch (Mu::TypedValue value)
            {
                if (Mu::ExceptionType::Exception* e =
                        (Mu::ExceptionType::Exception*)value._value._Pointer)
                {
                    str << "ERROR: " << e->string() << endl;
                    cerr << e->backtraceAsString() << endl;
                }
            }
            catch (std::exception& e)
            {
                cout << "ERROR: uncaught exception: " << e.what() << endl;
            }
        }

        return str.str();
    }

    void cli()
    {
        cout << "Type `help()' for a list of commands." << endl;
        cout << "or `help(\"name of command\")' for help on a specific command."
             << endl;

        while (1)
        {
            const char* line;
            line = getline("rv> ");

            if (!line)
                exit(0);
            if (!*line)
                continue;

            // add_history(line);

            string command = line;
            command += ";";

            try
            {
                Mu::TypedValue value = g_context->evalText(
                    command.c_str(), "input", g_process, g_modules);

                if (value._type && value._type != g_context->voidType())
                {
                    value._type->output(cout);
                    cout << " => ";
                    value._type->outputValue(cout, value._value);
                    cout << endl << flush;
                }
            }
            catch (Mu::TypedValue value)
            {
                if (Mu::ExceptionType::Exception* e =
                        (Mu::ExceptionType::Exception*)value._value._Pointer)
                {
                    cout << "ERROR: " << e->string() << endl;
                    cerr << e->backtraceAsString() << endl;
                }
            }
            catch (std::exception& e)
            {
                cout << "ERROR: uncaught exception: " << e.what() << endl;
            }
        }
    }

    void batch(MuLangContext* context, Process* process,
               const Context::ModuleList& modules, const char* filename)
    {
        NodeAssembler as(context, process);

        for (int i = 0; i < modules.size(); i++)
        {
            as.pushScope((Module*)modules[i], false);
        }

        ifstream in(UNICODE_C_STR(filename));
        context->setInput(in);

        if (Process* p = Parse(filename, &as))
        {
            if (p->rootNode())
            {
                Thread* thread = muAppThread();
                Value v = p->evaluate(thread);

                if (thread->uncaughtException())
                {
                    if (Mu::ExceptionType::Exception* e =
                            (Mu::ExceptionType::Exception*)thread->exception())

                    {
                        cerr << "ERROR: " << e->string() << endl;
                        cerr << e->backtraceAsString() << endl;
                    }

                    exit(-1);
                }
            }
        }
        else
        {
            exit(-1);
        }
    }

    MuLangContext* newMuContext(const char* batchFile, GCFilterFunc gc_filter,
                                Context::ModuleList& modules)
    {
#ifdef PLATFORM_DARWIN
        GarbageCollector::init();
        if (gc_filter != 0)
            GC_register_has_static_roots_callback(gc_filter);
#endif

        if (!g_machine)
            g_machine = new GenericMachine();
        if (!g_language)
            g_language = new MuLangLanguage;

        MuLangContext* context = new MuLangContext(
            batchFile ? "batch" : "cli", batchFile ? batchFile : "input");

        context->debugging(debugging);

        // Module* autodoc  = new Mu::AutoDocModule(context, "autodoc");
        // Module* io       = new Mu::IOModule(context, "io");
        // Module* sys      = new Mu::SystemModule(context, "system");
        // Module* img      = new Mu::ImageModule(context, "image");
        // Module* enc      = new Mu::EncodingModule(context, "encoding");
        // Module* lin      = new Mu::MathLinearModule(context, "math_linear");
        Module* commands = new CommandsModule(context, "commands");

        context->globalScope()->addSymbol(new EventType(context));
        context->globalScope()->addSymbol(new SettingsValueType(context));
        // context->globalScope()->addSymbol(autodoc);
        // context->globalScope()->addSymbol(io);
        // context->globalScope()->addSymbol(sys);
        // context->globalScope()->addSymbol(img);
        // context->globalScope()->addSymbol(enc);
        // context->globalScope()->addSymbol(lin);
        context->globalScope()->addSymbol(commands);

        modules.push_back(commands);
        // modules.push_back(autodoc);
        // modules.push_back(io);
        modules.push_back(context->mathModule());
        modules.push_back(context->mathUtilModule());

        return context;
    }

    void initMu(const char* batchFile, GCFilterFunc gc_filter)
    {
        if (!g_context)
        {
            g_context = newMuContext(batchFile, gc_filter, g_modules);
        }

        if (!g_process)
        {
            g_process = new Process(g_context);
        }

        if (!g_appThread)
        {
            g_appThread = muAppThread();
        }
    }

    void initWithString(MuLangContext* context, Process* process,
                        const Context::ModuleList& modules, const char* p)
    {
        try
        {
            // Timer t;
            context->evalText(p, "initWithString", process, modules);
            // cout << "EVAL: " << t.elapsed() << " seconds" << endl;
        }
        catch (StreamOpenFailureException)
        {
            // Not an error if no file exists
        }
    }

    void initWithFile(MuLangContext* context, Process* process,
                      const Context::ModuleList& modules, const char* file)
    {
        try
        {
            // Timer t;
            context->evalFile(file, process, modules);
            // cout << "EVAL: " << t.elapsed() << " seconds" << endl;
        }
        catch (StreamOpenFailureException)
        {
            // Not an error if no file exists
        }
    }

    void initRc(MuLangContext* context, Process* process,
                const Context::ModuleList& modules, const char* rcfile)
    {
        ostringstream str;
        str << getenv("HOME") << rcfile;
        initWithFile(context, process, modules, str.str().c_str());
    }

    Menu* createTwkAppMenu(const std::string& name, DynamicArray* array)
    {
        size_t size = array->size();
        Menu* menu = new Menu(name);

        for (int q = 0; q < size; q++)
        {
            if (ClassInstance* i = array->element<ClassInstance*>(q))
            {
                MenuItem::Struct* s = i->data<MenuItem::Struct>();
                Menu* subMenu = 0;

                if (s->subMenu)
                {
                    subMenu = createTwkAppMenu(s->label->c_str(), s->subMenu);
                }

                MuFuncAction* action =
                    s->actionCB ? new MuFuncAction(s->actionCB) : 0;
                MuStateFunc* sfunc =
                    s->stateCB ? new MuStateFunc(s->stateCB) : 0;

                menu->addItem(new Menu::Item(s->label->c_str(), action,
                                             s->key ? s->key->c_str() : "",
                                             sfunc, subMenu));
            }
        }

        return menu;
    }

    void runMuInterative()
    {
        InteractiveSession session;
        session.run(muContext(), muProcess(), muAppThread());
    }

} // namespace TwkApp
