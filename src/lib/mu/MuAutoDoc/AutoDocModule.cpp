//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuAutoDoc/AutoDocModule.h>
#include <Mu/Alias.h>
#include <Mu/Class.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/ParameterVariable.h>
#include <Mu/HashTable.h>
#include <Mu/MemberFunction.h>
#include <Mu/MuProcess.h>
#include <Mu/SymbolTable.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/Interface.h>
#include <Mu/Type.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <MuLang/DynamicArray.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <Mu/ParameterVariable.h>

namespace Mu
{
    using namespace std;
    typedef StringType::String StringObj;

    AutoDocModule::AutoDocModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    AutoDocModule::~AutoDocModule() {}

    void AutoDocModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Context* c = context();

        addSymbols(
            new Function(c, "document_symbol", AutoDocModule::document_symbol,
                         None, Return, "string", Parameters,
                         new Param(c, "symbol", "string"),
                         new Param(c, "format", "int", Value(0)),
                         new Param(c, "indent_level", "int", Value(0)), End),

            //
            // NOTE: document_modules probably belongs in 'runtime'.
            //
            new Function(c, "document_modules", AutoDocModule::document_modules,
                         None, Return, "string[]", End),

            new SymbolicConstant(c, "Text", "int", Value(0)),
            new SymbolicConstant(c, "HTML", "int", Value(1)),
            new SymbolicConstant(c, "Texinfo", "int", Value(2)),

            EndArguments);
    }

    static void outputIndent(ostream& o, int n)
    {
        for (int i = 0; i < n; i++)
            o << " ";
    }

    struct Comp
    {
        int operator()(Symbol* a, Symbol* b)
        {
            return strcmp(a->name().c_str(), b->name().c_str()) < 0;
        }
    };

    static String textDocSymbol(Process* p, Symbol* s, int indent,
                                bool overloaded = true)
    {
        ostringstream str;

        int n = 0;
        Function* f = 0;
        Module* m = 0;
        Type* t = 0;
        Class* c = 0;
        Alias* a = 0;
        Interface* i = 0;
        Variable* v = 0;
        SymbolicConstant* sc = 0;

        for (Symbol* q = s; q; q = q->nextOverload())
        {
            if (!f)
                f = dynamic_cast<Function*>(q);
            if (!t)
                t = dynamic_cast<Type*>(q);
            if (!a)
                a = dynamic_cast<Alias*>(q);
            if (!m)
                m = dynamic_cast<Module*>(q);
            if (!c)
                c = dynamic_cast<Class*>(q);
            if (!i)
                i = dynamic_cast<Interface*>(q);
            if (!v)
                v = dynamic_cast<Variable*>(q);
            if (!sc)
                sc = dynamic_cast<SymbolicConstant*>(q);
            n++;
            if (!overloaded)
                break;
        }

        if (sc)
        {
            for (Symbol* x = sc; x; x = x->nextOverload())
            {
                if (s = dynamic_cast<SymbolicConstant*>(x))
                {
                    outputIndent(str, indent);
                    s->output(str);
                    str << "\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (v)
        {
            for (Symbol* x = s; x; x = x->nextOverload())
            {
                if (v = dynamic_cast<Variable*>(x))
                {
                    outputIndent(str, indent);
                    v->output(str);
                    str << "\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (f)
        {
            for (Symbol* x = s; x; x = x->nextOverload())
            {
                if (f = dynamic_cast<Function*>(x))
                {
                    outputIndent(str, indent);
                    f->output(str);
                    str << "\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (a)
        {
            for (Symbol* x = s; x; x = x->nextOverload())
            {
                if (a = dynamic_cast<Alias*>(x))
                {
                    outputIndent(str, indent);
                    a->output(str);
                    str << "\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (m || t)
        {
            vector<Symbol*> symbols;

            typedef SymbolTable::SymbolHashTable HTable;
            typedef HTable::Iterator Iterator;

            outputIndent(str, indent);
            s->output(str);

            if (s->symbolTable())
            {
                str << "\n\n";

                HTable& table = s->symbolTable()->hashTable();

                for (Iterator i(table); i; ++i)
                {
                    symbols.push_back(*i);
                }

                sort(symbols.begin(), symbols.end(), Comp());

                for (int i = 0; i < symbols.size(); i++)
                {
                    str << textDocSymbol(p, symbols[i], indent + 4);
                }
            }

            str << "\n\n";
        }

        //
        //  Look up user docs
        //
        if (Object* o = p->documentSymbol(s))
        {
            MuLangContext* context = static_cast<MuLangContext*>(p->context());
            const Type* t = o->type();

            if (t == context->stringType())
            {
                const StringType::String* sobj =
                    static_cast<StringType::String*>(o);
                str << endl << sobj->c_str() << endl;
            }
        }

        return String(str.str().c_str());
    }

    static String htmlDocSymbol(Symbol* s, bool overloaded = true)
    {
        ostringstream str;

        int n = 0;
        Function* f = 0;
        Module* m = 0;
        Type* t = 0;
        Class* c = 0;
        Alias* a = 0;
        Interface* i = 0;
        Variable* v = 0;

        for (Symbol* q = s; q; q = q->nextOverload())
        {
            if (!f)
                f = dynamic_cast<Function*>(q);
            if (!t)
                t = dynamic_cast<Type*>(q);
            if (!a)
                a = dynamic_cast<Alias*>(q);
            if (!m)
                m = dynamic_cast<Module*>(q);
            if (!c)
                c = dynamic_cast<Class*>(q);
            if (!i)
                i = dynamic_cast<Interface*>(q);
            if (!v)
                v = dynamic_cast<Variable*>(q);
            n++;
            if (!overloaded)
                break;
        }

        str << "<DL>\n";

        if (v)
        {
            for (Symbol* x = s; x; x = x->nextOverload())
            {
                if (v = dynamic_cast<Variable*>(x))
                {
                    str << "<DT><CODE>";
                    v->output(str);
                    str << "</CODE></DT>\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (f)
        {
            for (Symbol* x = s; x; x = x->nextOverload())
            {
                if (f = dynamic_cast<Function*>(x))
                {
                    str << "<DT><CODE>";
                    f->output(str);
                    str << "</CODE></DT>\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (a)
        {
            for (Symbol* x = s; x; x = x->nextOverload())
            {
                if (a = dynamic_cast<Alias*>(x))
                {
                    str << "<DT><CODE>";
                    a->output(str);
                    str << "</CODE></DT>\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (m || t)
        {
            vector<Symbol*> symbols;

            typedef SymbolTable::SymbolHashTable HTable;
            typedef HTable::Iterator Iterator;

            str << "<H2>";
            s->output(str);
            str << "</H2>\n";

            if (s->symbolTable())
            {
                str << "\n\n";

                HTable& table = s->symbolTable()->hashTable();

                for (Iterator i(table); i; ++i)
                {
                    symbols.push_back(*i);
                }

                sort(symbols.begin(), symbols.end(), Comp());

                str << "<DL>\n";
                for (int i = 0; i < symbols.size(); i++)
                {
                    str << htmlDocSymbol(symbols[i]);
                }
                str << "</DL>\n";
            }

            str << "<P>\n";
        }

        str << "</DL>\n";

        return String(str.str().c_str());
    }

    static void texiDoc(ostream& o, Function* f)
    {
        o << "@deftypefn {Function} {} " << f->name() << " ("
          << f->returnType()->fullyQualifiedName() << "; ";

        for (int i = 0; i < f->numArgs(); i++)
        {
            if (i)
            {
                o << ", ";
            }

            if (f->hasParameters())
            {
                const ParameterVariable* p = f->parameter(i);
                o << p->storageClass()->fullyQualifiedName() << " @var{"
                  << p->name() << "}";

                if (p->hasDefaultValue())
                {
                    o << " = ";
                    p->storageClass()->outputValue(o, p->defaultValue());
                }
            }
            else
            {
                o << f->argType(i)->fullyQualifiedName();
            }
        }

        o << ")\n@end deftypefn\n";
    }

    static void texiDoc(ostream& o, Alias* a)
    {
        o << "@deftp {Alias} " << a->name() << " -> "
          << a->alias()->fullyQualifiedName() << "\n@end deftp\n";
    }

    static String texiDocSymbol(Symbol* s, bool overloaded = true)
    {
        ostringstream str;

        int n = 0;
        Function* f = 0;
        Module* m = 0;
        Type* t = 0;
        Class* c = 0;
        Alias* a = 0;
        Interface* i = 0;

        for (Symbol* q = s; q; q = q->nextOverload())
        {
            if (!f)
                f = dynamic_cast<Function*>(q);
            if (!t)
                t = dynamic_cast<Type*>(q);
            if (!a)
                a = dynamic_cast<Alias*>(q);
            if (!m)
                m = dynamic_cast<Module*>(q);
            if (!c)
                c = dynamic_cast<Class*>(q);
            if (!i)
                i = dynamic_cast<Interface*>(q);
            n++;

            if (!overloaded)
                break;
        }

        if (f)
        {
            for (Symbol* x = s; x; x = x->nextOverload())
            {
                if (f = dynamic_cast<Function*>(x))
                {
                    texiDoc(str, f);
                    str << "\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (a)
        {
            for (Symbol* x = s; x; x = x->nextOverload())
            {
                if (a = dynamic_cast<Alias*>(x))
                {
                    texiDoc(str, a);
                    str << "\n";
                }

                if (!overloaded)
                    break;
            }
        }

        if (m || t)
        {
            vector<Symbol*> symbols;

            typedef SymbolTable::SymbolHashTable HTable;
            typedef HTable::Iterator Iterator;

            if (m)
            {
                str << "@deftp {Module} " << m->fullyQualifiedName()
                    << "\n@end deftp";
            }
            else if (t)
            {
                str << "@deftp {Type} " << t->fullyQualifiedName()
                    << "\n@end deftp";
            }

            if (s->symbolTable())
            {
                str << "\n\n";
                HTable& table = s->symbolTable()->hashTable();

                for (Iterator i(table); i; ++i)
                {
                    symbols.push_back(*i);
                }

                sort(symbols.begin(), symbols.end(), Comp());

                for (int i = 0; i < symbols.size(); i++)
                {
                    str << texiDocSymbol(symbols[i]);
                }
            }

            str << "\n\n";
        }

        return String(str.str().c_str());
    }

    NODE_IMPLEMENTATION(AutoDocModule::document_symbol, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringObj* n = NODE_ARG_OBJECT(0, StringObj);
        int kind = NODE_ARG(1, int);
        int indent = NODE_ARG(2, int);

        QualifiedName qname = c->internName(n->c_str());
        Symbol::SymbolVector symbols;

        if (qname == "")
        {
            symbols.push_back(c->globalScope());
        }
        else
        {
            c->globalScope()->findSymbols(qname, symbols);
        }

        if (symbols.size())
        {
            String out;

            for (int i = 0; i < symbols.size(); i++)
            {
                switch (kind)
                {
                case 0:
                default:
                    out += textDocSymbol(p, symbols[i], indent, false);
                    break;
                case 1:
                    out += htmlDocSymbol(symbols[i], false);
                    break;
                case 2:
                    out += texiDocSymbol(symbols[i], false);
                    break;
                }
            }

            NODE_RETURN(c->stringType()->allocate(out));
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

    NODE_IMPLEMENTATION(AutoDocModule::document_modules, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());

        std::vector<std::string> moduleNames;

        SymbolTable* st = c->globalScope()->symbolTable();
        for (SymbolTable::Iterator iter(st); iter; ++iter)
        {
            const Symbol* s = (*iter);
            if (const Module* m = dynamic_cast<const Module*>(s))
            {
                moduleNames.push_back(s->name());
            }
        }

        DynamicArrayType* atype = (DynamicArrayType*)NODE_THIS.type();
        const Class* stype = c->stringType();
        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(moduleNames.size());
        for (int i = 0; i < moduleNames.size(); i++)
        {
            array->element<StringType::String*>(i) =
                c->stringType()->allocate(moduleNames[i]);
        }

        NODE_RETURN(array);
    }

} // namespace Mu
