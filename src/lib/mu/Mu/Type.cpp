//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/Archive.h>
#include <Mu/MachineRep.h>
#include <Mu/Node.h>
#include <Mu/Object.h>
#include <Mu/Type.h>
#include <Mu/config.h>
#include <Mu/NilType.h>
#include <iostream>

namespace Mu
{

    using namespace std;

    Type::Type(Context* context, const char* name, const MachineRep* rep)
        : Symbol(context, name)
        , _machineRep(rep)
        , _referenceType(0)
        , _isTypePattern(false)
        , _isTypeVariable(false)
        , _isUnresolvedType(false)
        , _isRefType(false)
        , _isPrimitive(true)
        , _isAggregate(false)
        , _isCollection(false)
        , _isSequence(false)
        , _selfManaged(false)
        , _isSerializable(true)
        , _isFixedSize(true)
        , _isGCAtomic(false)
    {
        _searchable = true;
        _datanode = true;
    }

    Type::Type(Context* context, const MachineRep* rep)
        : Symbol(context)
        , _machineRep(rep)
        , _referenceType(0)
        , _isTypePattern(false)
        , _isTypeVariable(false)
        , _isUnresolvedType(false)
        , _isRefType(false)
        , _isPrimitive(true)
        , _isAggregate(false)
        , _isCollection(false)
        , _isSequence(false)
        , _selfManaged(false)
        , _isSerializable(true)
        , _isFixedSize(true)
        , _isGCAtomic(false)
        , _isMutable(false)
    {
        _searchable = true;
        _datanode = true;
    }

    Type::~Type() {}

    void Type::output(std::ostream& o) const
    {
        Symbol::output(o);
        if (isTypePattern())
        {
            o << " (pseudo-type)";
        }
    }

    void Type::outputNode(std::ostream& o, const Node* n) const
    {
        const DataNode* dn = static_cast<const DataNode*>(n);
        output(o);
        o << " = ";
        outputValue(o, dn->_data);
    }

    void Type::outputValueRecursive(ostream& o, const ValuePointer,
                                    ValueOutputState&) const
    {
        o << "(type does not implement Type::outputValueRecursive())";
    }

    void Type::outputValue(std::ostream& o, const Value& value, bool full) const
    {
        o << "(type does not implement Type::outputValue() for Value)";
    }

    void Type::outputValue(std::ostream& o, const ValuePointer p,
                           bool full) const
    {
        ValueOutputState state(o, full);
        outputValueRecursive(o, p, state);
    }

    bool Type::match(const Type* t) const
    {
        Bindings bindings;
        return match(t, bindings) == Match;
    }

    Type::MatchResult Type::match(const Type* other, Bindings& bindings) const
    {
        if (isPrimitiveType())
        {
            return this == other ? Match : NoMatch;
        }
        else if (other->isPrimitiveType())
        {
            return NoMatch;
        }
        else
        {
            return dynamic_cast<const NilType*>(other) != 0 ? Match : NoMatch;
        }
    }

    size_t Type::objectSize() const { return 0; }

    const Type* Type::nodeReturnType(const Node*) const { return this; }

    void Type::deleteObject(Object* obj) const
    {
        cerr << "Type::deleteObject -- " << hex << obj << " " << name() << endl;
    }

    void Type::constructInstance(Pointer) const { abort(); }

    void Type::copyInstance(Pointer from, Pointer to) const { abort(); }

    const Type* Type::fieldType(size_t) const { return 0; }

    ValuePointer Type::fieldPointer(Object*, size_t) const { return 0; }

    const ValuePointer Type::fieldPointer(const Object*, size_t) const
    {
        return 0;
    }

    void Type::serialize(std::ostream& o, Archive::Writer& archive,
                         const ValuePointer p) const
    {
        if (isPrimitiveType())
        {
            o.write((const char*)p, machineRep()->size());
        }
        else
        {
            const Object* obj = *reinterpret_cast<const Object**>(p);
            const Type* ftype = 0;
            ValuePointer fp = 0;
            size_t index = 0;

            while ((ftype = fieldType(index))
                   && (fp = fieldPointer(obj, index)))
            {
                if (ftype->isPrimitiveType())
                {
                    ftype->serialize(o, archive, fp);
                }
                else
                {
                    const Object* fobj = *static_cast<const Object**>(fp);
                    archive.writeObjectId(o, fobj);
                }

                index++;
            }
        }
    }

    void Type::deserialize(std::istream& i, Archive::Reader& archive,
                           ValuePointer p) const
    {
        if (isPrimitiveType())
        {
            i.read((char*)p, machineRep()->size());
        }
        else
        {
            Object* obj = *reinterpret_cast<Object**>(p);
            const Type* ftype = 0;
            ValuePointer fp = 0;
            size_t index = 0;

            while ((ftype = fieldType(index))
                   && (fp = fieldPointer(obj, index)))
            {
                if (ftype->isPrimitiveType())
                {
                    ftype->deserialize(i, archive, fp);
                }
                else
                {
                    //
                    //  This is the object id (or nil)
                    //

                    *(size_t*)fp = archive.readObjectId(i);
                }

                index++;
            }
        }
    }

    void Type::reconstitute(Archive::Reader& archive, Object* obj) const
    {
        const Type* ftype = 0;
        ValuePointer fp = 0;
        size_t index = 0;

        while ((ftype = fieldType(index)) && (fp = fieldPointer(obj, index)))
        {
            if (!ftype->isPrimitiveType())
            {
                size_t id = *((size_t*)fp);
                *((Object**)fp) = archive.objectOfId(id);
            }

            index++;
        }
    }

    NODE_IMPLEMENTATION(Type::dataNodeReturnValue, Value)
    {
        return ((const DataNode&)NODE_THIS)._data;
    }

} // namespace Mu
