//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/DynamicArray.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Class.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/Module.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <ctype.h>
#include <iostream>
#include <sstream>

namespace Mu
{
    using namespace std;
    using namespace Mu;

    //----------------------------------------------------------------------

    ExceptionType::Exception::Exception(const Class* c)
        : ClassInstance(c)
    {
    }

    String ExceptionType::Exception::backtraceAsString() const
    {
        return Mu::Exception::backtraceAsString(_backtrace);
    }

    //----------------------------------------------------------------------

    ExceptionType::ExceptionType(Context* c, Class* super)
        : Class(c, "exception", super)
    {
    }

    ExceptionType::~ExceptionType() {}

    Object* ExceptionType::newObject() const { return new Exception(this); }

    void ExceptionType::deleteObject(Object* obj) const
    {
        delete reinterpret_cast<ExceptionType::Exception*>(obj);
    }

    void ExceptionType::outputValueRecursive(ostream& o, const ValuePointer vp,
                                             ValueOutputState& state) const
    {
        const Exception* s = *reinterpret_cast<const Exception**>(vp);

        if (s)
        {
            o << "exception: ";
            StringType::outputQuotedString(o, s->string());
        }
        else
        {
            o << "nil";
        }
    }

    void ExceptionType::freeze()
    {
        Class::freeze();
        _isGCAtomic = false;
    }

    void ExceptionType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        Context* c = context();

        MuLangContext* context = (MuLangContext*)globalModule()->context();
        context->arrayType(context->stringType(), 1, 0); // string[]

        s->addSymbols(
            new ReferenceType(c, "exception&", this),

            new Function(c, "exception", ExceptionType::construct, None, Return,
                         "exception", End),

            new Function(c, "exception", ExceptionType::dereference, Cast,
                         Return, "exception", Args, "exception&", End),

            new Function(c, "exception", ExceptionType::stringCast, Mapped,
                         Return, "exception", Args, "string", End),

            new Function(c, "=", ExceptionType::assign, AsOp, Return,
                         "exception&", Args, "exception&", "exception", End),

            new Function(c, "==", ExceptionType::equals, CommOp, Return, "bool",
                         Args, "exception", "exception", End),

            new Function(c, "print", ExceptionType::print, None, Return, "void",
                         Args, "exception", End),

            new Function(c, "__try", ExceptionType::mu__try, None, Return,
                         "void", Args, "?", "?", Optional, "?varargs", End),

            new Function(c, "__catch", ExceptionType::mu__catch, None, Return,
                         "bool", Args, "?", "?", End),

            new Function(c, "__catch_all", ExceptionType::mu__catch_all, None,
                         Return, "bool", Args, "?", End),

            new Function(c, "__exception", ExceptionType::mu__exception, None,
                         Return, "object", End),

            new Function(c, "__throw", ExceptionType::mu__throw_exception, None,
                         Return, "void", Args, "exception", End),

            new Function(c, "__throw", ExceptionType::mu__throw, None, Return,
                         "void", Args, "object", End),

            new Function(c, "__rethrow", ExceptionType::mu__rethrow, None,
                         Return, "void", End),

            EndArguments);

        addSymbols(new MemberFunction(c, "copy", ExceptionType::copy, Mapped,
                                      Return, "object", Args, "exception", End),

                   new MemberFunction(c, "backtrace", ExceptionType::backtrace,
                                      None, Return, "string[]", Args,
                                      "exception", End),
                   EndArguments);
    }

    NODE_IMPLEMENTATION(ExceptionType::mu__try, void)
    {
        try
        {
#if MU_SAFE_ARGUMENTS
            NODE_EVAL_VALUE(NODE_THIS.argNode(0), NODE_THREAD);
#else
            NODE_ARG(0, int);
#endif
        }
        catch (Mu::ProgramException& exc)
        {
            Object* e = exc.object();

            if (!e)
            {
                e = NODE_THREAD.exception();
            }

            NODE_THREAD.setException(e);

            for (int i = 1; NODE_THIS.argNode(i); i++)
            {
                if (bool caught = NODE_ARG(i, bool))
                {
                    return;
                }
            }

            NODE_THREAD.setException(e);
            throw ProgramException(NODE_THREAD, e);
        }
        catch (Mu::Exception& exc)
        {
            Process* p = NODE_THREAD.process();
            MuLangContext* context = (MuLangContext*)p->context();
            ExceptionType::Exception* e =
                new ExceptionType::Exception(context->exceptionType());
            e->string() = exc.what();
            e->backtrace() = exc.backtrace();
            NODE_THREAD.setException(e);

            for (int i = 1; NODE_THIS.argNode(i); i++)
            {
                if (bool caught = NODE_ARG(i, bool))
                {
                    return;
                }
            }

            NODE_THREAD.setException(e);
            throw ProgramException(NODE_THREAD, e);
        }
        catch (...)
        {
            Process* p = NODE_THREAD.process();
            MuLangContext* context = (MuLangContext*)p->context();
            ExceptionType::Exception* e =
                new ExceptionType::Exception(context->exceptionType());
            e->string() = "unknown exception";
            NODE_THREAD.setException(e);

            for (int i = 1; NODE_THIS.argNode(i); i++)
            {
                if (bool caught = NODE_ARG(i, bool))
                {
                    return;
                }
            }

            NODE_THREAD.setException(e);
            throw ProgramException(NODE_THREAD, e);
        }
    }

    NODE_IMPLEMENTATION(ExceptionType::mu__catch, bool)
    {
        bool rval = false;
        const Node* n = NODE_THIS.argNode(0);

        if (const ReferenceType* rt =
                dynamic_cast<const ReferenceType*>(n->type()))
        {
            if (const Type* t = rt->dereferenceType())
            {
                if (Object* e = NODE_THREAD.exception())
                {
                    if (rval = t->match(e->type()))
                    {
                        NODE_ARG(0, Pointer);
                        NODE_ARG(1, void);
                        NODE_THREAD.setException(0);
                    }
                }
            }
        }

        NODE_RETURN(rval);
    }

    NODE_IMPLEMENTATION(ExceptionType::mu__catch_all, bool)
    {
        NODE_ARG(0, void);
        NODE_THREAD.setException(0);
        NODE_RETURN(true);
    }

    NODE_IMPLEMENTATION(ExceptionType::mu__exception, Pointer)
    {
        NODE_RETURN(NODE_THREAD.exception());
    }

    NODE_IMPLEMENTATION(ExceptionType::mu__throw_exception, void)
    {
        Exception* e = reinterpret_cast<Exception*>(NODE_ARG(0, Pointer));
        NODE_THREAD.setException(e);
        // NOTE : Comment-out the following line to avoid a crash in the
        // backtrace code.
        NODE_THREAD.backtrace(e->backtrace());
        throw ProgramException(NODE_THREAD, e);
    }

    NODE_IMPLEMENTATION(ExceptionType::mu__throw, void)
    {
        // Exception *e = reinterpret_cast<Exception*>(NODE_ARG(0, Pointer));
        // NODE_THREAD.setException(e);
        // NODE_THREAD.backtrace(e->backtrace());
        throw ProgramException(NODE_THREAD, NODE_ARG_OBJECT(0, Mu::Object));
    }

    NODE_IMPLEMENTATION(ExceptionType::mu__rethrow, void)
    {
        if (NODE_THREAD.exception())
        {
            Exception* e = static_cast<Exception*>(NODE_THREAD.exception());
            NODE_THREAD.setException(e);
            throw ProgramException(NODE_THREAD, e);
        }
        else
        {
            Process* p = NODE_THREAD.process();
            MuLangContext* c = (MuLangContext*)p->context();
            ExceptionType::Exception* e =
                new ExceptionType::Exception(c->exceptionType());
            NODE_THREAD.backtrace(e->backtrace());
            e->string() =
                "Runtime Exception: rethrow with no current exception";
            NODE_THREAD.setException(e);
            throw ProgramException(NODE_THREAD, e);
        }
    }

    NODE_IMPLEMENTATION(ExceptionType::construct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const ExceptionType*>(NODE_THIS.type());
        ExceptionType::Exception* o = new ExceptionType::Exception(c);
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(ExceptionType::stringCast, Pointer)
    {
        const StringType::String* s =
            reinterpret_cast<StringType::String*>(NODE_ARG(0, Pointer));
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const ExceptionType*>(NODE_THIS.type());
        ExceptionType::Exception* o = new ExceptionType::Exception(c);
        o->string() = s->c_str();
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(ExceptionType::dereference, Pointer)
    {
        Exception** i = reinterpret_cast<Exception**>(NODE_ARG(0, Pointer));
        Exception* o = *i;
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(ExceptionType::assign, Pointer)
    {
        Exception** i = reinterpret_cast<Exception**>(NODE_ARG(0, Pointer));
        Exception* o = reinterpret_cast<Exception*>(NODE_ARG(1, Pointer));
        *i = o;
        NODE_RETURN(Pointer(i));
    }

    NODE_IMPLEMENTATION(ExceptionType::equals, bool)
    {
        Exception* a = reinterpret_cast<Exception*>(NODE_ARG(0, Pointer));
        Exception* b = reinterpret_cast<Exception*>(NODE_ARG(1, Pointer));

        bool result = a == b;

        if (!result)
        {
            result = a->string() == b->string();
        }

        NODE_RETURN(result);
    }

    NODE_IMPLEMENTATION(ExceptionType::print, void)
    {
        Exception* o = reinterpret_cast<Exception*>(NODE_ARG(0, Pointer));
        cout << o->string();
        return;
    }

    NODE_IMPLEMENTATION(ExceptionType::copy, Pointer)
    {
        Exception* s = reinterpret_cast<Exception*>(NODE_ARG(0, Pointer));
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const ExceptionType*>(NODE_THIS.type());
        ExceptionType::Exception* o = new ExceptionType::Exception(c);

        o->string() = s->string();

        NODE_RETURN(o);
    }

    NODE_IMPLEMENTATION(ExceptionType::backtrace, Pointer)
    {
        Exception* e = NODE_ARG_OBJECT(0, Exception);
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const ExceptionType*>(NODE_THIS.type());
        DynamicArrayType* atype = (DynamicArrayType*)(NODE_THIS.type());
        const Mu::StringType* stype =
            static_cast<const Mu::StringType*>(atype->elementType());
        DynamicArray* array = new DynamicArray(atype, 1);

        array->resize(e->backtrace().size());

        for (int i = 0; i < e->backtrace().size(); i++)
        {
            Node* n = e->backtrace()[i];
            const Symbol* s = n->symbol();

            ostringstream cstr;

            const Function* F = dynamic_cast<const Function*>(s);

            if (p->context()->debugging() && F && !F->hasHiddenArgument())
            {
                AnnotatedNode* anode = static_cast<AnnotatedNode*>(n);

                if (anode->sourceFileName())
                {
                    cstr << anode->sourceFileName() << ", line "
                         << anode->linenum() << ", char " << anode->charnum()
                         << ": ";
                }
            }

            s->outputNode(cstr, n);
            array->element<StringType::String*>(i) = stype->allocate(cstr);
        }

        NODE_RETURN(array);
    }

} // namespace Mu
