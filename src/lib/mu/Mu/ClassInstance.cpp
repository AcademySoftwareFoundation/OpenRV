//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/ClassInstance.h>
#include <Mu/Class.h>
#include <Mu/Thread.h>
#include <Mu/Context.h>
#include <Mu/MemberVariable.h>
#include <Mu/GarbageCollector.h>

// #include <stl_ext/block_alloc_arena.h>

namespace Mu
{
    using namespace std;
    using namespace stl_ext;

    ClassInstance::ClassInstance()
        : Object()
    {
    }

    ClassInstance::ClassInstance(const Class* type)
        : Object(type)
    {
        if (!type->isFrozen())
        {
            Class* c = const_cast<Class*>(type);
            c->freeze();
        }

        if (size_t size = type->instanceSize())
        {
            memset(structure(), 0, size);
        }

        //
        //  Initialize multiple inheritance layout. See Class.cpp comments
        //  in freeze().
        //

        const Class::TypeMemberVariableVector& tms = type->typeMembers();

        for (size_t i = 0, s = tms.size(); i < s; i++)
        {
            const InternalTypeMemberVariable* im = tms[i];

            const Class** cp = reinterpret_cast<const Class**>(
                structure() + im->instanceOffset());
            *cp = im->value();
        }
    }

    ClassInstance::ClassInstance(Thread& thread, const char* cname)
        : Object()
    {
        //
        //  This is a performance critical section. It needs to be
        //  optimized with thread specific cache or something.
        //

        Context* context = thread.context();

        Name n = context->internName(cname);
        Class* t = context->findSymbolOfTypeByQualifiedName<Class>(n);
        assert(t);
        if (!t->isFrozen())
            t->freeze();

        if (size_t size = t->instanceSize())
        {
            memset(structure(), 0, size);
        }

        //
        //  Do what Object constructor would normally do
        //

        _type = (Type*)t;
#ifdef MU_USE_BASE_COLLECTOR
        _type.mark(GarbageCollector::mark());
#endif
    }

    ClassInstance* ClassInstance::allocate(const Class* c)
    {
        size_t s = c->objectSize();
        unsigned char* obj =
            (unsigned char*)(c->isGCAtomic() ? MU_GC_ALLOC_ATOMIC(s)
                                             : MU_GC_ALLOC(s));
        c->constructInstance(obj);
        return (ClassInstance*)obj;
    }

    ClassInstance* ClassInstance::allocate(Thread& thread, const char* c)
    {
        Context* context = thread.context();
        Name n = context->internName(c);
        const Class* t = context->findSymbolOfTypeByQualifiedName<Class>(n);
        assert(t);
        size_t s = t->objectSize();
        unsigned char* obj =
            (unsigned char*)(t->isGCAtomic() ? MU_GC_ALLOC_ATOMIC(s)
                                             : MU_GC_ALLOC(s));
        t->constructInstance(obj);
        return (ClassInstance*)obj;
    }

    ClassInstance::~ClassInstance()
    {
        // if (size_t size = classType()->instanceSize())
        //{
        // Object::arena().deallocate(_structure, size);
        //_structure = (Structure)MU_DELETED;
        //}
    }

    void ClassInstance::deallocate(ClassInstance* p) { MU_GC_FREE(p); }

    unsigned long ClassInstance::hash() const
    {
        const Class* c = classType();
        size_t s = c->instanceSize();

        unsigned long h = 0, g;

        for (int i = 0; i < s; i++)
        {
            h = (h << 4) + structure()[i];
            if (g = h & 0xf0000000)
                h ^= g >> 24;
            h &= ~g;
        }

        return h ^ (unsigned long)c;
    }

    ValuePointer ClassInstance::field(size_t i)
    {
        return structure()
               + classType()->memberVariables()[i]->instanceOffset();
    }

    const ValuePointer ClassInstance::field(size_t i) const
    {
        return structure()
               + classType()->memberVariables()[i]->instanceOffset();
    }

} // namespace Mu
