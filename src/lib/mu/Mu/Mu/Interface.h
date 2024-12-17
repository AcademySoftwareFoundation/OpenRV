#ifndef __Mu__Interface__h__
#define __Mu__Interface__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Type.h>

namespace Mu
{
    class Class;
    class InterfaceImp;

    //
    //  class Interface
    //
    //  Similar to an Obj-C protocol.
    //

    class Interface : public Type
    {
    public:
        typedef STLVector<Interface*>::Type Interfaces;
        typedef STLVector<Class*>::Type Classes;

        Interface(Context* context, const char* name);
        Interface(Context* context, const char* name, const Interfaces&);
        virtual ~Interface();

        void freeze();

        bool isFrozen() const { return _frozen; }

        //
        //  Symbol/Type API
        //

        virtual Value nodeEval(const Node*, Thread&) const;
        virtual void nodeEval(void*, const Node*, Thread&) const;

        virtual Object* newObject() const;
        virtual void deleteObject(Object*) const;
        virtual void output(std::ostream&) const;
        virtual void outputNode(std::ostream&, const Node*) const;
        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;

        //
        //	As symbols are added to the interface, it will add virtual
        //	function table entries. Each interface has its own vtable per
        //	class that is case to it.
        //

        virtual void addSymbol(Symbol*);

        //
        //	Overriden from Symbol, this function looks up the class tree to
        //	find the specified name. Note: this function does not handle
        //	overriden names very well.
        //

        virtual const Symbol* findSymbol(Name) const;

        size_t numFunctions() const { return _numFunctions; }

        //
        //	Returns true if the types match.
        //

        virtual bool match(const Type*) const;

    private:
        //
        //  Construct and instance for a class
        //

        InterfaceImp* construct(const Class*) const;

    private:
        Classes _implementors;
        Interfaces _superInterfaces;
        size_t _numFunctions;
        mutable bool _frozen : 1;
        friend class Class;
    };

} // namespace Mu

#endif // __Mu__Interface__h__
