//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Archive.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/MachineRep.h>
#include <Mu/MachineRep.h>
#include <Mu/Module.h>
#include <Mu/NodePrinter.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <iostream>
#include <string>

namespace Mu
{
    using namespace std;

    FunctionType::FunctionType(Context* context, const char* name)
        : Class(context, name)
        , _signature(new Signature)
    {
        // an "ambiguous" function type
        _isSerializable = true;
    }

    FunctionType::FunctionType(Context* context, const char* name,
                               const Signature* sig)
        : Class(context, name)
        , _signature(sig)
    {
    }

    FunctionType::~FunctionType() {}

    void FunctionType::deleteObject(Object* o) const
    {
        FunctionObject* fo = static_cast<FunctionObject*>(o);
        delete fo;
    }

    Object* FunctionType::newObject() const { return new FunctionObject(this); }

    Value FunctionType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._PointerFunc)(*n, thread));
    }

    void FunctionType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        Pointer* pp = reinterpret_cast<Pointer*>(p);
        *pp = (*n->func()._PointerFunc)(*n, thread);
    }

    void FunctionType::outputNode(std::ostream&, const Node*) const {}

    void FunctionType::outputValueRecursive(std::ostream& o,
                                            const ValuePointer vp,
                                            ValueOutputState& state) const
    {
        const FunctionObject* obj =
            *reinterpret_cast<const FunctionObject**>(vp);

        if (obj)
        {
            if (state.traversedObjects.find(obj)
                != state.traversedObjects.end())
            {
                o << "...ad infinitum...";
            }
            else
            {
                state.traversedObjects.insert(obj);

                const FunctionType* type =
                    static_cast<const FunctionType*>(obj->type());

                if (const Function* f = obj->function())
                {
                    if (f->isLambda())
                    {
                        f->output(o);
                        o << " ";
                        NodePrinter printer(f->body(), o, state,
                                            NodePrinter::Lispy);
                        printer.traverse();
                    }
                    else if (type->name() == "(;)")
                    {
                        o << f->fullyQualifiedName();
                    }
                    else
                    {
                        // f->output(o);
                        o << f->fullyQualifiedName();
                    }
                }
                else
                {
                    output(o);
                }

                // state.traversedObjects.erase(obj);
            }
        }
        else
        {
            o << "nil";
        }
    }

    void FunctionType::serialize(std::ostream& o, Archive::Writer& archive,
                                 const ValuePointer p) const
    {
        const FunctionObject* obj =
            *reinterpret_cast<const FunctionObject**>(p);

        unsigned char flags = 0;

        if (obj->function()->native())
            flags |= 0x1;
        if (obj->dependent())
            flags |= 0x2;
        archive.writeByte(o, flags);
        archive.writeNameId(o, obj->function()->fullyQualifiedName());
    }

    void FunctionType::deserialize(std::istream& i, Archive::Reader& archive,
                                   ValuePointer p) const
    {
        FunctionObject* obj = *reinterpret_cast<FunctionObject**>(p);
        unsigned char flags = archive.readByte(i);

        Name fname = archive.readNameId(i);
        const Function* F =
            _context->findSymbolOfTypeByQualifiedName<Function>(fname, false);
        assert(F);
        obj->_function = F;
    }

    void FunctionType::reconstitute(Archive::Reader& archive, Object* o) const
    {
        FunctionObject* obj = static_cast<FunctionObject*>(o);
    }

    void FunctionType::load()
    {
        Context::PrimaryBit fence(context(), false);
        USING_MU_FUNCTION_SYMBOLS;

        //
        //  Try and resolve the signature
        //

        Symbol* s = scope();
        String tname = name();
        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();
        const char* an = "(;)";

        size_t size = signature() ? signature()->size() : 0;
        STLVector<String>::Type args(size);
        String rt = "void";
        const MachineRep* rep = 0;

        if (signature() && signature()->size())
        {
            args[0] = tn;

            if (signature()->resolved())
            {
                const Type* t = static_cast<const Type*>(
                    signature()->types().front().symbol);
                rt = t->fullyQualifiedName().c_str();
                rep = t->machineRep();

                for (int i = 1; i < size; i++)
                {
                    args[i] = signature()
                                  ->types()[i]
                                  .symbol->fullyQualifiedName()
                                  .c_str();
                }
            }
            else
            {
                rt = Name(signature()->types().front().name).c_str();

                for (int i = 1; i < size; i++)
                {
                    args[i] = Name(signature()->types()[i].name).c_str();
                }
            }
        }

        //     if (size != 0)
        //     {
        //         //
        //         //  This is not the anonymous function type so it gets
        //         //  a dynamic activation
        //         //

        //         addSymbol( new DynamicActivation(rt, args) );
        //     }

        Context* c = context();

        addSymbol(
            new Function(c, "()", NodeFunc(0),
                         Function::DynamicActivation
                             | Function::ContextDependent | Function::MaybePure,
                         Return, rt.c_str(), Function::ArgVector, &args, End));

        s->addSymbols(new ReferenceType(c, rn, this),

                      new Function(c, tn, FunctionType::dereference, Cast,
                                   Return, tn, Args, rn, End),

                      new Function(c, "=", FunctionType::assign, AsOp, Return,
                                   rn, Args, rn, tn, End),

                      new Function(c, "print", FunctionType::print, None,
                                   Return, "void", Args, tn, End),

                      EndArguments);

        if (signature())
        {
            s->addSymbols(new Function(c, tn, FunctionType::disambiguate, Cast,
                                       Return, tn, Args, an, End),

                          new Function(c, tn, FunctionType::ambiguate, Cast,
                                       Return, an, Args, tn, End),

                          EndArguments);
        }
    };

    NODE_IMPLEMENTATION(FunctionType::print, void)
    {
        FunctionObject* o = NODE_ARG_OBJECT(0, FunctionObject);
        o->type()->outputValue(cout, Value(o));
    }

    NODE_IMPLEMENTATION(FunctionType::assign, Pointer)
    {
        Pointer* i = reinterpret_cast<Pointer*>(NODE_ARG(0, Pointer));
        Pointer o = NODE_ARG(1, Pointer);
        *i = o;
        NODE_RETURN(Pointer(i));
    }

    NODE_IMPLEMENTATION(FunctionType::eq, bool)
    {
        Pointer a = NODE_ARG(0, Pointer);
        Pointer b = NODE_ARG(1, Pointer);
        NODE_RETURN(a == b);
    }

    NODE_IMPLEMENTATION(FunctionType::dereference, Pointer)
    {
        Pointer* pp = NODE_ARG_OBJECT(0, Pointer);
        NODE_RETURN(*pp);
    }

    NODE_IMPLEMENTATION(FunctionType::ambiguate, Pointer)
    {
        NODE_RETURN((Pointer)NODE_ARG_OBJECT(0, FunctionObject));
    }

    NODE_IMPLEMENTATION(FunctionType::disambiguate, Pointer)
    {
        Process* p = NODE_THREAD.process();
        FunctionObject* o = NODE_ARG_OBJECT(0, FunctionObject);
        const FunctionType* ftype =
            static_cast<const FunctionType*>(NODE_THIS.type());

        if (o)
        {
            const Function* func = o->function();

            if (func->isLambda())
            {
                if (func->type() == ftype)
                {
                    NODE_RETURN((Pointer)o);
                }
            }
            else
            {
                for (const Function* f = func->firstFunctionOverload(); f;
                     f = f->nextFunctionOverload())
                {
                    if (f->type() == ftype)
                    {
                        o = new FunctionObject(f);
                        NODE_RETURN((Pointer)o);
                    }
                }
            }

            throw BadDynamicCastException(NODE_THREAD);
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(FunctionType::call, void) { cout << "CALL" << endl; }

    Type::MatchResult FunctionType::match(const Type* other, Bindings& b) const
    {
        if (const FunctionType* ft = dynamic_cast<const FunctionType*>(other))
        {
            const Signature* a = signature();
            const Signature* b = ft->signature();

            if (a->size() != b->size())
                return NoMatch;

            for (int i = 0; i < a->size(); i++)
            {
                const Type* atype = static_cast<const Type*>((*a)[i].symbol);
                const Type* btype = static_cast<const Type*>((*b)[i].symbol);
                if (atype == btype)
                    continue;

                if (i)
                {
                    //
                    //  My argument must match (derived is-a base) the other.
                    //

                    if (!btype->match(atype))
                        return NoMatch;
                }
                else
                {
                    //
                    //  Return type of this must match the other
                    //

                    if (!atype->match(btype))
                        return NoMatch;
                }
            }

            return Match;
        }
        else
        {
            //
            //  Not a function type
            //

            return Type::match(other, b);
        }
    }

} // namespace Mu
