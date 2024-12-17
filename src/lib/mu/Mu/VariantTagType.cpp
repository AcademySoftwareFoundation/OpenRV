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
#include <Mu/MachineRep.h>
#include <Mu/Module.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolTable.h>
#include <Mu/VariantInstance.h>
#include <Mu/VariantTagType.h>
#include <algorithm>

namespace Mu
{
    using namespace std;

    VariantTagType::VariantTagType(Context* context, const char* name,
                                   const Type* rep)
        : Type(context, name, PointerRep::rep())
        , _index(0)
    {
        _repType.symbol = rep, _isPrimitive = false;
        _state = ResolvedState;
        _isGCAtomic = rep->isGCAtomic();
        _isSerializable = true;
    }

    VariantTagType::VariantTagType(Context* context, const char* name,
                                   const char* rep)
        : Type(context, name, PointerRep::rep())
        , _index(0)
    {
        _repType.name = context->internName(rep).nameRef();
        _isPrimitive = false;
    }

    VariantTagType::~VariantTagType() {}

    bool VariantTagType::resolveSymbols() const
    {
        const Module* global = globalModule();

        const Type* type =
            global->findSymbolOfTypeByQualifiedName<Type>(_repType.name);

        if (type)
        {
            _repType.symbol = type;
        }
        else
        {
#if 0
        cerr << "ERROR: unable to resolve " << _repType.name 
             << " in VariantTagType " << name()
             << endl;
#endif

            return false;
        }

        return true;
    }

    Type::MatchResult VariantTagType::match(const Type* type, Bindings& b) const
    {
        if (symbolState() != ResolvedState)
            resolve();
        return Type::match(type, b);
    }

    Object* VariantTagType::newObject() const
    {
        if (symbolState() != ResolvedState)
            resolve();
        return VariantInstance::allocate(this);
    }

    size_t VariantTagType::objectSize() const
    {
        if (symbolState() != ResolvedState)
            resolve();
        const Type* t = representationType();
        return t->objectSize() + sizeof(VariantInstance);
    }

    void VariantTagType::constructInstance(Pointer p) const
    {
        new (p) VariantInstance(this);
    }

    void VariantTagType::copyInstance(Pointer a, Pointer b) const
    {
        VariantInstance* src = reinterpret_cast<VariantInstance*>(a);
        VariantInstance* dst = reinterpret_cast<VariantInstance*>(b);
        representationType()->copyInstance(src->structure(), dst->structure());
    }

    Value VariantTagType::nodeEval(const Node* n, Thread& thread) const
    {
        return Value((*n->func()._PointerFunc)(*n, thread));
    }

    void VariantTagType::nodeEval(void* p, const Node* n, Thread& thread) const
    {
        Pointer* pp = reinterpret_cast<Pointer*>(p);
        *pp = (*n->func()._PointerFunc)(*n, thread);
    }

    const Type* VariantTagType::nodeReturnType(const Node*) const
    {
        if (symbolState() != ResolvedState)
            resolve();
        return variantType();
    }

    const Type* VariantTagType::fieldType(size_t t) const
    {
        if (t > 0)
            return 0;
        if (symbolState() != ResolvedState)
            resolve();
        return _repType.symbolOfType<Type>();
    }

    ValuePointer VariantTagType::fieldPointer(Object* o, size_t index) const
    {
        if (index > 0)
            return 0;
        VariantInstance* i = static_cast<VariantInstance*>(o);
        return i->field(index);
    }

    const ValuePointer VariantTagType::fieldPointer(const Object* o,
                                                    size_t index) const
    {
        if (index > 0)
            return 0;
        const VariantInstance* i = static_cast<const VariantInstance*>(o);
        return i->field(index);
    }

    void VariantTagType::outputValueRecursive(ostream& o, const ValuePointer p,
                                              ValueOutputState& state) const
    {
        VariantInstance* obj = *reinterpret_cast<VariantInstance**>(p);

        if (obj)
        {
            o << fullyQualifiedName();

            if (state.traversedObjects.find(obj)
                != state.traversedObjects.end())
            {
                o << "...ad infinitum...";
            }
            else
            {
                state.traversedObjects.insert(obj);

                if (representationType()
                    != globalModule()->context()->voidType())
                {
                    o << " {";

                    if (const Class* c =
                            dynamic_cast<const Class*>(representationType()))
                    {
                        Object* vobj = obj->data<Object>();
                        representationType()->outputValueRecursive(
                            o, ValuePointer(&vobj), state);
                    }
                    else
                    {
                        void* vobj = obj->data<void>();
                        representationType()->outputValueRecursive(
                            o, ValuePointer(vobj), state);
                    }

                    o << "}";
                }

                // state.traversedObjects.erase(obj);
            }
        }
        else
        {
            o << "nil";
        }
    }

    void VariantTagType::outputValue(ostream& o, const Value& value,
                                     bool full) const
    {
        ValueOutputState state(o, full);
        outputValueRecursive(o, ValuePointer(&value._Pointer), state);
    }

    void VariantTagType::serialize(ostream& o, Archive::Writer& writer,
                                   const ValuePointer p) const
    {
        VariantInstance* i = *(VariantInstance**)p;
        Pointer pp = i->structure();
        const Type* rtype = representationType();

        if (rtype->isPrimitiveType())
        {
            representationType()->serialize(o, writer, pp);
        }
        else
        {
            representationType()->serialize(o, writer, &pp);
        }
    }

    void VariantTagType::deserialize(istream& in, Archive::Reader& reader,
                                     ValuePointer p) const
    {
        VariantInstance* i = *(VariantInstance**)p;
        Pointer pp = i->structure();
        const Type* rtype = representationType();

        if (rtype->isPrimitiveType())
        {
            representationType()->deserialize(in, reader, pp);
        }
        else
        {
            representationType()->deserialize(in, reader, &pp);
        }
    }

    void VariantTagType::reconstitute(Archive::Reader& reader,
                                      Object* obj) const
    {
        VariantInstance* i = (VariantInstance*)obj;

        if (!representationType()->isPrimitiveType())
        {
            representationType()->reconstitute(reader, (Object*)i->structure());
        }
    }

    void VariantTagType::load()
    {
        Type::load();
        Context* context = this->context();

        USING_MU_FUNCTION_SYMBOLS;

        const Type* reptype = representationType();

        Symbol* s = scope();
        Symbol* g = s->globalScope();
        String fsname = s->fullyQualifiedName();
        String ttname = reptype->fullyQualifiedName();
        String tname = name();
        String ftname = fullyQualifiedName();
        String rname = tname + "&";
        const char* tn = tname.c_str();
        const char* ftn = ftname.c_str();
        const char* rn = rname.c_str();
        const char* fsn = fsname.c_str();
        const char* tt = ttname.c_str();

        //
        //  Add reference type
        //

        ReferenceType* rt = new ReferenceType(context, rn, this);
        scope()->addSymbol(rt);

        String rtfname = rt->fullyQualifiedName();
        const char* rtf = rtfname.c_str();

        VariantType* v = dynamic_cast<VariantType*>(scope());

        //
        //  Dereference
        //

        if (reptype != context->voidType())
        {
            Function* dr = new Function(context, tn, BaseFunctions::dereference,
                                        Cast, Return, ftn, Args, rtf, End);

            s->addSymbol(dr);

            Function* OpAs = new Function(
                context, "=", BaseFunctions::assign,
                Function::MemberOperator | Function::Operator, Return,
                rt->fullyQualifiedName().c_str(), Args, rtf, ftn, End);

            g->addSymbol(OpAs);

            Function* FT = new Function(
                context, tn, reptype->machineRep()->variantConstructorFunc(),
                Mapped, Return, v->fullyQualifiedName().c_str(), Args,
                reptype->fullyQualifiedName().c_str(), End);

            addSymbol(FT);
        }
        else
        {
            addSymbol(new Function(
                context, tn, reptype->machineRep()->variantConstructorFunc(),
                Mapped, Return, v->fullyQualifiedName().c_str(), End));
        }

        //
        //
        //

        s->addSymbols(new Function(context, tn, VariantTagType::upcast, Cast,
                                   Return, ftn, Args, fsn, End),

                      EndArguments);

        addSymbols(new Function(context, "__unpack",
                                reptype->machineRep()->unpackVariant(), Cast,
                                Return, tt, Args, ftn, End),

                   EndArguments);
    }

    const Type* VariantTagType::representationType() const
    {
        if (!isResolved())
            resolve();
        return _repType.symbolOfType<Type>();
    }

    NODE_IMPLEMENTATION(VariantTagType::upcast, Pointer)
    {
        if (VariantInstance* i = NODE_ARG_OBJECT(0, VariantInstance))
        {
            if (i->tagType() != NODE_THIS.type())
            {
                //
                //  "Return" as pattern failure
                //

                NODE_THREAD.jump(JumpReturnCode::PatternFail, 1);
                NODE_RETURN(0);
            }

            NODE_RETURN(i);
        }

        throw NilArgumentException(NODE_THREAD);
    }

} // namespace Mu
