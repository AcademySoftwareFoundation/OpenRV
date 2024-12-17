//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/FunctionObject.h>
#include <Mu/FunctionType.h>
#include <Mu/GarbageCollector.h>
#include <Mu/GlobalVariable.h>
#include <Mu/Interface.h>
#include <Mu/InterfaceImp.h>
#include <Mu/MachineRep.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/StackVariable.h>
#include <Mu/Thread.h>
#include <Mu/VariantInstance.h>
#include <Mu/Vector.h>
#include <iostream>
#include <stl_ext/stl_ext_algo.h>

#ifdef _MSC_VER
#define __alignof__ __alignof
#endif

#ifdef _MSC_VER
#define LOCAL_ARRAY(NAME, TYPE, SIZE) \
    TYPE* NAME = (TYPE*)alloca((SIZE) * sizeof(TYPE))
#else
#define LOCAL_ARRAY(NAME, TYPE, SIZE) TYPE NAME[SIZE]
#endif

namespace Mu
{
    using namespace std;
    using namespace stl_ext;

    GenericMachine::GenericMachine()
    {
        GarbageCollector::init();
        new VoidRep;
        new FloatRep;
        new DoubleRep;
        new IntRep;
        new Int64Rep;
        new ShortRep;
        new CharRep;
        new BoolRep;
        new PointerRep;
        new Vector4FloatRep;
        new Vector3FloatRep;
        new Vector2FloatRep;
    }

    GenericMachine::~GenericMachine() { MachineRep::deleteAll(); }

    //----------------------------------------------------------------------

    STLVector<MachineRep*>::Type MachineRep::_allReps;

    MachineRep::MachineRep(const char* name, const char* fmtName)
        : _constantFunc(0)
        , _referenceStackFunc(0)
        , _dereferenceStackFunc(0)
        , _referenceGlobalFunc(0)
        , _dereferenceGlobalFunc(0)
        , _referenceMemberFunc(0)
        , _dereferenceMemberFunc(0)
        , _extractMemberFunc(0)
        , _frameBlockFunc(0)
        , _simpleBlockFunc(0)
        , _patternBlockFunc(0)
        , _functionActivationFunc(0)
        , _functionReturnFunc(0)
        , _dynamicActivationFunc(0)
        , _functionTailFuseFunc(0)
        , _dereferenceClassMemberFunc(0)
        , _referenceClassMemberFunc(MachineRep::referenceClassMember)
        , _callMethodFunc(0)
        , _invokeInterfaceFunc(0)
        , _variantConstructorFunc(0)
        , _size(0)
        , _structAlignment(0)
        , _naturalAlignment(0)
        , _fmtName(fmtName)
        , _elementRep(this)
        , _width(1)
        , _name(name)
    {
        _allReps.push_back(this);
    }

    MachineRep::MachineRep(const char* name, const char* fmtName,
                           MachineRep* elementRep, size_t width)
        : _constantFunc(0)
        , _referenceStackFunc(0)
        , _dereferenceStackFunc(0)
        , _referenceGlobalFunc(0)
        , _dereferenceGlobalFunc(0)
        , _referenceMemberFunc(0)
        , _dereferenceMemberFunc(0)
        , _extractMemberFunc(0)
        , _frameBlockFunc(0)
        , _simpleBlockFunc(0)
        , _patternBlockFunc(0)
        , _functionActivationFunc(0)
        , _functionReturnFunc(0)
        , _dynamicActivationFunc(0)
        , _functionTailFuseFunc(0)
        , _dereferenceClassMemberFunc(0)
        , _referenceClassMemberFunc(MachineRep::referenceClassMember)
        , _callMethodFunc(0)
        , _invokeInterfaceFunc(0)
        , _variantConstructorFunc(0)
        , _size(0)
        , _structAlignment(0)
        , _naturalAlignment(0)
        , _fmtName(fmtName)
        , _elementRep(elementRep)
        , _width(width)
        , _name(name)
    {
        _allReps.push_back(this);
    }

    MachineRep::~MachineRep() {}

    void MachineRep::deleteAll()
    {
        //
        //	Delete all the MachineReps that have been stored. If any
        //	MachineRep is deleted, it is assumed they all will be.
        //

        delete_contents(MachineRep::_allReps);
        _allReps.clear();
    }

    NODE_IMPLEMENTATION(MachineRep::referenceClassMember, Pointer)
    {
        const MemberVariable* v =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);
        if (!i)
            throw NilArgumentException(NODE_THREAD);
        size_t offset = v->instanceOffset();
        NODE_RETURN((Pointer)(i->structure() + offset));
    }

    //----------------------------------------------------------------------

    FloatRep* FloatRep::_rep = 0;

    FloatRep::FloatRep()
        : MachineRep("float", "f")
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(float);
        _structAlignment = __alignof__(float);
        _naturalAlignment = __alignof__(float);
        _constantFunc = FloatRep::constant;
        _referenceStackFunc = FloatRep::referenceStack;
        _dereferenceStackFunc = FloatRep::dereferenceStack;
        _referenceGlobalFunc = FloatRep::referenceGlobal;
        _dereferenceGlobalFunc = FloatRep::dereferenceGlobal;
        _callMethodFunc = FloatRep::callMethod;
        _invokeInterfaceFunc = FloatRep::invokeInterface;
        _dereferenceClassMemberFunc = FloatRep::dereferenceClassMember;
        _frameBlockFunc = FloatRep::frameBlock;
        _simpleBlockFunc = FloatRep::simpleBlock;
        _patternBlockFunc = FloatRep::patternBlock;
        _functionActivationFunc = FloatRep::functionActivationFunc;
        _functionReturnFunc = FloatRep::functionReturnFunc;
        _dynamicActivationFunc = FloatRep::dynamicActivation;
        _functionTailFuseFunc = FloatRep::tailFuse;
        _variantConstructorFunc = FloatRep::variantConstructor;
        _unpackVariantFunc = FloatRep::unpackVariant;
    }

    FloatRep::~FloatRep() { _rep = 0; }

    ValuePointer FloatRep::valuePointer(Value& v) const
    {
        return ValuePointer(&v._float);
    }

    const ValuePointer FloatRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._float);
    }

    Value FloatRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<float*>(p));
    }

#define MACHINEFUNCS(CLASS, TYPE)                                             \
    NODE_IMPLEMENTATION(CLASS::referenceStack, Pointer)                       \
    {                                                                         \
        const StackVariable* sv =                                             \
            static_cast<const StackVariable*>(NODE_THIS.symbol());            \
        size_t index = sv->address() + NODE_THREAD.stackOffset();             \
        NODE_RETURN((Pointer) & (NODE_THREAD.stack()[index].as<TYPE>()));     \
    }                                                                         \
                                                                              \
    NODE_IMPLEMENTATION(CLASS::dereferenceStack, TYPE)                        \
    {                                                                         \
        const StackVariable* sv =                                             \
            static_cast<const StackVariable*>(NODE_THIS.symbol());            \
        size_t index = sv->address() + NODE_THREAD.stackOffset();             \
        NODE_RETURN(NODE_THREAD.stack()[index].as<TYPE>());                   \
    }                                                                         \
                                                                              \
    NODE_IMPLEMENTATION(CLASS::referenceGlobal, Pointer)                      \
    {                                                                         \
        const GlobalVariable* gv =                                            \
            static_cast<const GlobalVariable*>(NODE_THIS.symbol());           \
        Process* process = NODE_THREAD.process();                             \
        NODE_RETURN((Pointer) & (process->globals()[gv->address()]._##TYPE)); \
    }                                                                         \
                                                                              \
    NODE_IMPLEMENTATION(CLASS::dereferenceGlobal, TYPE)                       \
    {                                                                         \
        const GlobalVariable* gv =                                            \
            static_cast<const GlobalVariable*>(NODE_THIS.symbol());           \
        Process* process = NODE_THREAD.process();                             \
        NODE_RETURN(process->globals()[gv->address()]._##TYPE);               \
    }                                                                         \
                                                                              \
    NODE_IMPLEMENTATION(CLASS::constant, TYPE)                                \
    {                                                                         \
        TYPE v = NODE_DATA(TYPE);                                             \
        NODE_RETURN(v);                                                       \
    }

#define METHODFUNCS(CLASS, TYPE)                                      \
    NODE_IMPLEMENTATION(CLASS::callMethod, TYPE)                      \
    {                                                                 \
        const MemberFunction* f =                                     \
            static_cast<const MemberFunction*>(NODE_THIS.symbol());   \
        ClassInstance* i =                                            \
            reinterpret_cast<ClassInstance*>(NODE_ARG(0, Pointer));   \
        if (!i)                                                       \
            throw NilArgumentException(NODE_THREAD);                  \
        const MemberFunction* F = i->classType()->dynamicLookup(f);   \
        TYPE t;                                                       \
                                                                      \
        size_t nargs = NODE_THIS.numArgs();                           \
        LOCAL_ARRAY(argv, const Node*, nargs + 1);                    \
        DataNode dn(0, PointerRep::rep()->constantFunc(), i->type()); \
        dn._data._Pointer = i;                                        \
        argv[0] = &dn;                                                \
        argv[nargs] = 0;                                              \
        for (size_t i = 1; i < nargs; i++)                            \
            argv[i] = NODE_THIS.argNode(i);                           \
        Node n((Node**)argv, F);                                      \
                                                                      \
        try                                                           \
        {                                                             \
            t = (*F->func()._##TYPE##Func)(n, NODE_THREAD);           \
        }                                                             \
        catch (...)                                                   \
        {                                                             \
            n.releaseArgv();                                          \
            throw;                                                    \
        }                                                             \
                                                                      \
        n.releaseArgv();                                              \
        NODE_RETURN(t);                                               \
    }

#define CLASSMEMBERFUNCS(CLASS, TYPE)                                \
    NODE_IMPLEMENTATION(CLASS::dereferenceClassMember, TYPE)         \
    {                                                                \
        const MemberVariable* v =                                    \
            static_cast<const MemberVariable*>(NODE_THIS.symbol());  \
        ClassInstance* i = NODE_ARG_OBJECT(0, ClassInstance);        \
        if (!i)                                                      \
            throw NilArgumentException(NODE_THREAD);                 \
        size_t offset = v->instanceOffset();                         \
        TYPE* tp = reinterpret_cast<TYPE*>(i->structure() + offset); \
        NODE_RETURN(*tp);                                            \
    }

#define VARIANTFUNCS(CLASS, TYPE)                                            \
    NODE_IMPLEMENTATION(CLASS::unpackVariant, TYPE)                          \
    {                                                                        \
        VariantInstance* i = NODE_ARG_OBJECT(0, VariantInstance);            \
        NODE_RETURN(*(i->data<TYPE>()));                                     \
    }                                                                        \
                                                                             \
    NODE_IMPLEMENTATION(CLASS::variantConstructor, Pointer)                  \
    {                                                                        \
        const VariantTagType* c =                                            \
            static_cast<const VariantTagType*>(NODE_THIS.symbol()->scope()); \
        VariantInstance* i = VariantInstance::allocate(c);                   \
        *(i->data<TYPE>()) = NODE_ARG(0, TYPE);                              \
        NODE_RETURN(i);                                                      \
    }

//  NOTE: there needs to be a MU_SAFE_ARGUMENTS version of this too.
//  also, the return should be ignored on all but the last call.
#define FRAMEBLOCK(CLASS, TYPE)                                              \
    NODE_IMPLEMENTATION(CLASS::frameBlock, TYPE)                             \
    {                                                                        \
        Thread::StackRecord record(NODE_THREAD);                             \
        record.newStackFrame(NODE_DATA(int));                                \
        int n = NODE_NUM_ARGS() - 1;                                         \
                                                                             \
        for (int i = 0; i < n; i++)                                          \
            NODE_ANY_TYPE_ARG(i);                                            \
        TYPE v = NODE_ARG(n, TYPE);                                          \
                                                                             \
        NODE_RETURN(v);                                                      \
    }                                                                        \
                                                                             \
    NODE_IMPLEMENTATION(CLASS::simpleBlock, TYPE)                            \
    {                                                                        \
        int n = NODE_NUM_ARGS() - 1;                                         \
        for (int i = 0; i < n; i++)                                          \
            NODE_ANY_TYPE_ARG(i);                                            \
        TYPE v = NODE_ARG(n, TYPE);                                          \
                                                                             \
        NODE_RETURN(v);                                                      \
    }                                                                        \
                                                                             \
    NODE_IMPLEMENTATION(CLASS::patternBlock, TYPE)                           \
    {                                                                        \
        Thread::JumpRecord record(NODE_THREAD, JumpReturnCode::PatternFail); \
        int rv = SETJMP(NODE_THREAD.jumpPoint());                            \
                                                                             \
        switch (rv)                                                          \
        {                                                                    \
        case JumpReturnCode::PatternFail:                                    \
            NODE_THREAD.jumpPointRestore();                                  \
            throw PatternFailedException();                                  \
            break;                                                           \
                                                                             \
        case JumpReturnCode::NoJump:                                         \
        {                                                                    \
            int n = NODE_NUM_ARGS() - 1;                                     \
            for (int i = 0; i < n; i++)                                      \
                NODE_ANY_TYPE_ARG(i);                                        \
            TYPE v = NODE_ARG(n, TYPE);                                      \
            NODE_RETURN(v);                                                  \
        }                                                                    \
        };                                                                   \
                                                                             \
        NODE_RETURN(TYPE());                                                 \
    }

#define ACTIVATIONFUNCS(CLASS, TYPE)                                          \
    NODE_IMPLEMENTATION(CLASS::functionActivationFunc, TYPE)                  \
    {                                                                         \
        const Function* f = static_cast<const Function*>(NODE_THIS.symbol()); \
        int n = NODE_NUM_ARGS();                                              \
        int s = f->stackSize();                                               \
        Thread::StackRecord record(NODE_THREAD);                              \
        record.beginActivation(s);                                            \
                                                                              \
        Value val;                                                            \
        for (int i = 0; i < s; i++)                                           \
        {                                                                     \
            if (i < n)                                                        \
            {                                                                 \
                val = NODE_EVAL_VALUE(NODE_THIS.argNode(i), NODE_THREAD);     \
            }                                                                 \
            else                                                              \
            {                                                                 \
                zero(val);                                                    \
            }                                                                 \
                                                                              \
            record.setParameter(i, val);                                      \
        }                                                                     \
                                                                              \
        record.endParameters();                                               \
                                                                              \
        const Node* body = f->body();                                         \
        if (!body)                                                            \
            throw UnimplementedMethodException(NODE_THREAD);                  \
        TYPE v;                                                               \
                                                                              \
        if (NodeFunc func = body->func())                                     \
        {                                                                     \
            NODE_THREAD.jumpPointBegin(JumpReturnCode::Return                 \
                                       | JumpReturnCode::TailFuse);           \
            int rv = SETJMP(NODE_THREAD.jumpPoint());                         \
                                                                              \
            if (rv == JumpReturnCode::NoJump)                                 \
            {                                                                 \
                v = (func._##TYPE##Func)(*body, NODE_THREAD);                 \
            }                                                                 \
            else if (rv == JumpReturnCode::TailFuse)                          \
            {                                                                 \
                return CLASS::functionActivationFunc(                         \
                    *NODE_THREAD.continuation(), NODE_THREAD);                \
            }                                                                 \
            else                                                              \
            {                                                                 \
                NODE_THREAD.jumpPointRestore();                               \
                v = NODE_THREAD.returnValue()._##TYPE;                        \
            }                                                                 \
                                                                              \
            NODE_THREAD.jumpPointEnd();                                       \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            throw NilNodeFuncException(NODE_THREAD);                          \
        }                                                                     \
                                                                              \
        NODE_RETURN(v);                                                       \
    }                                                                         \
                                                                              \
    NODE_IMPLEMENTATION(CLASS::functionReturnFunc, TYPE)                      \
    {                                                                         \
        Value v(NODE_ARG(0, TYPE));                                           \
        NODE_THREAD.jump(JumpReturnCode::Return, 1, v);                       \
        NODE_RETURN(v._##TYPE);                                               \
    }                                                                         \
                                                                              \
    NODE_IMPLEMENTATION(CLASS::invokeInterface, TYPE)                         \
    {                                                                         \
        const Function* f = static_cast<const Function*>(NODE_THIS.symbol()); \
        const Interface* i = static_cast<const Interface*>(f->scope());       \
        ClassInstance* o =                                                    \
            reinterpret_cast<ClassInstance*>(NODE_ARG(0, Pointer));           \
                                                                              \
        if (const InterfaceImp* imp = o->classType()->implementation(i))      \
        {                                                                     \
            size_t offset = f->interfaceIndex();                              \
            NodeFunc func = imp->func(offset);                                \
                                                                              \
            size_t nargs = NODE_THIS.numArgs();                               \
            LOCAL_ARRAY(argv, const Node*, nargs + 1);                        \
            DataNode dn(0, PointerRep::rep()->constantFunc(), o->type());     \
            dn._data._Pointer = o;                                            \
            argv[0] = &dn;                                                    \
            argv[nargs] = 0;                                                  \
            for (size_t i = 1; i < nargs; i++)                                \
                argv[i] = NODE_THIS.argNode(i);                               \
            Node n((Node**)argv, f);                                          \
            TYPE t = (*func._##TYPE##Func)(n, NODE_THREAD);                   \
            n.releaseArgv();                                                  \
            NODE_RETURN(t);                                                   \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            throw BadInterfaceInvocationException(NODE_THREAD);               \
        }                                                                     \
    }                                                                         \
                                                                              \
    NODE_IMPLEMENTATION(CLASS::tailFuse, TYPE)                                \
    {                                                                         \
        NODE_THREAD.setContinuation(NODE_THIS.argNode(0));                    \
        NODE_THREAD.jump(JumpReturnCode::TailFuse);                           \
        Value v;                                                              \
        NODE_RETURN(v._##TYPE);                                               \
    }

#define DYNACTIVATIONFUNCS(CLASS, TYPE)                                \
    NODE_IMPLEMENTATION(CLASS::dynamicActivation, TYPE)                \
    {                                                                  \
        if (FunctionObject* fobj = NODE_ARG_OBJECT(0, FunctionObject)) \
        {                                                              \
            if (const Function* f = fobj->function())                  \
            {                                                          \
                Node node(NODE_THIS.argv() + 1, f);                    \
                NodeFunc nf = f->func(&node);                          \
                TYPE p;                                                \
                                                                       \
                try                                                    \
                {                                                      \
                    p = (*nf._##TYPE##Func)(node, NODE_THREAD);        \
                }                                                      \
                catch (...)                                            \
                {                                                      \
                    node.releaseArgv();                                \
                    throw;                                             \
                }                                                      \
                                                                       \
                node.releaseArgv();                                    \
                NODE_RETURN(p);                                        \
            }                                                          \
            else                                                       \
            {                                                          \
                throw NilArgumentException(NODE_THREAD);               \
            }                                                          \
        }                                                              \
        else                                                           \
        {                                                              \
            throw NilArgumentException(NODE_THREAD);                   \
        }                                                              \
    }

    ACTIVATIONFUNCS(FloatRep, float)
    DYNACTIVATIONFUNCS(FloatRep, float)
    FRAMEBLOCK(FloatRep, float)
    MACHINEFUNCS(FloatRep, float)
    METHODFUNCS(FloatRep, float)
    VARIANTFUNCS(FloatRep, float)
    CLASSMEMBERFUNCS(FloatRep, float)

    //----------------------------------------------------------------------

    DoubleRep* DoubleRep::_rep = 0;

    DoubleRep::DoubleRep()
        : MachineRep("double", "F")
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(double);
        _structAlignment = __alignof__(double);
        _naturalAlignment = __alignof__(double);
        _constantFunc = DoubleRep::constant;
        _referenceStackFunc = DoubleRep::referenceStack;
        _dereferenceStackFunc = DoubleRep::dereferenceStack;
        _referenceGlobalFunc = DoubleRep::referenceGlobal;
        _dereferenceGlobalFunc = DoubleRep::dereferenceGlobal;
        _callMethodFunc = DoubleRep::callMethod;
        _invokeInterfaceFunc = DoubleRep::invokeInterface;
        _dereferenceClassMemberFunc = DoubleRep::dereferenceClassMember;
        _frameBlockFunc = DoubleRep::frameBlock;
        _simpleBlockFunc = DoubleRep::simpleBlock;
        _patternBlockFunc = DoubleRep::patternBlock;
        _functionActivationFunc = DoubleRep::functionActivationFunc;
        _functionReturnFunc = DoubleRep::functionReturnFunc;
        _dynamicActivationFunc = DoubleRep::dynamicActivation;
        _functionTailFuseFunc = DoubleRep::tailFuse;
        _variantConstructorFunc = DoubleRep::variantConstructor;
        _unpackVariantFunc = DoubleRep::unpackVariant;
    }

    DoubleRep::~DoubleRep() { _rep = 0; }

    ValuePointer DoubleRep::valuePointer(Value& v) const { return &v._double; }

    const ValuePointer DoubleRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._double);
    }

    Value DoubleRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<double*>(p));
    }

    ACTIVATIONFUNCS(DoubleRep, double)
    DYNACTIVATIONFUNCS(DoubleRep, double)
    FRAMEBLOCK(DoubleRep, double)
    MACHINEFUNCS(DoubleRep, double)
    METHODFUNCS(DoubleRep, double)
    VARIANTFUNCS(DoubleRep, double)
    CLASSMEMBERFUNCS(DoubleRep, double)

    //----------------------------------------------------------------------

    IntRep* IntRep::_rep = 0;

    IntRep::IntRep()
        : MachineRep("int", "i")
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(int);
        _structAlignment = __alignof__(int);
        _naturalAlignment = __alignof__(int);
        _constantFunc = IntRep::constant;
        _referenceStackFunc = IntRep::referenceStack;
        _dereferenceStackFunc = IntRep::dereferenceStack;
        _referenceGlobalFunc = IntRep::referenceGlobal;
        _dereferenceGlobalFunc = IntRep::dereferenceGlobal;
        _callMethodFunc = IntRep::callMethod;
        _invokeInterfaceFunc = IntRep::invokeInterface;
        _dereferenceClassMemberFunc = IntRep::dereferenceClassMember;
        _frameBlockFunc = IntRep::frameBlock;
        _simpleBlockFunc = IntRep::simpleBlock;
        _patternBlockFunc = IntRep::patternBlock;
        _functionActivationFunc = IntRep::functionActivationFunc;
        _functionReturnFunc = IntRep::functionReturnFunc;
        _dynamicActivationFunc = IntRep::dynamicActivation;
        _functionTailFuseFunc = IntRep::tailFuse;
        _variantConstructorFunc = IntRep::variantConstructor;
        _unpackVariantFunc = IntRep::unpackVariant;
    }

    IntRep::~IntRep() { _rep = 0; }

    ValuePointer IntRep::valuePointer(Value& v) const { return &v._int; }

    const ValuePointer IntRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._int);
    }

    Value IntRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<int*>(p));
    }

    MACHINEFUNCS(IntRep, int)
    FRAMEBLOCK(IntRep, int)
    METHODFUNCS(IntRep, int)
    VARIANTFUNCS(IntRep, int)
    CLASSMEMBERFUNCS(IntRep, int)
    ACTIVATIONFUNCS(IntRep, int);
    DYNACTIVATIONFUNCS(IntRep, int);

    //----------------------------------------------------------------------

    Int64Rep* Int64Rep::_rep = 0;

    Int64Rep::Int64Rep()
        : MachineRep("int64", "L")
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(int64);
        //_structAlignment	  	= __alignof__(int64);
        _structAlignment = 4; // __alignof__ not working on IA32 OS X?
        _naturalAlignment = 8;
        _constantFunc = Int64Rep::constant;
        _referenceStackFunc = Int64Rep::referenceStack;
        _dereferenceStackFunc = Int64Rep::dereferenceStack;
        _referenceGlobalFunc = Int64Rep::referenceGlobal;
        _dereferenceGlobalFunc = Int64Rep::dereferenceGlobal;
        _callMethodFunc = Int64Rep::callMethod;
        _invokeInterfaceFunc = Int64Rep::invokeInterface;
        _dereferenceClassMemberFunc = Int64Rep::dereferenceClassMember;
        _frameBlockFunc = Int64Rep::frameBlock;
        _simpleBlockFunc = Int64Rep::simpleBlock;
        _patternBlockFunc = Int64Rep::patternBlock;
        _functionActivationFunc = Int64Rep::functionActivationFunc;
        _functionReturnFunc = Int64Rep::functionReturnFunc;
        _dynamicActivationFunc = Int64Rep::dynamicActivation;
        _functionTailFuseFunc = Int64Rep::tailFuse;
        _variantConstructorFunc = Int64Rep::variantConstructor;
        _unpackVariantFunc = Int64Rep::unpackVariant;
    }

    Int64Rep::~Int64Rep() { _rep = 0; }

    ValuePointer Int64Rep::valuePointer(Value& v) const { return &v._int64; }

    const ValuePointer Int64Rep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._int64);
    }

    Value Int64Rep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<int64*>(p));
    }

    MACHINEFUNCS(Int64Rep, int64)
    FRAMEBLOCK(Int64Rep, int64)
    METHODFUNCS(Int64Rep, int64)
    VARIANTFUNCS(Int64Rep, int64)
    CLASSMEMBERFUNCS(Int64Rep, int64)
    ACTIVATIONFUNCS(Int64Rep, int64);
    DYNACTIVATIONFUNCS(Int64Rep, int64);

    //----------------------------------------------------------------------

    ShortRep* ShortRep::_rep = 0;

    ShortRep::ShortRep()
        : MachineRep("short", "s")
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(short);
        _structAlignment = __alignof__(short);
        _naturalAlignment = __alignof__(short);
        _constantFunc = ShortRep::constant;
        _referenceStackFunc = ShortRep::referenceStack;
        _dereferenceStackFunc = ShortRep::dereferenceStack;
        _referenceGlobalFunc = ShortRep::referenceGlobal;
        _dereferenceGlobalFunc = ShortRep::dereferenceGlobal;
        _callMethodFunc = ShortRep::callMethod;
        _invokeInterfaceFunc = ShortRep::invokeInterface;
        _dereferenceClassMemberFunc = ShortRep::dereferenceClassMember;
        _frameBlockFunc = ShortRep::frameBlock;
        _simpleBlockFunc = ShortRep::simpleBlock;
        _patternBlockFunc = ShortRep::patternBlock;
        _functionActivationFunc = ShortRep::functionActivationFunc;
        _functionReturnFunc = ShortRep::functionReturnFunc;
        _dynamicActivationFunc = ShortRep::dynamicActivation;
        _functionTailFuseFunc = ShortRep::tailFuse;
        _variantConstructorFunc = ShortRep::variantConstructor;
        _unpackVariantFunc = ShortRep::unpackVariant;
    }

    ShortRep::~ShortRep() { _rep = 0; }

    ValuePointer ShortRep::valuePointer(Value& v) const { return &v._short; }

    const ValuePointer ShortRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._short);
    }

    Value ShortRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<short*>(p));
    }

    MACHINEFUNCS(ShortRep, short)
    FRAMEBLOCK(ShortRep, short)
    METHODFUNCS(ShortRep, short)
    VARIANTFUNCS(ShortRep, short)
    CLASSMEMBERFUNCS(ShortRep, short)
    ACTIVATIONFUNCS(ShortRep, short);
    DYNACTIVATIONFUNCS(ShortRep, short);

    //----------------------------------------------------------------------

    CharRep* CharRep::_rep = 0;

    CharRep::CharRep()
        : MachineRep("char", "c")
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(char);
        _structAlignment = __alignof__(char);
        _naturalAlignment = __alignof__(char);
        _constantFunc = CharRep::constant;
        _referenceStackFunc = CharRep::referenceStack;
        _dereferenceStackFunc = CharRep::dereferenceStack;
        _referenceGlobalFunc = CharRep::referenceGlobal;
        _dereferenceGlobalFunc = CharRep::dereferenceGlobal;
        _callMethodFunc = CharRep::callMethod;
        _invokeInterfaceFunc = CharRep::invokeInterface;
        _dereferenceClassMemberFunc = CharRep::dereferenceClassMember;
        _frameBlockFunc = CharRep::frameBlock;
        _simpleBlockFunc = CharRep::simpleBlock;
        _patternBlockFunc = CharRep::patternBlock;
        _functionActivationFunc = CharRep::functionActivationFunc;
        _functionReturnFunc = CharRep::functionReturnFunc;
        _dynamicActivationFunc = CharRep::dynamicActivation;
        _functionTailFuseFunc = CharRep::tailFuse;
        _variantConstructorFunc = CharRep::variantConstructor;
        _unpackVariantFunc = CharRep::unpackVariant;
    }

    CharRep::~CharRep() { _rep = 0; }

    ValuePointer CharRep::valuePointer(Value& v) const { return &v._char; }

    const ValuePointer CharRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._char);
    }

    Value CharRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<char*>(p));
    }

    MACHINEFUNCS(CharRep, char)
    FRAMEBLOCK(CharRep, char)
    METHODFUNCS(CharRep, char)
    VARIANTFUNCS(CharRep, char)
    CLASSMEMBERFUNCS(CharRep, char)
    ACTIVATIONFUNCS(CharRep, char)
    DYNACTIVATIONFUNCS(CharRep, char)

    //----------------------------------------------------------------------

    BoolRep* BoolRep::_rep = 0;

    BoolRep::BoolRep()
        : MachineRep("bool", "b")
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(bool);
        _structAlignment = __alignof__(bool);
        _naturalAlignment = __alignof__(bool);
        _constantFunc = BoolRep::constant;
        _referenceStackFunc = BoolRep::referenceStack;
        _dereferenceStackFunc = BoolRep::dereferenceStack;
        _referenceGlobalFunc = BoolRep::referenceGlobal;
        _dereferenceGlobalFunc = BoolRep::dereferenceGlobal;
        _callMethodFunc = BoolRep::callMethod;
        _invokeInterfaceFunc = BoolRep::invokeInterface;
        _dereferenceClassMemberFunc = BoolRep::dereferenceClassMember;
        _frameBlockFunc = BoolRep::frameBlock;
        _simpleBlockFunc = BoolRep::simpleBlock;
        _patternBlockFunc = BoolRep::patternBlock;
        _functionActivationFunc = BoolRep::functionActivationFunc;
        _functionReturnFunc = BoolRep::functionReturnFunc;
        _dynamicActivationFunc = BoolRep::dynamicActivation;
        _functionTailFuseFunc = BoolRep::tailFuse;
        _variantConstructorFunc = BoolRep::variantConstructor;
        _unpackVariantFunc = BoolRep::unpackVariant;
    }

    BoolRep::~BoolRep() { _rep = 0; }

    ValuePointer BoolRep::valuePointer(Value& v) const { return &v._bool; }

    const ValuePointer BoolRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._bool);
    }

    Value BoolRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<bool*>(p));
    }

    MACHINEFUNCS(BoolRep, bool)
    FRAMEBLOCK(BoolRep, bool)
    METHODFUNCS(BoolRep, bool)
    VARIANTFUNCS(BoolRep, bool)
    CLASSMEMBERFUNCS(BoolRep, bool)
    ACTIVATIONFUNCS(BoolRep, bool)
    DYNACTIVATIONFUNCS(BoolRep, bool)

    //----------------------------------------------------------------------

    static NODE_IMPLEMENTATION(xconstant, Pointer)
    {
        const DataNode& dn = static_cast<const DataNode&>(NODE_THIS);
        Pointer v = NODE_DATA(Pointer);
        NODE_RETURN(v);
    }

    PointerRep* PointerRep::_rep = 0;

    PointerRep::PointerRep()
        : MachineRep("Pointer", "p")
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(void*);
        _structAlignment = __alignof__(void*);
        _naturalAlignment = __alignof__(void*);
        _constantFunc = PointerRep::constant;
        //_constantFunc               = xconstant;
        _referenceStackFunc = PointerRep::referenceStack;
        _dereferenceStackFunc = PointerRep::dereferenceStack;
        _referenceGlobalFunc = PointerRep::referenceGlobal;
        _dereferenceGlobalFunc = PointerRep::dereferenceGlobal;
        _callMethodFunc = PointerRep::callMethod;
        _invokeInterfaceFunc = PointerRep::invokeInterface;
        _dereferenceClassMemberFunc = PointerRep::dereferenceClassMember;
        _frameBlockFunc = PointerRep::frameBlock;
        _simpleBlockFunc = PointerRep::simpleBlock;
        _patternBlockFunc = PointerRep::patternBlock;
        _functionActivationFunc = PointerRep::functionActivationFunc;
        _functionReturnFunc = PointerRep::functionReturnFunc;
        _dynamicActivationFunc = PointerRep::dynamicActivation;
        _functionTailFuseFunc = PointerRep::tailFuse;
        _variantConstructorFunc = PointerRep::variantConstructor;
        _unpackVariantFunc = PointerRep::unpackVariant;
    }

    PointerRep::~PointerRep() { _rep = 0; }

    ValuePointer PointerRep::valuePointer(Value& v) const
    {
        return &v._Pointer;
    }

    const ValuePointer PointerRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._Pointer);
    }

    Value PointerRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<Pointer*>(p));
    }

    MACHINEFUNCS(PointerRep, Pointer)
    FRAMEBLOCK(PointerRep, Pointer)
    METHODFUNCS(PointerRep, Pointer)
    // unique variant func
    CLASSMEMBERFUNCS(PointerRep, Pointer)
    ACTIVATIONFUNCS(PointerRep, Pointer)
    DYNACTIVATIONFUNCS(PointerRep, Pointer)

    NODE_IMPLEMENTATION(PointerRep::unpackVariant, Pointer)
    {
        VariantInstance* i = NODE_ARG_OBJECT(0, VariantInstance);
        NODE_RETURN(i->structure());
    }

    NODE_IMPLEMENTATION(PointerRep::variantConstructor, Pointer)
    {
        const VariantTagType* c =
            static_cast<const VariantTagType*>(NODE_THIS.symbol()->scope());
        VariantInstance* i = VariantInstance::allocate(c);
        const Type* t = i->tagType()->representationType();
        t->copyInstance(NODE_ARG(0, Pointer), i->structure());
        NODE_RETURN(i);
    }

    //----------------------------------------------------------------------

    Vector4FloatRep* Vector4FloatRep::_rep = 0;

    Vector4FloatRep::Vector4FloatRep()
        : MachineRep("Vector4f", "4f", FloatRep::rep(), 4)
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(Vector4f);
        _structAlignment = __alignof__(Vector4f);
        _naturalAlignment = __alignof__(Vector4f);
        _constantFunc = Vector4FloatRep::constant;
        _referenceStackFunc = Vector4FloatRep::referenceStack;
        _dereferenceStackFunc = Vector4FloatRep::dereferenceStack;
        _referenceGlobalFunc = Vector4FloatRep::referenceGlobal;
        _dereferenceGlobalFunc = Vector4FloatRep::dereferenceGlobal;
        _referenceMemberFunc = Vector4FloatRep::referenceMember;
        _dereferenceMemberFunc = Vector4FloatRep::dereferenceMember;
        _extractMemberFunc = Vector4FloatRep::extractMember;
        _callMethodFunc = Vector4FloatRep::callMethod;
        _invokeInterfaceFunc = Vector4FloatRep::invokeInterface;
        _dereferenceClassMemberFunc = Vector4FloatRep::dereferenceClassMember;
        _frameBlockFunc = Vector4FloatRep::frameBlock;
        _simpleBlockFunc = Vector4FloatRep::simpleBlock;
        _patternBlockFunc = Vector4FloatRep::patternBlock;
        _functionActivationFunc = Vector4FloatRep::functionActivationFunc;
        _functionReturnFunc = Vector4FloatRep::functionReturnFunc;
        _dynamicActivationFunc = Vector4FloatRep::dynamicActivation;
        _functionTailFuseFunc = Vector4FloatRep::tailFuse;
        _variantConstructorFunc = Vector4FloatRep::variantConstructor;
        _unpackVariantFunc = Vector4FloatRep::unpackVariant;
    }

    Vector4FloatRep::~Vector4FloatRep() { _rep = 0; }

    ValuePointer Vector4FloatRep::valuePointer(Value& v) const
    {
        return ValuePointer(&v._Vector4f);
    }

    const ValuePointer Vector4FloatRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._Vector4f);
    }

    Value Vector4FloatRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<Vector4f*>(p));
    }

    MACHINEFUNCS(Vector4FloatRep, Vector4f)
    FRAMEBLOCK(Vector4FloatRep, Vector4f)
    METHODFUNCS(Vector4FloatRep, Vector4f)
    VARIANTFUNCS(Vector4FloatRep, Vector4f)
    CLASSMEMBERFUNCS(Vector4FloatRep, Vector4f)
    ACTIVATIONFUNCS(Vector4FloatRep, Vector4f)
    DYNACTIVATIONFUNCS(Vector4FloatRep, Vector4f)

    NODE_IMPLEMENTATION(Vector4FloatRep::referenceMember, Pointer)
    {
        const MemberVariable* v =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector4f* vp = reinterpret_cast<Vector4f*>(NODE_ARG(0, Pointer));
        NODE_RETURN((Pointer) & (*vp)[v->address()]);
    }

    NODE_IMPLEMENTATION(Vector4FloatRep::dereferenceMember, float)
    {
        const MemberVariable* v =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector4f* vp = reinterpret_cast<Vector4f*>(NODE_ARG(0, Pointer));
        NODE_RETURN((*vp)[v->address()]);
    }

    NODE_IMPLEMENTATION(Vector4FloatRep::extractMember, float)
    {
        const MemberVariable* var =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector4f v = NODE_ARG(0, Vector4f);
        NODE_RETURN(v[var->address()]);
    }

    //----------------------------------------------------------------------

    Vector3FloatRep* Vector3FloatRep::_rep = 0;

    Vector3FloatRep::Vector3FloatRep()
        : MachineRep("Vector3f", "3f", FloatRep::rep(), 3)
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(Vector3f);
        _structAlignment = __alignof__(Vector3f);
        _naturalAlignment = __alignof__(Vector3f);
        _constantFunc = Vector3FloatRep::constant;
        _referenceStackFunc = Vector3FloatRep::referenceStack;
        _dereferenceStackFunc = Vector3FloatRep::dereferenceStack;
        _referenceGlobalFunc = Vector3FloatRep::referenceGlobal;
        _dereferenceGlobalFunc = Vector3FloatRep::dereferenceGlobal;
        _referenceMemberFunc = Vector3FloatRep::referenceMember;
        _dereferenceMemberFunc = Vector3FloatRep::dereferenceMember;
        _extractMemberFunc = Vector3FloatRep::extractMember;
        _callMethodFunc = Vector3FloatRep::callMethod;
        _invokeInterfaceFunc = Vector3FloatRep::invokeInterface;
        _dereferenceClassMemberFunc = Vector3FloatRep::dereferenceClassMember;
        _frameBlockFunc = Vector3FloatRep::frameBlock;
        _simpleBlockFunc = Vector3FloatRep::simpleBlock;
        _patternBlockFunc = Vector3FloatRep::patternBlock;
        _functionActivationFunc = Vector3FloatRep::functionActivationFunc;
        _functionReturnFunc = Vector3FloatRep::functionReturnFunc;
        _dynamicActivationFunc = Vector3FloatRep::dynamicActivation;
        _functionTailFuseFunc = Vector3FloatRep::tailFuse;
        _variantConstructorFunc = Vector3FloatRep::variantConstructor;
        _unpackVariantFunc = Vector3FloatRep::unpackVariant;
    }

    Vector3FloatRep::~Vector3FloatRep() { _rep = 0; }

    ValuePointer Vector3FloatRep::valuePointer(Value& v) const
    {
        return &v._Vector3f;
    }

    const ValuePointer Vector3FloatRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._Vector3f);
    }

    Value Vector3FloatRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<Vector3f*>(p));
    }

    MACHINEFUNCS(Vector3FloatRep, Vector3f)
    FRAMEBLOCK(Vector3FloatRep, Vector3f)
    METHODFUNCS(Vector3FloatRep, Vector3f)
    VARIANTFUNCS(Vector3FloatRep, Vector3f)
    CLASSMEMBERFUNCS(Vector3FloatRep, Vector3f)
    ACTIVATIONFUNCS(Vector3FloatRep, Vector3f)
    DYNACTIVATIONFUNCS(Vector3FloatRep, Vector3f)

    NODE_IMPLEMENTATION(Vector3FloatRep::referenceMember, Pointer)
    {
        const MemberVariable* v =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector3f* vp = reinterpret_cast<Vector3f*>(NODE_ARG(0, Pointer));
        NODE_RETURN((Pointer) & (*vp)[v->address()]);
    }

    NODE_IMPLEMENTATION(Vector3FloatRep::dereferenceMember, float)
    {
        const MemberVariable* v =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector3f* vp = reinterpret_cast<Vector3f*>(NODE_ARG(0, Pointer));
        NODE_RETURN((*vp)[v->address()]);
    }

    NODE_IMPLEMENTATION(Vector3FloatRep::extractMember, float)
    {
        const MemberVariable* var =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector3f v = NODE_ARG(0, Vector3f);
        NODE_RETURN(v[var->address()]);
    }

    //----------------------------------------------------------------------

    Vector2FloatRep* Vector2FloatRep::_rep = 0;

    Vector2FloatRep::Vector2FloatRep()
        : MachineRep("Vector2f", "2f", FloatRep::rep(), 2)
    {
        assert(_rep == 0);
        _rep = this;
        _size = sizeof(Vector2f);
        _structAlignment = __alignof__(Vector2f);
        _naturalAlignment = __alignof__(Vector2f);
        _constantFunc = Vector2FloatRep::constant;
        _referenceStackFunc = Vector2FloatRep::referenceStack;
        _dereferenceStackFunc = Vector2FloatRep::dereferenceStack;
        _referenceGlobalFunc = Vector2FloatRep::referenceGlobal;
        _dereferenceGlobalFunc = Vector2FloatRep::dereferenceGlobal;
        _referenceMemberFunc = Vector2FloatRep::referenceMember;
        _dereferenceMemberFunc = Vector2FloatRep::dereferenceMember;
        _extractMemberFunc = Vector2FloatRep::extractMember;
        _callMethodFunc = Vector2FloatRep::callMethod;
        _invokeInterfaceFunc = Vector2FloatRep::invokeInterface;
        _dereferenceClassMemberFunc = Vector2FloatRep::dereferenceClassMember;
        _frameBlockFunc = Vector2FloatRep::frameBlock;
        _simpleBlockFunc = Vector2FloatRep::simpleBlock;
        _patternBlockFunc = Vector2FloatRep::patternBlock;
        _functionActivationFunc = Vector2FloatRep::functionActivationFunc;
        _functionReturnFunc = Vector2FloatRep::functionReturnFunc;
        _dynamicActivationFunc = Vector2FloatRep::dynamicActivation;
        _functionTailFuseFunc = Vector2FloatRep::tailFuse;
        _variantConstructorFunc = Vector2FloatRep::variantConstructor;
        _unpackVariantFunc = Vector2FloatRep::unpackVariant;
    }

    Vector2FloatRep::~Vector2FloatRep() { _rep = 0; }

    ValuePointer Vector2FloatRep::valuePointer(Value& v) const
    {
        return &v._Vector2f;
    }

    const ValuePointer Vector2FloatRep::valuePointer(const Value& v) const
    {
        return ValuePointer(&v._Vector2f);
    }

    Value Vector2FloatRep::value(ValuePointer p) const
    {
        return Value(*reinterpret_cast<Vector2f*>(p));
    }

    MACHINEFUNCS(Vector2FloatRep, Vector2f)
    FRAMEBLOCK(Vector2FloatRep, Vector2f)
    METHODFUNCS(Vector2FloatRep, Vector2f)
    VARIANTFUNCS(Vector2FloatRep, Vector2f)
    CLASSMEMBERFUNCS(Vector2FloatRep, Vector2f)
    ACTIVATIONFUNCS(Vector2FloatRep, Vector2f)
    DYNACTIVATIONFUNCS(Vector2FloatRep, Vector2f)

    NODE_IMPLEMENTATION(Vector2FloatRep::referenceMember, Pointer)
    {
        const MemberVariable* v =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector2f* vp = reinterpret_cast<Vector2f*>(NODE_ARG(0, Pointer));
        NODE_RETURN((Pointer) & (*vp)[v->address()]);
    }

    NODE_IMPLEMENTATION(Vector2FloatRep::dereferenceMember, float)
    {
        const MemberVariable* v =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector2f* vp = reinterpret_cast<Vector2f*>(NODE_ARG(0, Pointer));
        NODE_RETURN((*vp)[v->address()]);
    }

    NODE_IMPLEMENTATION(Vector2FloatRep::extractMember, float)
    {
        const MemberVariable* var =
            static_cast<const MemberVariable*>(NODE_THIS.symbol());
        Vector2f v = NODE_ARG(0, Vector2f);
        NODE_RETURN(v[var->address()]);
    }

    //----------------------------------------------------------------------

    VoidRep* VoidRep::_rep = 0;

    VoidRep::VoidRep()
        : MachineRep("void", "v")
    {
        assert(_rep == 0);
        _size = 0;
        _structAlignment = 0;
        _naturalAlignment = 0;
        _constantFunc = VoidRep::constant;
        _frameBlockFunc = VoidRep::frameBlock;
        _simpleBlockFunc = VoidRep::simpleBlock;
        _patternBlockFunc = VoidRep::patternBlock;
        _callMethodFunc = VoidRep::callMethod;
        _invokeInterfaceFunc = VoidRep::invokeInterface;
        _functionActivationFunc = VoidRep::functionActivationFunc;
        _functionReturnFunc = VoidRep::functionReturnFunc;
        _dynamicActivationFunc = VoidRep::dynamicActivation;
        _functionTailFuseFunc = VoidRep::tailFuse;
        _variantConstructorFunc = VoidRep::variantConstructor;
        _unpackVariantFunc = VoidRep::unpackVariant;
        _rep = this;
    }

    ValuePointer VoidRep::valuePointer(Value& v) const { return 0; }

    const ValuePointer VoidRep::valuePointer(const Value& v) const { return 0; }

    Value VoidRep::value(ValuePointer p) const { return Value(); }

    VoidRep::~VoidRep() { _rep = 0; }

    NODE_IMPLEMENTATION(VoidRep::frameBlock, void)
    {
        Thread::StackRecord record(NODE_THREAD);
        record.newStackFrame(NODE_DATA(int));
        int n = NODE_NUM_ARGS();
        for (int i = 0; i < n; i++)
            NODE_ANY_TYPE_ARG(i);
    }

    NODE_IMPLEMENTATION(VoidRep::simpleBlock, void)
    {
        int n = NODE_NUM_ARGS();
        for (int i = 0; i < n; i++)
            NODE_ANY_TYPE_ARG(i);
    }

    NODE_IMPLEMENTATION(VoidRep::patternBlock, void)
    {
        Thread::JumpRecord record(NODE_THREAD, JumpReturnCode::PatternFail);
        int rv = SETJMP(NODE_THREAD.jumpPoint());

        switch (rv)
        {
        case JumpReturnCode::PatternFail:
            NODE_THREAD.jumpPointRestore();
            throw PatternFailedException();
            break;

        case JumpReturnCode::NoJump:
        {
            int n = NODE_NUM_ARGS();
            for (int i = 0; i < n; i++)
                NODE_ANY_TYPE_ARG(i);
        }
        };
    }

    NODE_IMPLEMENTATION(VoidRep::functionActivationFunc, void)
    {
        const Function* f = static_cast<const Function*>(NODE_THIS.symbol());
        int n = NODE_NUM_ARGS();
        int s = f->stackSize();
        Thread::StackRecord record(NODE_THREAD);
        record.beginActivation(s);
        Value v;

        for (int i = 0; i < s; i++)
        {
            if (i < n)
            {
                v = NODE_EVAL_VALUE(NODE_THIS.argNode(i), NODE_THREAD);
            }
            else
            {
                zero(v);
            }

            record.setParameter(i, v);
        }

        record.endParameters();

        const Node* body = f->body();
        if (!body)
            throw UnimplementedMethodException(NODE_THREAD);

        if (NodeFunc func = body->func())
        {
            NODE_THREAD.jumpPointBegin(JumpReturnCode::Return);
            int rv = SETJMP(NODE_THREAD.jumpPoint());

            switch (rv)
            {
            case JumpReturnCode::TailFuse:
            {
                //
                //  This is the tail call optimization. So why in the
                //  world is it just recursively calling this function?
                //  Modern gcc includes a recursive tail call optimization
                //  itself so let's just use it! In our case, the tail
                //  call can be *any* function not just a self-recursive
                //  one so it should work for mutual recursion as well.
                //  Pretty sweet deal!
                //

                return VoidRep::functionActivationFunc(
                    *NODE_THREAD.continuation(), NODE_THREAD);
            }
            case JumpReturnCode::NoJump:
                (func._voidFunc)(*body, NODE_THREAD);
                break;
            default:
                NODE_THREAD.jumpPointRestore();
                break;
            }

            NODE_THREAD.jumpPointEnd();
        }
        else
        {
            throw NilNodeFuncException(NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(VoidRep::tailFuse, void)
    {
        NODE_THREAD.setContinuation(NODE_THIS.argNode(0));
        NODE_THREAD.jump(JumpReturnCode::TailFuse);
    }

    NODE_IMPLEMENTATION(VoidRep::functionReturnFunc, void)
    {
        NODE_THREAD.jump(JumpReturnCode::Return);
    }

    NODE_IMPLEMENTATION(VoidRep::dynamicActivation, void)
    {
        if (FunctionObject* fobj = NODE_ARG_OBJECT(0, FunctionObject))
        {
            const Function* f = fobj->function();

            Node node(NODE_THIS.argv() + 1, f);
            NodeFunc nf = f->func(&node);

            try
            {
                (*nf._voidFunc)(node, NODE_THREAD);
            }
            catch (...)
            {
                node.releaseArgv();
                throw;
            }

            node.releaseArgv();
        }
        else
        {
            throw NilArgumentException(NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(VoidRep::invokeInterface, void)
    {
        const Function* f = static_cast<const Function*>(NODE_THIS.symbol());
        const Interface* i = static_cast<const Interface*>(f->scope());
        ClassInstance* o =
            reinterpret_cast<ClassInstance*>(NODE_ARG(0, Pointer));

        if (const InterfaceImp* imp = o->classType()->implementation(i))
        {
            size_t offset = f->interfaceIndex();
            NodeFunc func = imp->func(offset);

            size_t nargs = NODE_THIS.numArgs();
            LOCAL_ARRAY(argv, const Node*, nargs + 1);
            DataNode dn(0, PointerRep::rep()->constantFunc(), o->type());
            dn._data._Pointer = o;
            argv[0] = &dn;
            argv[nargs] = 0;
            for (size_t i = 1; i < nargs; i++)
                argv[i] = NODE_THIS.argNode(i);
            Node n((Node**)argv, f);

            (*func._voidFunc)(n, NODE_THREAD);
            n.releaseArgv();
        }
        else
        {
            throw UnresolvedFunctionException(NODE_THREAD);
        }
    }

    NODE_IMPLEMENTATION(VoidRep::constant, void) { return; }

    NODE_IMPLEMENTATION(VoidRep::variantConstructor, Pointer)
    {
        const VariantTagType* c =
            static_cast<const VariantTagType*>(NODE_THIS.symbol()->scope());
        VariantInstance* i = VariantInstance::allocate(c);
        NODE_RETURN(i);
    }

    NODE_IMPLEMENTATION(VoidRep::callMethod, void)
    {
        const MemberFunction* f =
            static_cast<const MemberFunction*>(NODE_THIS.symbol());
        ClassInstance* i =
            reinterpret_cast<ClassInstance*>(NODE_ARG(0, Pointer));
        if (!i)
            throw NilArgumentException(NODE_THREAD);
        const MemberFunction* F = i->classType()->dynamicLookup(f);

        // if (!F)
        // {
        //     F = i->classType()->dynamicLookup(f);
        // }

        assert(F);

        size_t nargs = NODE_THIS.numArgs();
        LOCAL_ARRAY(argv, const Node*, nargs + 1);
        DataNode dn(0, PointerRep::rep()->constantFunc(), i->type());
        dn._data._Pointer = i;
        argv[0] = &dn;
        argv[nargs] = 0;
        for (size_t i = 1; i < nargs; i++)
            argv[i] = NODE_THIS.argNode(i);
        Node n((Node**)argv, F);

        try
        {
            (*F->func()._voidFunc)(n, NODE_THREAD);
        }
        catch (...)
        {
            n.releaseArgv();
            throw;
        }

        n.releaseArgv();
    }

    NODE_IMPLEMENTATION(VoidRep::unpackVariant, void)
    {
        // Don't do anything
        VariantInstance* i = NODE_ARG_OBJECT(0, VariantInstance);
    }

} // namespace Mu

#ifdef _MSC_VER
#undef __alignof__
#endif
