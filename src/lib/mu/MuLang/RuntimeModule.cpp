//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/RuntimeModule.h>
#include <MuLang/NameType.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/List.h>
#include <Mu/ListType.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Signature.h>
#include <Mu/Symbol.h>
#include <Mu/SymbolTable.h>
#include <Mu/SymbolType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/TypeModifier.h>
#include <Mu/TupleType.h>
#include <Mu/VariantType.h>
#include <Mu/VariantTagType.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/Native.h>
#include <MuLang/StringType.h>
#include <algorithm>
#include <sstream>
#include <stdlib.h>
#include <vector>

namespace Mu
{
    using namespace std;
    static vector<FunctionObject*> gc_callbacks;
    static vector<Thread*> gc_callback_threads;

    static void gc_cb()
    {
        for (int i = 0; i < gc_callbacks.size(); i++)
        {
            FunctionObject* o = gc_callbacks[i];
            Thread* thread = gc_callback_threads[i];

            const FunctionType* ftype =
                static_cast<const FunctionType*>(o->type());

            const Function* f = o->function();
            Node node(0, f);
            NodeFunc nf = f->func(&node);
            (*nf._voidFunc)(node, *thread);
            node.releaseArgv();
        }
    }

    void RuntimeModule::init()
    {
        static bool initialized = false;

        if (!initialized)
        {
            GarbageCollector::addCollectCallback(gc_cb);
            initialized = true;
        }
    }

    RuntimeModule::RuntimeModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    RuntimeModule::~RuntimeModule() {}

    void RuntimeModule::load()
    {
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        USING_MU_FUNCTION_SYMBOLS;

        Type* nt = new NameType(context);
        Type* st = new SymbolType(context, "symbol");
        Type* tt = new TypeSymbolType(context, "type_symbol");
        Type* ft = new FunctionSymbolType(context, "function_symbol");
        Type* vt = new ParameterSymbolType(context, "variable_symbol");
        Type* pt = new ParameterSymbolType(context, "parameter_symbol");
        Type* sct = new ParameterSymbolType(context, "symbolic_constant");

        addSymbols(nt, st, tt, ft, vt, pt, sct, EndArguments);
        context->listType(st);
        context->listType(context->intType());
        Context::TypeVector types(3);
        types[0] = tt;
        types[1] = context->listType(tt);
        types[2] = context->listType(pt);
        context->tupleType(types);

        types.resize(2);
        types[0] = context->listType(ft);
        types[1] = context->listType(vt);
        context->tupleType(types);

        //
        //  Make the function type (void;)
        //

        Signature* sig = new Signature;
        sig->push_back(context->voidType());
        FunctionType* voidFuncType = context->functionType(sig);

        Context::TypeVector ttypes(6);
        ttypes[0] = ttypes[1] = context->stringType();
        ttypes[2] = ttypes[3] = ttypes[4] = ttypes[5] = context->intType();

        context->listType(context->stringType()); // [string]
        context->listType(context->tupleType(ttypes));

        ttypes.resize(2);
        ttypes[0] = context->stringType();
        ttypes[1] = context->stringType();
        context->listType(context->tupleType(ttypes));

        context->functionType("(void;string,int64)");
        Context* c = context;

        Module* gc = new Module(context, "gc");

        gc->addSymbols(
            new Function(c, "perform_collection",
                         RuntimeModule::gc_perform_collection, None, Return,
                         "void", End),

            new Function(c, "parallel_enabled",
                         RuntimeModule::gc_parallel_enabled, None, Return,
                         "bool", End),

            new Function(c, "all_interior_pointers",
                         RuntimeModule::gc_all_interior_pointers, None, Return,
                         "bool", End),

            new Function(c, "num_collections",
                         RuntimeModule::gc_num_collections, None, Return, "int",
                         End),

            new Function(c, "call_on_collect",
                         RuntimeModule::gc_call_on_collect, None, Return,
                         "void", Args, "(void;)", End),

            new Function(c, "dump", RuntimeModule::gc_dump, None, Return,
                         "void", End),

            new Function(c, "enable", RuntimeModule::gc_enable, None, Return,
                         "void", End),

            new Function(c, "disable", RuntimeModule::gc_disable, None, Return,
                         "void", End),

            new Function(c, "get_heap_size", RuntimeModule::gc_get_heap_size,
                         None, Return, "int64", End),

            new Function(c, "get_free_bytes", RuntimeModule::gc_get_free_bytes,
                         None, Return, "int64", End),

            new Function(c, "get_bytes_since_gc",
                         RuntimeModule::gc_get_bytes_since_gc, None, Return,
                         "int64", End),

            new Function(c, "get_total_bytes",
                         RuntimeModule::gc_get_total_bytes, None, Return,
                         "int64", End),

            new Function(
                c, "set_warning_function",
                RuntimeModule::gc_set_warning_function, None, Return,
                "(void;string,int64)", Parameters,
                new ParameterVariable(c, "func", "(void;string,int64)"), End),

            new Function(c, "push_api", RuntimeModule::gc_push_api, None,
                         Return, "void", Parameters,
                         new ParameterVariable(c, "api", "int"), End),

            new Function(c, "pop_api", RuntimeModule::gc_pop_api, None, Return,
                         "void", End),

            EndArguments);

        addSymbols(
            gc,

            new Function(c, "eval", RuntimeModule::eval, None, Return, "string",
                         Parameters, new ParameterVariable(c, "text", "string"),
                         new ParameterVariable(c, "module_list", "[string]"),
                         End),

            new Function(c, "varying_size", RuntimeModule::varying_size, None,
                         Return, "int", Args, "int", End),

            new Function(c, "set_varying_size", RuntimeModule::set_varying_size,
                         None, Return, "void", Args, "int", "int", End),

            new Function(
                c, "dump_symbols", RuntimeModule::dump_symbols, None, Return,
                "string", Parameters,
                new ParameterVariable(c, "symbol_name", "string", Value()),
                new ParameterVariable(c, "primary_only", "bool", Value(false)),
                End),

            new Function(c, "layout_traits", RuntimeModule::layout_traits, None,
                         Return, "[int]", End),

            new Function(c, "machine_types", RuntimeModule::machine_types, None,
                         Return, "[(string,string,int,int,int,int)]", End),

            new Function(
                c, "exit", RuntimeModule::exit, None, Parameters,
                new ParameterVariable(c, "exit_value", "int", Value(0)), Return,
                "void", End),

            new Function(c, "stack_traits", RuntimeModule::stack_traits, None,
                         Return, "[int]", End),

            new Function(c, "module_locations", RuntimeModule::module_locations,
                         None, Return, "[(string,string)]", End),

            new Function(c, "backtrace", RuntimeModule::backtrace, None, Return,
                         "[string]", End),

            new Function(c, "load_module", RuntimeModule::load_module, None,
                         Return, "bool", Parameters,
                         new ParameterVariable(c, "module_name", "string"),
                         End),

            new Function(c, "intern_name", RuntimeModule::intern_name, None,
                         Return, "runtime.name", Parameters,
                         new ParameterVariable(c, "name", "string"), End),

            new Function(c, "lookup_name", RuntimeModule::lookup_name, None,
                         Return, "runtime.name", Parameters,
                         new ParameterVariable(c, "name", "string"), End),

            new Function(c, "lookup_function", RuntimeModule::lookup_function,
                         None, Return, "(;)", Parameters,
                         new ParameterVariable(c, "name", "runtime.name"), End),

            new Function(c, "build_os", RuntimeModule::build_os, None, Return,
                         "string", End),

            new Function(c, "build_architecture", RuntimeModule::build_arch,
                         None, Return, "string", End),

            new Function(c, "build_compiler", RuntimeModule::build_compiler,
                         None, Return, "string", End),

            new Function(c, "symbol_from_name", RuntimeModule::symbol_from_name,
                         None, Return, "runtime.symbol", Parameters,
                         new ParameterVariable(c, "name", "runtime.name"), End),

            new Function(c, "symbol_is_nil", RuntimeModule::symbol_is_nil, None,
                         Return, "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(c, "symbol_is_type", RuntimeModule::symbol_is_type,
                         None, Return, "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(c, "symbol_is_module", RuntimeModule::symbol_is_module,
                         None, Return, "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(c, "symbol_is_symbolic_constant",
                         RuntimeModule::symbol_is_symbolic_constant, None,
                         Return, "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(
                c, "symbol_is_function", RuntimeModule::symbol_is_function,
                None, Return, "bool", Parameters,
                new ParameterVariable(c, "sym", "runtime.symbol"), End),

            new Function(c, "symbol_is_method", RuntimeModule::symbol_is_method,
                         None, Return, "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(
                c, "symbol_is_parameter", RuntimeModule::symbol_is_parameter,
                None, Return, "bool", Parameters,
                new ParameterVariable(c, "sym", "runtime.symbol"), End),

            new Function(c, "symbol_is_type_modifier",
                         RuntimeModule::symbol_is_type_modifier, None, Return,
                         "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(
                c, "symbol_is_variable", RuntimeModule::symbol_is_variable,
                None, Return, "bool", Parameters,
                new ParameterVariable(c, "sym", "runtime.symbol"), End),

            new Function(c, "symbol", RuntimeModule::cast_to_symbol, Cast,
                         Return, "runtime.symbol", Parameters,
                         new ParameterVariable(c, "typ", "runtime.type_symbol"),
                         End),

            new Function(
                c, "symbol", RuntimeModule::cast_to_symbol, Cast, Parameters,
                new ParameterVariable(c, "func", "runtime.function_symbol"),
                Return, "runtime.symbol", End),

            new Function(
                c, "symbol", RuntimeModule::cast_to_symbol, Cast, Parameters,
                new ParameterVariable(c, "func", "runtime.parameter_symbol"),
                Return, "runtime.symbol", End),

            new Function(
                c, "symbol", RuntimeModule::cast_to_symbol, Cast, Parameters,
                new ParameterVariable(c, "func", "runtime.variable_symbol"),
                Return, "runtime.symbol", End),

            new Function(
                c, "variable_symbol", RuntimeModule::cast_to_symbol, Cast,
                Parameters,
                new ParameterVariable(c, "func", "runtime.parameter_symbol"),
                Return, "runtime.variable_symbol", End),

            new Function(c, "type_from_symbol", RuntimeModule::type_from_symbol,
                         None, Return, "runtime.type_symbol", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(
                c, "function_from_symbol", RuntimeModule::function_from_symbol,
                None, Return, "runtime.function_symbol", Parameters,
                new ParameterVariable(c, "sym", "runtime.symbol"), End),

            new Function(c, "parameter_from_symbol",
                         RuntimeModule::parameter_from_symbol, None, Return,
                         "runtime.parameter_symbol", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(c, "symbol_scope", RuntimeModule::symbol_scope, None,
                         Return, "runtime.symbol", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(c, "symbol_symbols_in_scope",
                         RuntimeModule::symbol_symbols_in_scope, None, Return,
                         "[runtime.symbol]", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(c, "symbol_name", RuntimeModule::symbol_name, None,
                         Return, "string", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(c, "symbol_fully_qualified_name",
                         RuntimeModule::symbol_fully_qualified_name, None,
                         Return, "string", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(
                c, "symbol_documentation", RuntimeModule::symbol_documentation,
                None, Return, "string", Parameters,
                new ParameterVariable(c, "sym", "runtime.symbol"), End),

            new Function(c, "symbol_overloaded_symbols",
                         RuntimeModule::symbol_overloaded_symbols, None, Return,
                         "[runtime.symbol]", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(
                c, "function_type", RuntimeModule::function_type, None, Return,
                "runtime.type_symbol", Parameters,
                new ParameterVariable(c, "func", "runtime.function_symbol"),
                End),

            new Function(
                c, "function_signature", RuntimeModule::function_signature,
                None, Return,
                "(runtime.type_symbol,[runtime.type_symbol],[runtime.parameter_"
                "symbol])",
                Parameters,
                new ParameterVariable(c, "func", "runtime.function_symbol"),
                End),

            new Function(
                c, "variable_from_symbol", RuntimeModule::variable_from_symbol,
                None, Return, "runtime.variable_symbol", Parameters,
                new ParameterVariable(c, "sym", "runtime.symbol"), End),

            new Function(
                c, "variable_type", RuntimeModule::variable_type, None, Return,
                "runtime.type_symbol", Parameters,
                new ParameterVariable(c, "sym", "runtime.variable_symbol"),
                End),

            new Function(
                c, "parameter_default_value_as_string",
                RuntimeModule::parameter_default_value_as_string, None, Return,
                "string", Parameters,
                new ParameterVariable(c, "sym", "runtime.parameter_symbol"),
                End),

            new Function(c, "type_is_union", RuntimeModule::type_is_union, None,
                         Return, "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.type_symbol"),
                         End),

            new Function(c, "type_is_class", RuntimeModule::type_is_class, None,
                         Return, "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.type_symbol"),
                         End),

            new Function(
                c, "type_is_interface", RuntimeModule::type_is_interface, None,
                Return, "bool", Parameters,
                new ParameterVariable(c, "sym", "runtime.type_symbol"), End),

            new Function(c, "type_is_opaque", RuntimeModule::type_is_opaque,
                         None, Return, "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.type_symbol"),
                         End),

            new Function(
                c, "type_is_union_tag", RuntimeModule::type_is_union_tag, None,
                Return, "bool", Parameters,
                new ParameterVariable(c, "sym", "runtime.type_symbol"), End),

            new Function(c, "type_is_reference_type",
                         RuntimeModule::type_is_reference_type, None, Return,
                         "bool", Parameters,
                         new ParameterVariable(c, "sym", "runtime.type_symbol"),
                         End),

            new Function(
                c, "type_structure_info", RuntimeModule::type_structure_info,
                None, Return,
                "([runtime.function_symbol],[runtime.variable_symbol])",
                Parameters,
                new ParameterVariable(c, "sym", "runtime.type_symbol"), End),

            new Function(c, "symbolic_constant_from_symbol",
                         RuntimeModule::symbolic_constant_from_symbol, None,
                         Return, "runtime.symbolic_constant", Parameters,
                         new ParameterVariable(c, "sym", "runtime.symbol"),
                         End),

            new Function(
                c, "symbolic_constant_value_as_string",
                RuntimeModule::symbolic_constant_value_as_string, None, Return,
                "string", Parameters,
                new ParameterVariable(c, "sym", "runtime.symbolic_constant"),
                End),

            EndArguments);

        globalScope()->addSymbols(
            new Function(c, "==", RuntimeModule::symbol_equals, CommOp, Return,
                         "bool", Args, "runtime.symbol", "runtime.symbol", End),

            new Function(c, "!=", RuntimeModule::symbol_nequals, CommOp, Return,
                         "bool", Args, "runtime.symbol", "runtime.symbol", End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(RuntimeModule::eval, Pointer)
    {
        typedef StringType::String String;

        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        String* s = NODE_ARG_OBJECT(0, String);
        ClassInstance* mlist = NODE_ARG_OBJECT(1, ClassInstance);

        Context::ModuleList modules;

        for (List list(p, mlist); !list.isNil(); list++)
        {
            if (String* mname = list.value<String*>())
            {
                QualifiedName n = c->lookupName(mname->c_str());
                const Module* m = c->findSymbolOfTypeByQualifiedName<Module>(n);
                modules.push_back(m);
            }
        }

        ostringstream str;

        TypedValue v = c->evalText(s->c_str(), "runtime.eval", p, modules);

        StringType::String* o = 0;

        if (v._type)
        {
            v._type->outputValue(str, v._value);
            string temp = str.str();
            o = c->stringType()->allocate(temp.c_str());
        }
        else
        {
            o = c->stringType()->allocate("");
        }

        NODE_RETURN(o);
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_set_warning_function, Pointer)
    {
        NODE_RETURN(0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_get_heap_size, int64)
    {
        NODE_RETURN(int64(GC_get_heap_size()));
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_get_free_bytes, int64)
    {
        NODE_RETURN(int64(GC_get_free_bytes()));
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_get_bytes_since_gc, int64)
    {
        NODE_RETURN(int64(GC_get_bytes_since_gc()));
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_get_total_bytes, int64)
    {
        NODE_RETURN(int64(GC_get_total_bytes()));
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_dump, void) { GC_dump(); }

    NODE_IMPLEMENTATION(RuntimeModule::gc_enable, void) { GC_enable(); }

    NODE_IMPLEMENTATION(RuntimeModule::gc_disable, void) { GC_disable(); }

    NODE_IMPLEMENTATION(RuntimeModule::gc_call_on_collect, void)
    {
        Process* p = NODE_THREAD.process();
        FunctionObject* F = NODE_ARG_OBJECT(0, FunctionObject);

        if (find(gc_callbacks.begin(), gc_callbacks.end(), F)
            == gc_callbacks.end())
        {
            gc_callbacks.push_back(F);
            gc_callback_threads.push_back(&NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_perform_collection, void)
    {
        GC_gcollect();
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_parallel_enabled, bool)
    {
        NODE_RETURN(GC_parallel ? true : false);
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_all_interior_pointers, bool)
    {
        NODE_RETURN(GC_all_interior_pointers ? true : false);
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_num_collections, int)
    {
        NODE_RETURN(GC_gc_no);
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_push_api, void)
    {
        switch (NODE_ARG(0, int))
        {
        default:
        case 0:
            GarbageCollector::pushMainHeapAPI();
            break;
        case 1:
            GarbageCollector::pushAutoreleaseAPI();
            break;
        case 2:
            GarbageCollector::pushMallocAutoreleaseAPI();
            break;
        case 3:
            GarbageCollector::pushStaticAutoreleaseAPI();
            break;
        case 4:
            GarbageCollector::pushStatAPI();
            break;
        case 5:
            GarbageCollector::pushMainHeapNoOptAPI();
            break;
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::gc_pop_api, void)
    {
        GarbageCollector::popAPI();
    }

    //----------------------------------------------------------------------

    NODE_IMPLEMENTATION(RuntimeModule::varying_size, int)
    {
        Process* p = NODE_THREAD.process();
        int dimension = NODE_ARG(0, int);

        if (dimension < 0 || dimension > 2)
        {
            throw OutOfRangeException();
        }

        NODE_RETURN(p->varyingSize(dimension));
    }

    NODE_IMPLEMENTATION(RuntimeModule::set_varying_size, void)
    {
        Process* p = NODE_THREAD.process();
        int dimension = NODE_ARG(0, int);
        int size = NODE_ARG(1, int);

        if (dimension < 0 || dimension > 2)
        {
            throw OutOfRangeException();
        }

        p->setVaryingSizeDimension(dimension, size);
    }

    static void dumpSymbols(ostream& o, Symbol* s, int depth,
                            bool primaryOnly = false)
    {
        typedef Mu::SymbolTable::SymbolHashTable HT;

        bool show = true;
        if (primaryOnly)
            show = s->isPrimary();

        if (show)
        {
            for (int i = 0; i < depth; i++)
                o << " ";
            o << hex << s << dec << " ";
            s->output(o);
            o << endl;
        }

        if (s->symbolTable())
        {
            HT& table = s->symbolTable()->hashTable();

            for (HT::Iterator it(table); it; ++it)
            {
                if (show)
                    o << it.index() << ":";

                for (Symbol* ss = (*it); ss; ss = ss->nextOverload())
                {
                    dumpSymbols(o, ss, depth + 1, primaryOnly);
                }
            }
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::dump_symbols, Pointer)
    {
        typedef StringType::String String;

        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const String* n = NODE_ARG_OBJECT(0, String);
        bool primary = NODE_ARG(1, bool);

        ostringstream str;

        if (n && (*n != ""))
        {
            QualifiedName qname = c->lookupName(n->c_str());
            Symbol::SymbolVector s;
            c->globalScope()->findSymbols(qname, s);

            if (s.size())
            {
                for (int i = 0; i < s.size(); i++)
                    dumpSymbols(str, s[i], 0, primary);
            }
            else
            {
                ExceptionType::Exception* e =
                    new ExceptionType::Exception(c->exceptionType());
                e->string() += "no symbol with qualified name ";
                e->string() += n->c_str();
                e->string() += " exists";
                ProgramException exc;
                exc.message() = e->string();
                throw exc;
            }
        }
        else
        {
            dumpSymbols(str, c->globalScope(), 0, primary);
        }

        StringType::String* o = c->stringType()->allocate(str);

        NODE_RETURN(o);
    }

    NODE_IMPLEMENTATION(RuntimeModule::layout_traits, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* type = static_cast<const ListType*>(NODE_THIS.type());
        List list(p, type);

        //
        //  Reports the layout characteristics of the platform. The
        //  information is encoded in an array. This is not intended to be
        //  used by user code.
        //

        Value value;
        typedef unsigned long ul;
        ul vp = reinterpret_cast<ul>(&value);

        list.append(reinterpret_cast<ul>(&value._Vector4f) - vp);
        list.append(reinterpret_cast<ul>(&value._Vector3f) - vp);
        list.append(reinterpret_cast<ul>(&value._Vector2f) - vp);
        list.append(reinterpret_cast<ul>(&value._float) - vp);
        list.append(reinterpret_cast<ul>(&value._int) - vp);
        list.append(reinterpret_cast<ul>(&value._bool) - vp);
        list.append(reinterpret_cast<ul>(&value._Pointer) - vp);

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(RuntimeModule::machine_types, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* type = static_cast<const ListType*>(NODE_THIS.type());
        const TupleType* ttype =
            static_cast<const TupleType*>(type->elementType());
        const MachineRep::MachineReps& reps = MachineRep::allReps();
        List list(p, type);

        struct TupleData
        {
            StringType::String* name;
            StringType::String* fmt;
            int size;
            int width;
            int salign;
            int nalign;
        };

        for (int i = 0; i < reps.size(); i++)
        {
            const MachineRep* rep = reps[i];
            ClassInstance* obj = ClassInstance::allocate(ttype);
            TupleData* o = (TupleData*)obj->structure();

            String rname = rep->name();
            String fmtName = rep->fmtName();
            o->name = MU_NEW_STRING(rname.c_str());
            o->fmt = MU_NEW_STRING(fmtName.c_str());
            o->size = rep->size();
            o->width = rep->width();
            o->salign = rep->structAlignment();
            o->nalign = rep->naturalAlignment();

            list.append(obj);
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(RuntimeModule::exit, void)
    {
        int value = NODE_ARG(0, int);
        ::exit(value);
    }

    NODE_IMPLEMENTATION(RuntimeModule::stack_traits, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* type = static_cast<const ListType*>(NODE_THIS.type());
        List list(p, type);

        list.append(NODE_THREAD.stack().capacity());
        list.append(NODE_THREAD.stack().size());
        list.append(NODE_THREAD.stackOffset());
        list.append((long int)(NODE_THREAD.bottomOfStack()));
        list.append(sizeof(Value));

        NODE_RETURN(list.head());
    }

    struct StringTuple
    {
        StringType::String* first;
        StringType::String* second;
    };

    NODE_IMPLEMENTATION(RuntimeModule::module_locations, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* ltype = static_cast<const ListType*>(NODE_THIS.type());
        const TupleType* ttype =
            static_cast<const TupleType*>(ltype->elementType());
        const StringType* stype =
            static_cast<const StringType*>(ttype->fieldType(0));

        List list(p, ltype);

        for (SymbolTable::RecursiveIterator i(
                 p->context()->globalScope()->symbolTable());
             i; ++i)
        {
            if (const Module* m = dynamic_cast<const Module*>(*i))
            {
                ClassInstance* obj = ClassInstance::allocate(ttype);
                StringTuple* st = obj->data<StringTuple>();
                st->first = stype->allocate(m->fullyQualifiedName().c_str());
                st->second = stype->allocate(m->location().c_str());
                list.append(obj);
            }
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(RuntimeModule::backtrace, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const ListType* ltype = static_cast<const ListType*>(NODE_THIS.type());
        const Mu::StringType* stype = c->stringType();
        List list(p, ltype);

        Thread::BackTrace backtrace;
        NODE_THREAD.backtrace(backtrace);

        for (int i = 0; i < backtrace.size(); i++)
        {
            Node* n = backtrace[i].node;
            const Symbol* s = backtrace[i].symbol;

            ostringstream cstr;

            const Function* F = dynamic_cast<const Function*>(s);

            if (c->debugging() && F && !F->hasHiddenArgument())
            {
                AnnotatedNode* anode = static_cast<AnnotatedNode*>(n);

                if (anode->sourceFileName())
                {
                    cstr << anode->sourceFileName() << ", line "
                         << anode->linenum() << ", char " << anode->charnum()
                         << ": ";
                }
            }

            if (s)
                s->outputNode(cstr, n);

            list.append(stype->allocate(cstr));
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(RuntimeModule::load_module, bool)
    {
        typedef StringType::String String;

        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const String* n = NODE_ARG_OBJECT(0, String);

        Name name = c->internName(n->c_str());

        if (Module* m = Module::load(name, p, c))
        {
            NODE_RETURN(true);
        }
        else
        {
            NODE_RETURN(false);
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::intern_name, Pointer)
    {
        typedef StringType::String String;
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const String* n = NODE_ARG_OBJECT(0, String);

        NODE_RETURN(Pointer(c->internName(n->c_str()).nameRef()));
    }

    NODE_IMPLEMENTATION(RuntimeModule::lookup_name, Pointer)
    {
        typedef StringType::String String;
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const String* n = NODE_ARG_OBJECT(0, String);

        if (c->namePool().exists(n->c_str()))
        {
            NODE_RETURN(Pointer(c->namePool().find(n->c_str()).nameRef()));
        }
        else
        {
            NODE_RETURN(0);
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::lookup_function, Pointer)
    {
        typedef StringType::String String;
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        Name qname = reinterpret_cast<Name::Ref>(NODE_ARG(0, Pointer));

        if (const Function* F =
                c->findSymbolOfTypeByQualifiedName<Function>(qname))
        {
            const Function* F0 = F->firstFunctionOverload();
            FunctionObject* obj = new FunctionObject(F0);
            NODE_RETURN(obj);
        }
        else
        {
            NODE_RETURN(Pointer(0));
        }
    }

#define xstr(x) str(x)
#define str(x) #x

    NODE_IMPLEMENTATION(RuntimeModule::build_os, Pointer)
    {
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        NODE_RETURN(stype->allocate(xstr(PLATFORM)));
    }

    NODE_IMPLEMENTATION(RuntimeModule::build_arch, Pointer)
    {
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        NODE_RETURN(stype->allocate(xstr(ARCH)));
    }

    NODE_IMPLEMENTATION(RuntimeModule::build_compiler, Pointer)
    {
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        NODE_RETURN(stype->allocate(xstr(COMPILER)));
    }

#undef str
#undef xstr

    //----------------------------------------------------------------------

    NODE_IMPLEMENTATION(RuntimeModule::symbol_from_name, Pointer)
    {
        typedef StringType::String String;
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        Name qname = reinterpret_cast<Name::Ref>(NODE_ARG(0, Pointer));

        if (qname == "")
        {
            NODE_RETURN(Pointer(c->globalScope()));
        }
        else if (const Symbol* s = c->findSymbolByQualifiedName(qname, false))
        {
            NODE_RETURN(Pointer(s));
        }
        else
        {
            NODE_RETURN(Pointer(0));
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_function, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const Function*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_method, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const MemberFunction*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_parameter, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const ParameterVariable*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_type_modifier, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const TypeModifier*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_variable, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const Variable*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::function_from_symbol, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        if (const Function* f = dynamic_cast<const Function*>(symbol))
        {
            NODE_RETURN(Pointer(f));
        }
        else
        {
            throw BadCastException();
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_nil, bool)
    {
        Pointer p = NODE_ARG(0, Pointer);
        NODE_RETURN(p == 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_type, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const Type*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_module, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const Module*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_is_symbolic_constant, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const SymbolicConstant*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::type_from_symbol, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        if (const Type* t = dynamic_cast<const Type*>(symbol))
        {
            NODE_RETURN(Pointer(t));
        }
        else
        {
            throw BadCastException();
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_name, Pointer)
    {
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        NODE_RETURN(stype->allocate(symbol->name()));
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_scope, Pointer)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(Pointer(symbol->scope()));
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_fully_qualified_name, Pointer)
    {
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        if (symbol)
        {
            NODE_RETURN(stype->allocate(symbol->fullyQualifiedName()));
        }
        else
        {
            throw NilArgumentException();
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_documentation, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());

        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        if (symbol)
        {
            if (Object* o = p->documentSymbol(symbol))
            {
                NODE_RETURN(Pointer(o));
            }
        }

        StringType::String* obj = new StringType::String(stype);
        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_overloaded_symbols, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* ltype = static_cast<const ListType*>(NODE_THIS.type());
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);

        if (!symbol)
            throw NilArgumentException();

        List list(p, ltype);

        for (const Symbol* s = symbol->firstOverload(); s;
             s = s->nextOverload())
        {
            list.append(s);
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_symbols_in_scope, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const ListType* ltype = static_cast<const ListType*>(NODE_THIS.type());
        const Symbol* s = NODE_ARG_OBJECT(0, const Symbol);

        if (!s)
            throw NilArgumentException();

        List list(p, ltype);
        typedef Mu::SymbolTable::SymbolHashTable HT;

        if (s->symbolTable())
        {
            HT& table = s->symbolTable()->hashTable();

            for (HT::Iterator it(table); it; ++it)
            {
                list.append(*it);
            }
        }

        NODE_RETURN(list.head());
    }

    NODE_IMPLEMENTATION(RuntimeModule::function_type, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Function* f = NODE_ARG_OBJECT(0, const Function);
        if (!f)
            throw NilArgumentException();

        if (const Type* t = f->type())
            NODE_RETURN(Pointer(t));
        else
            throw BadCastException();
    }

    NODE_IMPLEMENTATION(RuntimeModule::cast_to_symbol, Pointer)
    {
        Process* p = NODE_THREAD.process();
        Pointer s = NODE_ARG(0, Pointer);
        NODE_RETURN(s);
    }

    NODE_IMPLEMENTATION(RuntimeModule::variable_from_symbol, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        if (const Variable* v = dynamic_cast<const Variable*>(symbol))
        {
            NODE_RETURN(Pointer(v));
        }
        else
        {
            throw BadCastException();
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::variable_type, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Variable* var = NODE_ARG_OBJECT(0, const Variable);
        if (!var)
            throw NilArgumentException();
        NODE_RETURN(Pointer(var->storageClass()));
    }

    NODE_IMPLEMENTATION(RuntimeModule::parameter_from_symbol, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        if (const ParameterVariable* v =
                dynamic_cast<const ParameterVariable*>(symbol))
        {
            NODE_RETURN(Pointer(v));
        }
        else
        {
            throw BadCastException();
        }
    }

    struct PPTuple
    {
        Pointer rtype;
        Pointer alist;
        Pointer plist;
    };

    NODE_IMPLEMENTATION(RuntimeModule::function_signature, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Function* f = NODE_ARG_OBJECT(0, const Function);
        if (!f)
            throw NilArgumentException();

        const TupleType* ttype =
            static_cast<const TupleType*>(NODE_THIS.type());
        const ListType* lttype =
            static_cast<const ListType*>(ttype->fieldType(1));
        const ListType* lptype =
            static_cast<const ListType*>(ttype->fieldType(2));

        ClassInstance* obj = ClassInstance::allocate(ttype);
        PPTuple* pp = obj->data<PPTuple>();

        const Signature* sig = f->signature();
        const Signature::Types& types = sig->types();

        pp->rtype = Pointer(types.front().symbol);

        List list(p, lttype);
        List plist(p, lptype);

        for (size_t i = 1; i < types.size(); i++)
        {
            list.append(types[i].symbol);
        }

        for (size_t i = 0; i < f->numArgs() + f->numFreeVariables(); i++)
        {
            plist.append(f->parameter(i));
        }

        pp->alist = Pointer(list.head());
        pp->plist = Pointer(plist.head());

        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(RuntimeModule::parameter_default_value_as_string,
                        Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        const ParameterVariable* param =
            NODE_ARG_OBJECT(0, const ParameterVariable);
        if (!param)
            throw NilArgumentException();

        if (param->hasDefaultValue())
        {
            ostringstream str;
            param->storageClass()->outputValue(str, param->defaultValue());
            NODE_RETURN(stype->allocate(str));
        }
        else
        {
            NODE_RETURN(Pointer(0));
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::type_is_union, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const VariantType*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::type_is_union_tag, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const VariantTagType*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::type_is_class, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const Class*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::type_is_interface, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const Interface*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::type_is_opaque, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const OpaqueType*>(symbol) != 0);
    }

    NODE_IMPLEMENTATION(RuntimeModule::type_is_reference_type, bool)
    {
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();
        NODE_RETURN(dynamic_cast<const ReferenceType*>(symbol) != 0);
    }

    struct TSITuple
    {
        Pointer clist;
        Pointer flist;
    };

    NODE_IMPLEMENTATION(RuntimeModule::type_structure_info, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        const TupleType* ttype =
            static_cast<const TupleType*>(NODE_THIS.type());
        const ListType* cttype =
            static_cast<const ListType*>(ttype->fieldType(0));
        const ListType* fttype =
            static_cast<const ListType*>(ttype->fieldType(1));

        ClassInstance* obj = ClassInstance::allocate(ttype);
        TSITuple* pp = obj->data<TSITuple>();

        List clist(p, cttype);
        List flist(p, fttype);
        typedef Mu::SymbolTable::SymbolHashTable HT;

        if (const Class* c = dynamic_cast<const Class*>(symbol))
        {
            const Class::MemberVariableVector& vars = c->memberVariables();

            for (size_t i = 0; i < vars.size(); i++)
                flist.append(vars[i]);

            if (const Function* F = c->findSymbolOfType<Function>(c->name()))
            {
                for (const Function* fo = F->firstFunctionOverload(); fo;
                     fo = fo->nextFunctionOverload())
                {
                    if (fo->name() == c->name())
                        clist.append(fo);
                }
            }

            if (const Function* F =
                    c->scope()->findSymbolOfType<Function>(c->name()))
            {
                for (const Function* fo = F->firstFunctionOverload(); fo;
                     fo = fo->nextFunctionOverload())
                {
                    if (fo->name() == c->name())
                        clist.append(fo);
                }
            }
        }

        if (const VariantType* vt = dynamic_cast<const VariantType*>(symbol))
        {
            if (vt->symbolTable())
            {
                HT& table = vt->symbolTable()->hashTable();

                for (HT::Iterator it(table); it; ++it)
                {
                    if (const VariantTagType* t =
                            dynamic_cast<const VariantTagType*>(*it))
                    {
                        if (const Function* C =
                                t->findSymbolOfType<Function>(t->name()))
                        {
                            clist.append(C);
                        }
                    }
                }
            }
        }

        pp->clist = Pointer(clist.head());
        pp->flist = Pointer(flist.head());

        NODE_RETURN(obj);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_equals, bool)
    {
        const Symbol* a = NODE_ARG_OBJECT(0, const Symbol);
        const Symbol* b = NODE_ARG_OBJECT(1, const Symbol);
        NODE_RETURN(a == b);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbol_nequals, bool)
    {
        const Symbol* a = NODE_ARG_OBJECT(0, const Symbol);
        const Symbol* b = NODE_ARG_OBJECT(1, const Symbol);
        NODE_RETURN(a != b);
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbolic_constant_from_symbol, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Symbol* symbol = NODE_ARG_OBJECT(0, const Symbol);
        if (!symbol)
            throw NilArgumentException();

        if (const SymbolicConstant* s =
                dynamic_cast<const SymbolicConstant*>(symbol))
        {
            NODE_RETURN(Pointer(s));
        }
        else
        {
            throw BadCastException();
        }
    }

    NODE_IMPLEMENTATION(RuntimeModule::symbolic_constant_value_as_string,
                        Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(NODE_THIS.type());
        const SymbolicConstant* sc = NODE_ARG_OBJECT(0, const SymbolicConstant);
        if (!sc)
            throw NilArgumentException();

        ostringstream str;
        sc->type()->outputValue(str, sc->value());
        NODE_RETURN(stype->allocate(str));
    }

} // namespace Mu
