//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __stl_ext__fixed_block_allocator_h__
#define __stl_ext__fixed_block_allocator_h__
#include <stl_ext/stl_ext_config.h>
#include <stl_ext/markable_pointer.h>
#include <pthread.h>
#include <assert.h>
#include <vector>

namespace stl_ext
{

    //
    //  class fixed_block_allocator
    //
    //  fixed_block_allocator has four public functions which determine
    //  its behavior completely:
    //
    //      allocate()      - return a (possibly) new region of memory
    //  		          for a given sized object
    //      deallocate()    - free a previously allocated() memory location
    //      is_allocated()  - determine if a memory object was allocated by
    //  		          the fixed_block_allocator
    //      is_live()	- determine if a memory object is live or dead
    //
    //  The complexity for is_allocated is K * O(N) with the number of
    //  blocks in the worst case. The number of blocks if N objects are
    //  allocated is ...
    //
    //  		        log ((N+1)/2)
    //			   2
    //
    //  which is the max number of intervals that needs to be checked.
    //  The complexity of is_live() is O(N + N    ) which is very slow.
    //                                        free
    //
    //  This class is useful in the context of a garbage collector. In
    //  that case, you will want to use the additional iterator class
    //  which iterates over the objects in memory (returns
    //  markable_pointer refererences) and the mark_free() function.
    //
    //  For garbage collected objects, the first member of each allocated
    //  object should be a markable_pointer.
    //

    class STL_EXT_EXPORT fixed_block_allocator
    {
    public:
        typedef void value_type;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef size_t size_type;
        typedef fixed_block_allocator this_type;
        typedef unsigned char byte;
        typedef byte* byte_pointer;
        typedef const byte_pointer const_byte_pointer;
        typedef std::vector<byte_pointer> internal_vector;
        typedef markable_pointer object;
        typedef object* object_pointer;
        typedef object& object_reference;
        typedef markable_pointer dead_object;
        typedef dead_object* dead_object_pointer;

        fixed_block_allocator(size_t value_size);
        ~fixed_block_allocator();

        pointer allocate();
        void deallocate(pointer);

        bool is_allocated(const_pointer) const;
        bool is_live(const_pointer);

        size_t capacity();

        class iterator
        {
        public:
            iterator()
                : _block_num(0)
                , _p(0)
                , _ba(0)
            {
            }

            iterator(fixed_block_allocator* b)
                : _block_num(0)
                , _ba(b)
            {
                init();
            }

            object_reference operator*();

            object_pointer operator->() { return &(operator*()); }

            void operator++() { next(); }

            void operator++(int) { next(); }

            bool operator==(const iterator& i) const { return _p == i._p; }

            bool operator!=(const iterator& i) const { return _p != i._p; }

        private:
            void next();
            void init();

        private:
            size_t _block_num;
            object_pointer _p;
            const fixed_block_allocator* _ba;
        };

        void mark_free(bool);

        iterator begin() { return iterator(this); }

        iterator end() { return iterator(); }

    private:
        fixed_block_allocator() {}

        fixed_block_allocator(const fixed_block_allocator&) {}

        void operator=(const fixed_block_allocator&) {}

    private:
        internal_vector _blocks;
        size_t _value_size;
        byte_pointer _unused;
        dead_object_pointer _free;
        pthread_mutex_t _lock;
        friend class iterator;
        friend class fixed_block_arena;
    };

    inline fixed_block_allocator::object_reference
    fixed_block_allocator::iterator::operator*()
    {
        // #if STL_EXT_DEBUG
        //    assert(_p);
        // #endif
        return *_p;
    }

} // namespace stl_ext

#endif // __stl_ext__fixed_block_allocator_h__
