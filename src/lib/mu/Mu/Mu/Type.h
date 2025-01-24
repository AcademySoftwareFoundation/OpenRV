#ifndef __Mu__Type__h__
#define __Mu__Type__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/config.h>
#include <Mu/Symbol.h>
#include <Mu/Node.h>
#include <Mu/Object.h>
#include <iosfwd>
#include <vector>
#include <map>

namespace Mu
{

    class ReferenceType;
    class MachineRep;
    class Object;
    class TypeVariable;
    class Process;

    namespace Archive
    {
        class Reader;
        class Writer;
    } // namespace Archive

    //
    //  class Type
    //
    //  Type determines both a type in the language as well as memory
    //  management policy for its objects.
    //

    class Type : public Symbol
    {
    public:
        //
        //  Types
        //

        typedef STLMap<const TypeVariable*, const Type*>::Type Bindings;
        typedef STLSet<const Object*>::Type ObjectSet;

        struct ValueOutputState
        {
            ValueOutputState(std::ostream& ostr, bool doFull = false)
                : fullOutput(doFull)
                , startPos(ostr.tellp())
            {
            }

            bool fullOutput; // full output, do not truncate long values
            ObjectSet traversedObjects;
            std::streampos startPos;
        };

        //
        //	The Value::Rep indicates how the type will store a value.
        //
        //	You will need to supply a MachineRep* pointer. Examples are:
        //	FloatRep::rep(), IntRep::rep(), etc. These will hold the basic
        //	MachineRep class pointers.
        //

        Type(Context* context, const char* name, const MachineRep*);
        virtual ~Type();

        //
        //	The class indicating the machine representation of the
        //	type. The value passed into the constructor is returned.
        //

        const MachineRep* machineRep() const { return _machineRep; }

        //
        //	A TypePattern is a subclass of Type that is a placeholder for
        //	matching other types. A good example is the "..." in a C var
        //	args prototype. Mu makes these almost types (although you
        //	can't make one only match to it.)
        //
        //  A TypeVariable is a subclass of Type that is used in
        //  polymorphic function and structure definitions.
        //
        //	A ReferenceType is a Type that is derived (like pointer to
        //	type) and is used to represent a LHS (or lvalue).
        //
        //	A PrimitiveType is a type the value of which can be held in
        //	the Value union directly.
        //
        //	A self managed type is one that at least partially manages its
        //	memory on the fly. So it does not completely rely on garbage
        //	collection to free memory. These types may require special
        //	nodes inserted before memory retaining functions to function
        //	efficiently. The type should implement the __retain function
        //	in its scope. This function takes an object of this type and
        //	returns (probably) the same object. This function should be
        //	used to do any special memory management.
        //
        //	An aggregate type is composed of fields. An array is an
        //	example of this, so is a struct. All Mu classes are aggregate
        //	types. The public type fields of these types can be
        //	initialized directly.
        //
        //  A collection is a special kind of aggregate type in which all the
        //  possible fields are the same type (fieldType(N) always returns the
        //  same value). An array is a collection. A struct with all the same
        //  type fields is also a collection. A collection is not necessarily
        //  ordered.
        //
        //  A sequence is a special type of collection that is ordered -- an
        //  array and a list are sequences.
        //
        //  A fixed size type has a fixed number of type fields. A dynamic
        //  array or a list is *not* a fixed size type.
        //

        bool isTypePattern() const { return _isTypePattern; }

        bool isTypeVariable() const { return _isTypeVariable; }

        bool isUnresolvedType() const { return _isUnresolvedType; }

        bool isReferenceType() const { return _isRefType; }

        bool isPrimitiveType() const { return _isPrimitive; }

        bool isSelfManaged() const { return _selfManaged; }

        bool isAggregate() const { return _isAggregate; }

        bool isSequence() const { return _isSequence; }

        bool isCollection() const { return _isCollection; }

        bool isSerializable() const { return _isSerializable; }

        bool isFixedSize() const { return _isFixedSize; }

        bool isGCAtomic() const { return _isGCAtomic; }

        bool isMutable() const { return _isMutable; }

        //
        //	Any type can have a reference type associated with it
        //	(including reference types themselves).
        //

        const ReferenceType* referenceType() const { return _referenceType; }

        //
        //	Retuns a new Object (memory rep) of an instance of this
        //	type. There is typically a new subclass of Object for each new
        //	Type.
        //

        virtual Object* newObject() const = 0;

        //
        //  Returns the size of an instance object (in bytes) including
        //  the Object base size. In the case of primitive types, this
        //  will return the size of the primitive. Default == 0
        //

        virtual size_t objectSize() const;

        //
        //	Indicates the return type of a node.
        //

        virtual const Type* nodeReturnType(const Node*) const;

        //
        //	Evaluates a node and returns a Value.
        //

        virtual Value nodeEval(const Node*, Thread& t) const = 0;

        //
        //	Evaluates a node and places the value at the memory location.
        // Make 	sure the alignment is correct before calling this!
        //

        virtual void nodeEval(void*, const Node*, Thread& t) const = 0;

        //
        //	Deletes (for real) an Object of this type. This can't be
        //	implemented in the object because it (the Object) can't have
        //	virtual functions. Therefore the Type class must do all the
        //	work.
        //

        virtual void deleteObject(Object*) const;

        //
        //	Returns Match if the types match. Intended to be overriden by
        //	a sub-class of Type. For example, a ReferenceType might match
        //	a non-ReferenceType.
        //
        //  When resolving type variable bindings, this function will be
        //  called with the current binding set. If there is a conflict,
        //  then Conflict should be returned.
        //

        enum MatchResult
        {
            NoMatch,
            Match,
            Conflict
        };

        virtual bool match(const Type*) const;
        virtual MatchResult match(const Type*, Bindings&) const;

        //
        //	Convenience Functions.  If your type uses a DataNode to hold
        //	its constant, you will not need to override outputNode() if
        //	outputValue() is implemented.
        //

        virtual void output(std::ostream&) const;
        virtual void outputNode(std::ostream&, const Node*) const;
        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        void outputValue(std::ostream&, const ValuePointer,
                         bool full = false) const;

        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;

        //
        //	Aggregate Type API, by default, these return 0.
        //
        //  fieldType() should return the type of the Nth field or 0 if
        //  the field is known not to exist
        //
        //  fieldPointer() should return a pointer to the Nth field in the
        //  specified object or 0 if the field does not exist. Note that
        //  the fieldType may report that a field has a type, but the type
        //  may not exist in the given object.
        //

        virtual const Type* fieldType(size_t) const;
        virtual ValuePointer fieldPointer(Object*, size_t) const;
        virtual const ValuePointer fieldPointer(const Object*, size_t) const;

        //
        //  Archiving.
        //  serialize converts an object into a stream using the archive
        //  deserialize reads an object froma stream using the archive
        //  reconstitute reconnects object references after a read.
        //
        //  These functions operate entirely using the field* functions
        //  above. If you have special opaque data that is not reported
        //  with the field functions, then you need to implement these
        //  functions yourself.
        //

        virtual void serialize(std::ostream&, Archive::Writer&,
                               const ValuePointer) const;

        virtual void deserialize(std::istream&, Archive::Reader&,
                                 ValuePointer) const;

        virtual void reconstitute(Archive::Reader&, Object*) const;

        //
        //  Variant Type instances require using placement new to create
        //  the instance. Only the VariantTagType's representation type
        //  knows how to call the constructor.
        //

        virtual void constructInstance(Pointer) const;

        //
        //  An already constructed instance copied to an an already
        //  constructed instance. the from and to objects will already be
        //  constructed, so in the case of an Object* or derivative, the
        //  type information will already be there and only the fields
        //  need to be copied.
        //

        virtual void copyInstance(Pointer from, Pointer to) const;

    protected:
        Type(Context* context, const MachineRep*);

    protected:
        static NODE_DECLARAION(dataNodeReturnValue, Value);

    protected:
        ReferenceType* _referenceType;
        const MachineRep* _machineRep;
        bool _isTypePattern : 1;
        bool _isTypeVariable : 1;
        bool _isUnresolvedType : 1;
        bool _isRefType : 1;
        bool _isPrimitive : 1;
        bool _selfManaged : 1;
        bool _isAggregate : 1;
        bool _isCollection : 1;
        bool _isSequence : 1;
        bool _isSerializable : 1;
        bool _isFixedSize : 1;
        bool _isGCAtomic : 1;
        bool _isMutable : 1;

        friend class Object;
        friend class ReferenceType;
    };

} // namespace Mu

#endif // __Mu__Type__h__
