//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/List.h>
#include <Mu/Thread.h>

namespace Mu
{
    using namespace std;

    List::List(Process* p, ClassInstance* o)
        : _process(p)
        , _current(o)
        , _head(o)
    {
        if (o)
        {
            _type = static_cast<const ListType*>(_current->classType());
            _voff = _type->valueOffset();
            _noff = _type->nextOffset();
        }
        else
        {
            _type = 0;
            _voff = 0;
            _noff = 0;
        }
    }

    List::List(Process* p, const ListType* l)
        : _process(p)
        , _type(l)
        , _voff(l->valueOffset())
        , _noff(l->nextOffset())
    {
        _head = 0;
        _current = _head;
    }

    List::List(const ListType* l, Thread& thread, const Node* n)
        : _type(l)
        , _process(thread.process())
        , _voff(l->valueOffset())
        , _noff(l->nextOffset())
    {
        _head = ClassInstance::allocate(_type);
        _current = _head;
        _type->elementType()->nodeEval(valuePointer(), n, thread);
    }

    void List::appendDefaultValue()
    {
        while (_current && next())
            _current = next();
        ClassInstance* o = ClassInstance::allocate(_type);
        if (_current)
            next() = o;
        if (!_head)
            _head = o;
        _current = o;
    }

    void List::append(Thread& thread, const Node* n)
    {
        while (_current && next())
            _current = next();
        ClassInstance* o = ClassInstance::allocate(_type);
        if (_current)
            next() = o;
        if (!_head)
            _head = o;
        _current = o;
        _type->elementType()->nodeEval(valuePointer(), n, thread);
    }

    ClassInstance* List::splice(Thread& thread, const Node* n)
    {
        ClassInstance* o = ClassInstance::allocate(_type);
        ClassInstance* c = _current;
        if (!_head)
            _head = o;
        _current = o;
        _head = o;
        _type->elementType()->nodeEval(valuePointer(), n, thread);
        return o;
    }

} // namespace Mu
