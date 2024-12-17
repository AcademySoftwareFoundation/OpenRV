#ifndef __Mu__ArenaAllocator__h__
#define __Mu__ArenaAllocator__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Object.h>

namespace Mu
{

    //
    //  class ArenaAllocator
    //
    //  This is an STL allocator which uses the Object::arena() to
    //  allocate. You can use this with standard STL containers if you
    //  need their memory to be included in the garbage collectors
    //  book-keeping.
    //

    template <class T> class ArenaAllocator
    {
    public:
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T value_type;

        template <class U> struct rebind
        {
            typedef ArenaAllocator<U> other;
        };

        ArenaAllocator() throw() {}

        ArenaAllocator(const ArenaAllocator&) throw() {}

        template <class U> ArenaAllocator(const ArenaAllocator<U>) throw() {}

        ~ArenaAllocator() throw() {}

        pointer address(reference x) const { return &x; }

        const_pointer address(const_reference x) const { return &x; }

        void construct(pointer p, const T& val) { new (p) T(val); }

        void destroy(pointer p) { ((T*)p)->~T(); }

        pointer allocate(size_type n, const void* = 0)
        {
            return reinterpret_cast<T*>(
                Object::arena().allocate(n * sizeof(T)));
        }

        void deallocate(pointer p, size_type n)
        {
            Object::arena().deallocate(p, n * sizeof(T));
        }

        size_type max_size() const throw() { return size_t(-1) / sizeof(T); }
    };

    template <> class ArenaAllocator<void>
    {
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef void* pointer;
        typedef const void* const_pointer;
        typedef void value_type;

        template <class U> struct rebind
        {
            typedef ArenaAllocator<U> other;
        };
    };

} // namespace Mu

#endif // __Mu__ArenaAllocator__h__
