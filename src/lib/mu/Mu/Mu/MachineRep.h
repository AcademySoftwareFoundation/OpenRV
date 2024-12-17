#ifndef __Mu__MachineRep__h__
#define __Mu__MachineRep__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
#include <Mu/Symbol.h>
#include <Mu/NodeFunc.h>
#include <Mu/Node.h>
#include <string>

namespace Mu
{

    //
    //  class GenericMachine
    //
    //  Typically you instantiate a specific Machine/ABI class --
    //  e.g. DarwinPPC or LinuxIA32. However, in the absense of these
    //  machine descriptions, you can use GenericMachine. It will just
    //  instantiate all the base classes -- so only the interpreter will
    //  work if you use GenericMachine.
    //

    class GenericMachine
    {
    public:
        MU_GC_NEW_DELETE
        GenericMachine();
        ~GenericMachine();
    };

    //----------------------------------------------------------------------

    //
    //  class MachineRep
    //
    //  MachineRep describes a native machine type (or perhaps a native
    //  host language type -- e.g. C++ types).
    //

    class MachineRep
    {
    public:
        MU_GC_NEW_DELETE
        typedef STLVector<MachineRep*>::Type MachineReps;

        //
        //	A truely primitive machine type
        //

        MachineRep(const char* name, const char* fmtName);

        //
        //	A composed (vector) machine type
        //

        MachineRep(const char* name, const char* fmtName,
                   MachineRep* elementRep, size_t width);

        virtual ~MachineRep();

        //
        //	List of all MachineReps
        //

        static const MachineReps& allReps() { return _allReps; }

        //
        //	This is for cleaning up on exit only
        //

        static void deleteAll();

        //
        //	Name passed into constructor. MachineRep interns its names in
        // the 	Symbol class's namePool().
        //

        String name() const { return _name; }

        String fmtName() const { return _fmtName; }

        //
        //	Element rep if applicable
        //

        const MachineRep* elementRep() const { return _elementRep; }

        //
        //	Width if applicable
        //

        size_t width() const { return _width; }

        //
        //	Information API
        //

        NodeFunc constantFunc() const { return _constantFunc; }

        NodeFunc referenceStackFunc() const { return _referenceStackFunc; }

        NodeFunc dereferenceStackFunc() const { return _dereferenceStackFunc; }

        NodeFunc referenceGlobalFunc() const { return _referenceGlobalFunc; }

        NodeFunc dereferenceGlobalFunc() const
        {
            return _dereferenceGlobalFunc;
        }

        NodeFunc referenceMemberFunc() const { return _referenceMemberFunc; }

        NodeFunc dereferenceMemberFunc() const
        {
            return _dereferenceMemberFunc;
        }

        NodeFunc extractMemberFunc() const { return _extractMemberFunc; }

        NodeFunc dereferenceClassMemberFunc() const
        {
            return _dereferenceClassMemberFunc;
        }

        NodeFunc referenceClassMemberFunc() const
        {
            return _referenceClassMemberFunc;
        }

        NodeFunc callMethodFunc() const { return _callMethodFunc; }

        NodeFunc invokeInterfaceFunc() const { return _invokeInterfaceFunc; }

        NodeFunc frameBlockFunc() const { return _frameBlockFunc; }

        NodeFunc simpleBlockFunc() const { return _simpleBlockFunc; }

        NodeFunc patternBlockFunc() const { return _patternBlockFunc; }

        NodeFunc functionActivationFunc() const
        {
            return _functionActivationFunc;
        }

        NodeFunc functionReturnFunc() const { return _functionReturnFunc; }

        NodeFunc dynamicActivationFunc() const
        {
            return _dynamicActivationFunc;
        }

        NodeFunc functionTailFuseFunc() const { return _functionTailFuseFunc; }

        NodeFunc variantConstructorFunc() const
        {
            return _variantConstructorFunc;
        }

        NodeFunc unpackVariant() const { return _unpackVariantFunc; }

        //
        //	structAlignment -- this should be the "packed" or structure
        //	alignment for the machine type. This would correspond to what
        // the 	ABI uses for alignment. If you don't care about C struct
        //	compatibility, you can use the default natural alignment which
        // is 	the size of the type.
        //
        //	naturalAlignment -- this is almost always the same as the size
        // of 	the type.
        //
        //	size -- basically sizeof(type)
        //

        size_t structAlignment() const { return _structAlignment; }

        size_t naturalAlignment() const { return _naturalAlignment; }

        size_t size() const { return _size; }

        //
        //  ValuePointer from Value
        //

        virtual ValuePointer valuePointer(Value&) const = 0;
        virtual const ValuePointer valuePointer(const Value&) const = 0;
        virtual Value value(ValuePointer) const = 0;

        //
        //	Nodes common to many types
        //

        static NODE_DECLARATION(referenceClassMember, Pointer);

    protected:
        MachineRep* _elementRep;
        size_t _width;
        String _fmtName;
        size_t _structAlignment;
        size_t _naturalAlignment;
        size_t _size;
        NodeFunc _constantFunc;
        NodeFunc _referenceStackFunc;
        NodeFunc _dereferenceStackFunc;
        NodeFunc _referenceGlobalFunc;
        NodeFunc _dereferenceGlobalFunc;
        NodeFunc _referenceMemberFunc;
        NodeFunc _dereferenceMemberFunc;
        NodeFunc _extractMemberFunc;
        NodeFunc _dereferenceClassMemberFunc;
        NodeFunc _referenceClassMemberFunc;
        NodeFunc _callMethodFunc;
        NodeFunc _invokeInterfaceFunc;
        NodeFunc _frameBlockFunc;
        NodeFunc _simpleBlockFunc;
        NodeFunc _patternBlockFunc;
        NodeFunc _functionActivationFunc;
        NodeFunc _functionReturnFunc;
        NodeFunc _dynamicActivationFunc;
        NodeFunc _functionTailFuseFunc;
        NodeFunc _variantConstructorFunc;
        NodeFunc _unpackVariantFunc;

    private:
        String _name;
        static MachineReps _allReps;
    };

    //----------------------------------------------------------------------

    class FloatRep : public MachineRep
    {
    public:
        FloatRep();
        virtual ~FloatRep();

        static FloatRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, float);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, float);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, float);
        static NODE_DECLARATION(callMethod, float);
        static NODE_DECLARATION(invokeInterface, float);
        static NODE_DECLARATION(dereferenceClassMember, float);
        static NODE_DECLARATION(frameBlock, float);
        static NODE_DECLARATION(simpleBlock, float);
        static NODE_DECLARATION(patternBlock, float);
        static NODE_DECLARATION(functionActivationFunc, float);
        static NODE_DECLARATION(functionReturnFunc, float);
        static NODE_DECLARATION(dynamicActivation, float);
        static NODE_DECLARATION(tailFuse, float);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, float);
        static FloatRep* _rep;
    };

    //----------------------------------------------------------------------

    class DoubleRep : public MachineRep
    {
    public:
        DoubleRep();
        virtual ~DoubleRep();

        static DoubleRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, double);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, double);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, double);
        static NODE_DECLARATION(callMethod, double);
        static NODE_DECLARATION(invokeInterface, double);
        static NODE_DECLARATION(dereferenceClassMember, double);
        static NODE_DECLARATION(frameBlock, double);
        static NODE_DECLARATION(simpleBlock, double);
        static NODE_DECLARATION(patternBlock, double);
        static NODE_DECLARATION(functionActivationFunc, double);
        static NODE_DECLARATION(functionReturnFunc, double);
        static NODE_DECLARATION(dynamicActivation, double);
        static NODE_DECLARATION(tailFuse, double);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, double);
        static DoubleRep* _rep;
    };

    //----------------------------------------------------------------------

    class IntRep : public MachineRep
    {
    public:
        IntRep();
        virtual ~IntRep();

        static IntRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, int);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, int);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, int);
        static NODE_DECLARATION(callMethod, int);
        static NODE_DECLARATION(invokeInterface, int);
        static NODE_DECLARATION(dereferenceClassMember, int);
        static NODE_DECLARATION(frameBlock, int);
        static NODE_DECLARATION(simpleBlock, int);
        static NODE_DECLARATION(patternBlock, int);
        static NODE_DECLARATION(functionActivationFunc, int);
        static NODE_DECLARATION(functionReturnFunc, int);
        static NODE_DECLARATION(dynamicActivation, int);
        static NODE_DECLARATION(tailFuse, int);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, int);
        static IntRep* _rep;
    };

    //----------------------------------------------------------------------

    class Int64Rep : public MachineRep
    {
    public:
        Int64Rep();
        virtual ~Int64Rep();

        static Int64Rep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, int64);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, int64);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, int64);
        static NODE_DECLARATION(callMethod, int64);
        static NODE_DECLARATION(invokeInterface, int64);
        static NODE_DECLARATION(dereferenceClassMember, int64);
        static NODE_DECLARATION(frameBlock, int64);
        static NODE_DECLARATION(simpleBlock, int64);
        static NODE_DECLARATION(patternBlock, int64);
        static NODE_DECLARATION(functionActivationFunc, int64);
        static NODE_DECLARATION(functionReturnFunc, int64);
        static NODE_DECLARATION(dynamicActivation, int64);
        static NODE_DECLARATION(tailFuse, int64);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, int64);
        static Int64Rep* _rep;
    };

    //----------------------------------------------------------------------

    class ShortRep : public MachineRep
    {
    public:
        ShortRep();
        virtual ~ShortRep();

        static ShortRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, short);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, short);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, short);
        static NODE_DECLARATION(callMethod, short);
        static NODE_DECLARATION(invokeInterface, short);
        static NODE_DECLARATION(dereferenceClassMember, short);
        static NODE_DECLARATION(frameBlock, short);
        static NODE_DECLARATION(simpleBlock, short);
        static NODE_DECLARATION(patternBlock, short);
        static NODE_DECLARATION(functionActivationFunc, short);
        static NODE_DECLARATION(functionReturnFunc, short);
        static NODE_DECLARATION(dynamicActivation, short);
        static NODE_DECLARATION(tailFuse, short);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, short);
        static ShortRep* _rep;
    };

    //----------------------------------------------------------------------

    class CharRep : public MachineRep
    {
    public:
        CharRep();
        virtual ~CharRep();

        static CharRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, char);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, char);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, char);
        static NODE_DECLARATION(callMethod, char);
        static NODE_DECLARATION(invokeInterface, char);
        static NODE_DECLARATION(dereferenceClassMember, char);
        static NODE_DECLARATION(frameBlock, char);
        static NODE_DECLARATION(simpleBlock, char);
        static NODE_DECLARATION(patternBlock, char);
        static NODE_DECLARATION(functionActivationFunc, char);
        static NODE_DECLARATION(functionReturnFunc, char);
        static NODE_DECLARATION(dynamicActivation, char);
        static NODE_DECLARATION(tailFuse, char);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, char);
        static CharRep* _rep;
    };

    //----------------------------------------------------------------------

    class BoolRep : public MachineRep
    {
    public:
        BoolRep();
        virtual ~BoolRep();

        static BoolRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, bool);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, bool);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, bool);
        static NODE_DECLARATION(callMethod, bool);
        static NODE_DECLARATION(invokeInterface, bool);
        static NODE_DECLARATION(dereferenceClassMember, bool);
        static NODE_DECLARATION(frameBlock, bool);
        static NODE_DECLARATION(simpleBlock, bool);
        static NODE_DECLARATION(patternBlock, bool);
        static NODE_DECLARATION(functionActivationFunc, bool);
        static NODE_DECLARATION(functionReturnFunc, bool);
        static NODE_DECLARATION(dynamicActivation, bool);
        static NODE_DECLARATION(tailFuse, bool);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, bool);
        static BoolRep* _rep;
    };

    //----------------------------------------------------------------------

    class PointerRep : public MachineRep
    {
    public:
        PointerRep();
        virtual ~PointerRep();

        static PointerRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, Pointer);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, Pointer);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, Pointer);
        static NODE_DECLARATION(callMethod, Pointer);
        static NODE_DECLARATION(invokeInterface, Pointer);
        static NODE_DECLARATION(dereferenceClassMember, Pointer);
        static NODE_DECLARATION(frameBlock, Pointer);
        static NODE_DECLARATION(simpleBlock, Pointer);
        static NODE_DECLARATION(patternBlock, Pointer);
        static NODE_DECLARATION(functionActivationFunc, Pointer);
        static NODE_DECLARATION(functionReturnFunc, Pointer);
        static NODE_DECLARATION(dynamicActivation, Pointer);
        static NODE_DECLARATION(tailFuse, Pointer);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, Pointer);
        static PointerRep* _rep;
    };

    //----------------------------------------------------------------------

    class Vector4FloatRep : public MachineRep
    {
    public:
        Vector4FloatRep();
        virtual ~Vector4FloatRep();

        static Vector4FloatRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, Vector4f);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, Vector4f);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, Vector4f);
        static NODE_DECLARATION(referenceMember, Pointer);
        static NODE_DECLARATION(dereferenceMember, float);
        static NODE_DECLARATION(extractMember, float);
        static NODE_DECLARATION(callMethod, Vector4f);
        static NODE_DECLARATION(invokeInterface, Vector4f);
        static NODE_DECLARATION(dereferenceClassMember, Vector4f);
        static NODE_DECLARATION(frameBlock, Vector4f);
        static NODE_DECLARATION(simpleBlock, Vector4f);
        static NODE_DECLARATION(patternBlock, Vector4f);
        static NODE_DECLARATION(functionActivationFunc, Vector4f);
        static NODE_DECLARATION(functionReturnFunc, Vector4f);
        static NODE_DECLARATION(dynamicActivation, Vector4f);
        static NODE_DECLARATION(tailFuse, Vector4f);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, Vector4f);
        static Vector4FloatRep* _rep;
    };

    //----------------------------------------------------------------------

    class Vector3FloatRep : public MachineRep
    {
    public:
        Vector3FloatRep();
        virtual ~Vector3FloatRep();

        static Vector3FloatRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, Vector3f);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, Vector3f);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, Vector3f);
        static NODE_DECLARATION(referenceMember, Pointer);
        static NODE_DECLARATION(dereferenceMember, float);
        static NODE_DECLARATION(extractMember, float);
        static NODE_DECLARATION(callMethod, Vector3f);
        static NODE_DECLARATION(invokeInterface, Vector3f);
        static NODE_DECLARATION(dereferenceClassMember, Vector3f);
        static NODE_DECLARATION(frameBlock, Vector3f);
        static NODE_DECLARATION(simpleBlock, Vector3f);
        static NODE_DECLARATION(patternBlock, Vector3f);
        static NODE_DECLARATION(functionActivationFunc, Vector3f);
        static NODE_DECLARATION(functionReturnFunc, Vector3f);
        static NODE_DECLARATION(dynamicActivation, Vector3f);
        static NODE_DECLARATION(tailFuse, Vector3f);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, Vector3f);
        static Vector3FloatRep* _rep;
    };

    //----------------------------------------------------------------------

    class Vector2FloatRep : public MachineRep
    {
    public:
        Vector2FloatRep();
        virtual ~Vector2FloatRep();

        static Vector2FloatRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;

    private:
        static NODE_DECLARATION(constant, Vector2f);
        static NODE_DECLARATION(referenceStack, Pointer);
        static NODE_DECLARATION(dereferenceStack, Vector2f);
        static NODE_DECLARATION(referenceGlobal, Pointer);
        static NODE_DECLARATION(dereferenceGlobal, Vector2f);
        static NODE_DECLARATION(referenceMember, Pointer);
        static NODE_DECLARATION(dereferenceMember, float);
        static NODE_DECLARATION(extractMember, float);
        static NODE_DECLARATION(callMethod, Vector2f);
        static NODE_DECLARATION(invokeInterface, Vector2f);
        static NODE_DECLARATION(dereferenceClassMember, Vector2f);
        static NODE_DECLARATION(frameBlock, Vector2f);
        static NODE_DECLARATION(simpleBlock, Vector2f);
        static NODE_DECLARATION(patternBlock, Vector2f);
        static NODE_DECLARATION(functionActivationFunc, Vector2f);
        static NODE_DECLARATION(functionReturnFunc, Vector2f);
        static NODE_DECLARATION(dynamicActivation, Vector2f);
        static NODE_DECLARATION(tailFuse, Vector2f);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, Vector2f);
        static Vector2FloatRep* _rep;
    };

    //----------------------------------------------------------------------

    class VoidRep : public MachineRep
    {
    public:
        VoidRep();
        virtual ~VoidRep();

        static VoidRep* rep() { return _rep; }

        virtual ValuePointer valuePointer(Value&) const;
        virtual const ValuePointer valuePointer(const Value&) const;
        virtual Value value(ValuePointer) const;
        static NODE_DECLARATION(constant, void);
        static NODE_DECLARATION(callMethod, void);
        static NODE_DECLARATION(invokeInterface, void);
        static NODE_DECLARATION(functionActivationFunc, void);
        static NODE_DECLARATION(functionReturnFunc, void);
        static NODE_DECLARATION(dynamicActivation, void);
        static NODE_DECLARATION(frameBlock, void);
        static NODE_DECLARATION(simpleBlock, void);
        static NODE_DECLARATION(patternBlock, void);
        static NODE_DECLARATION(tailFuse, void);
        static NODE_DECLARATION(variantConstructor, Pointer);
        static NODE_DECLARATION(unpackVariant, void);

    private:
        static VoidRep* _rep;
    };

} // namespace Mu

#endif // __Mu__MachineRep__h__
