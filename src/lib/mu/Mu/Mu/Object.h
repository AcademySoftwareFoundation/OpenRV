#ifndef __Mu__Object__h__
#define __Mu__Object__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/config.h>
#include <Mu/Node.h>
#include <Mu/Type.h>
#include <stl_ext/markable_pointer.h>
#include <stl_ext/block_alloc_arena.h>

namespace Mu
{

    class Process;

    //
    //  class Object
    //
    //  Object is a dynamicly allocated chunck of memory. These is the
    //  basic unit of heap representation at run-time. (See also
    //  Mu::Value). The size of this class should be the wordsize of the
    //  machine (ideally). 32 bits is the only tested size.
    //
    //  The Object class does two things:
    //
    //      * Keeps type information for the object in question. Given
    //	  Object it is possible to retrieve the Type* and therefore any
    //	  information you could want about it. This is the main difference
    //	  between Object and Value.
    //
    //      * After being freed, a list of the objects of the same size is
    //        kept. Hence the union. Allocation becomes fast after the free
    //        list is established.
    //
    //  (This concerns use of the built-in precise collector)
    //  Note that functions which would normally be virtual in an ideal
    //  world are implemented by the Object's Type class. Since this class
    //  needs to have a size of MU_WORD_SIZE it cannot have any virtual
    //  functions -- the virtual function table would take up a
    //  word. Fortunately, since the Type is abvailable, these things are
    //  not a problem.
    //
    //  Porting Note: Since the Type* is smashed down a few bits, the
    //  assumption is that the memory of the Type class instance is word
    //  aligned (and therefor the least significant two bits of the
    //  pointer are always 00). This is necessarily true on PPC and
    //  similar RISC processors, but in some cases might need to be
    //  implemented by Type::operator new().
    //

    class Object
    {
    public:
        typedef STLMap<Object*, int>::Type ExternalHeap;

        MU_GCAPI_NEW_DELETE

        explicit Object(const Type*);

        //
        //	Returns the same type as is passed into the constructor.
        //

        const Type* type() const;

        void typeDelete();

        void retainExternal();
        void releaseExternal();

    protected:
        Object();
        ~Object();

        const Type* _type;

    private:
        static ExternalHeap _externalHeap;

        friend class GarbageCollector;
        friend class Process;
    };

    inline const Type* Object::type() const { return _type; }

} // namespace Mu

#endif // __Mu__Object__h__
