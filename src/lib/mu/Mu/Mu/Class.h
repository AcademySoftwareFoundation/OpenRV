#ifndef __Mu__Class__h__
#define __Mu__Class__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Type.h>

namespace Mu
{
    class MemberFunction;
    class MemberVariable;
    class InternalTypeMemberVariable;
    class Interface;
    class InterfaceImp;
    class Function;
    class Signature;
    class ClassInstance;

    //
    //  class Class
    //
    //  "Class" represents a method for generating ClassInstance
    //  Objects. These are objects which have methods and data members.
    //

    class Class : public Type
    {
    public:
        typedef STLVector<const MemberFunction*>::Type MemberFunctionVector;
        typedef STLVector<MemberVariable*>::Type MemberVariableVector;
        typedef STLVector<const Class*>::Type ClassVector;
        typedef STLVector<InterfaceImp*>::Type InterfaceImps;
        typedef STLVector<size_t>::Type ClassOffsets;
        typedef STLVector<InternalTypeMemberVariable*>::Type
            TypeMemberVariableVector;

        Class(Context* context, const char* name, Class* superClass = 0);
        Class(Context* context, const char* name,
              const ClassVector& superClasses);
        virtual ~Class();

        //
        //  Used by Archive
        //

        void addSuperClass(const Class*);

        //
        //	Symbol + Type API
        //

        virtual Value nodeEval(const Node*, Thread&) const;
        virtual void nodeEval(void*, const Node*, Thread&) const;

        virtual Object* newObject() const;
        virtual size_t objectSize() const;
        virtual void deleteObject(Object*) const;

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        virtual void symbolDependancies(ConstSymbolVector&) const;

        virtual const Type* fieldType(size_t) const;
        virtual ValuePointer fieldPointer(Object*, size_t) const;
        virtual const ValuePointer fieldPointer(const Object*, size_t) const;
        virtual void constructInstance(Pointer) const;
        virtual void copyInstance(Pointer, Pointer) const;

        //
        //	As symbols are added to the class, it may add virtual function
        //	table entries. Note that this may have an effect on derived
        //	classes, so the base class must keep track of all derived
        //	classes.
        //

        virtual void addSymbol(Symbol*);

        //
        //	Overriden from Symbol, this function looks up the class tree to
        //	find the specified name. Note: this function does not handle
        //	overriden names very well.
        //

        virtual const Symbol* findSymbol(Name) const;
        virtual void findSymbols(QualifiedName, SymbolVector&);
        virtual void findSymbols(QualifiedName, ConstSymbolVector&) const;

        //
        //  Find the function of the given name and signature in the class
        //  or the super class. Returns 0 on failure.
        //

        const Function* findFunction(Name, const Signature*) const;

        //
        //	Returns the class passed into the constructor
        //

        const ClassVector& superClasses() const { return _superClasses; }

        const ClassVector& derivedClasses() const { return _children; }

        const ClassOffsets& superOffsets() const { return _superOffsets; }

        const TypeMemberVariableVector& typeMembers() const
        {
            return _typeMembers;
        }

        //
        //	Determine if this class is derived class (ancestor) of another
        //

        bool isA(const Class*) const;

        //
        //  Can a ClassInstance of this class be substituted for the given
        //  class. In single inheritance situations this will always be
        //  true and isA() should always be true for the input. In
        //  multiple inheritance cases a dynamic cast maybe needed to
        //  adjust the ClassInstance
        //

        bool substitutable(const Class*) const;

        //
        //  May return a ClassInstance pointer inside the passed in object
        //

        ClassInstance* dynamicCast(ClassInstance*, const Class*,
                                   bool upcastOK) const;

        //
        //	Returns true if the types match.
        //

        virtual MatchResult match(const Type*, Bindings&) const;

        //
        //	Freeze the class -- the virtual function table size is
        //	computed and filled. No more members may be added to the class
        //	after freeze is called.
        //

        virtual void freeze();

        bool isFrozen() const { return _frozen; }

        //
        //	Virtual function table for this class
        //

        const MemberVariableVector& memberVariables() const
        {
            return _memberVariables;
        }

        void allMemberVariables(MemberVariableVector&) const;

        bool isInBaseClass(const MemberVariable*) const;

        //
        //  Dynamic method lookup. Takes any MemberFunction
        //  (i.e. derived/base,etc) and finds the one that maps to this
        //  class. This is used at runtime to invoke the correct method.
        //

        const MemberFunction* dynamicLookup(const MemberFunction*) const;

        //
        //	Instance information. This information is only valid after the
        //	class has been frozen.
        //

        size_t instanceSize() const { return _instanceSize; }

        //
        //  May be done at runtime: determines dynamically whether a class
        //  conforms to an interface. If so, an InterfaceInstance will be
        //  returned. (These are cached).
        //

        const InterfaceImp* implementation(const Interface*) const;

        const InterfaceImps& knownInterfaces() const { return _interfaces; }

        //
        //  Find all functions that override the given member function.
        //

        void findOverridingFunctions(const MemberFunction*,
                                     MemberFunctionVector&) const;

    protected:
        //
        //  Classes with nebulous ancestry need more logic for isA() to
        //  return a correct answer
        //

        virtual bool nebulousIsA(const Class*) const;

    private:
        ClassVector _superClasses;
        ClassOffsets _superOffsets;
        ClassVector _castClasses;
        ClassOffsets _castOffsets;
        TypeMemberVariableVector _typeMembers;
        mutable ClassVector _children;
        MemberVariableVector _memberVariables;
        mutable InterfaceImps _interfaces;

    protected:
        size_t _instanceSize;

    private:
        mutable bool _frozen : 1;

    protected:
        bool _nebulousAncestry : 1;
    };

} // namespace Mu

#endif // __Mu__Class__h__
