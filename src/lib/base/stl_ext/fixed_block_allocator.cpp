//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <assert.h>
#include <string.h>
#include <stl_ext/fixed_block_allocator.h>
#include <stl_ext/pvector.h>
#include <iostream>

namespace stl_ext
{
    using namespace std;

    fixed_block_allocator::fixed_block_allocator(size_t value_size)
        : _blocks()
        , _free(0)
        , _value_size(value_size)
    {
        pthread_mutex_init(&_lock, 0);
        assert(value_size >= sizeof(pointer));
        _blocks.push_back(new byte[sizeof(byte) * _value_size]);
        _unused = (byte_pointer)_blocks.front();
        memset(_unused, 0, sizeof(byte) * _value_size);
    }

    fixed_block_allocator::~fixed_block_allocator()
    {
        pthread_mutex_destroy(&_lock);
        for (int i = 0; i < _blocks.size(); i++)
            delete[] _blocks[i];
    }

    fixed_block_allocator::pointer fixed_block_allocator::allocate()
    {
        pthread_mutex_lock(&_lock);

        if (_free)
        {
            dead_object_pointer obj = _free;
            _free = (dead_object_pointer)obj->pointer();

            /* AJG - doesn't like STL_EXT_DEBUG
            #if STL_EXT_DEBUG
                    assert(_free != obj);
            #endif
            */
            pthread_mutex_unlock(&_lock);

            return (pointer)obj;
        }
        else
        {
            size_t index = _blocks.size() - 1;
            size_t size = (1 << index) * _value_size;

            if (_unused >= (_blocks[index] + size))
            {
                //
                // need to allocate another block _unused is past end
                //

                index++;
                size = (1 << index) * _value_size;

                try
                {
                    _unused = new byte[sizeof(byte) * size];
                }
                catch (...)
                {
                    pthread_mutex_unlock(&_lock);
                    throw;
                }

                memset(_unused, 0, sizeof(byte) * size);
                _blocks.push_back(_unused);
            }

            pointer p = (pointer)_unused;
            _unused += _value_size;

            pthread_mutex_unlock(&_lock);
            return p;
        }
    }

    void fixed_block_allocator::deallocate(fixed_block_allocator::pointer p)
    {
        if (p)
        {
            pthread_mutex_lock(&_lock);

            /* AJG - doesn't like STL_EXT_DEBUG
            #if STL_EXT_DEBUG
                    assert(is_live(p));
            #endif
            */
            dead_object_pointer obj = (dead_object_pointer)p;
            obj->pointer(_free);
            _free = obj;

            pthread_mutex_unlock(&_lock);
        }
    }

    size_t fixed_block_allocator::capacity()
    {
        size_t bytes = 0;

        for (int i = 0; i < _blocks.size(); i++)
        {
            bytes += (1 << i) * _value_size;
        }

        return bytes;
    }

    bool fixed_block_allocator::is_allocated(
        fixed_block_allocator::const_pointer ptr) const
    {
        const_byte_pointer p = (const_byte_pointer)ptr;

        for (int i = _blocks.size(); i--;)
        {
            //
            //  This loop is backwards on purpose: the blocks are in order
            //  of size so the largest is last. Odds are the pointer is in
            //  that block (50% chance) so that should be checked
            //  first. The odds are 50% that it will be found in next
            //  block if not in the first, etc.. (assuming the thing was
            //  actually allocated by the class). This is your basic
            //  binary search, but linear in the block list.
            //

            size_t size = (1 << i) * _value_size;

            if (p >= _blocks[i] && p < (_blocks[i] + size))
            {
                //
                //	Now check the alignment to find out if its *really* an
                //	object in this block.
                //

                return (size_t(p) - size_t(_blocks[i])) % _value_size == 0;
            }
        }

        return false;
    }

    bool
    fixed_block_allocator::is_live(fixed_block_allocator::const_pointer ptr)
    {
        if (is_allocated(ptr))
        {
            for (dead_object_pointer p = _free; p;
                 p = (dead_object_pointer)p->pointer())
            {
                if (p == ptr)
                    return false;
            }

            return true;
        }

        return false;
    }

    void fixed_block_allocator::mark_free(bool b)
    {
        pthread_mutex_lock(&_lock);

        for (dead_object_pointer p = _free; p;
             p = (dead_object_pointer)(p->pointer()))
        {
            /* AJG - doesn't like STL_EXT_DEBUG
            #if STL_EXT_DEBUG
                    assert(is_allocated(p));
            #endif
            */
            p->mark(b);
        }

        pthread_mutex_unlock(&_lock);
    }

    //----------------------------------------------------------------------

    void fixed_block_allocator::iterator::init()
    {
        if (_block_num < _ba->_blocks.size() && _ba->_blocks[_block_num])
        {
            _p = reinterpret_cast<object_pointer>(_ba->_blocks[_block_num]);
        }
        else
        {
            //
            // Make it into the end iterator if there's no data
            //

            _p = 0;
            _ba = 0;
        }
    }

    void fixed_block_allocator::iterator::next()
    {
        if (!_p)
            return;
        size_t size = (1 << _block_num) * _ba->_value_size;
        byte_pointer p = reinterpret_cast<byte_pointer>(_p);
        p += _ba->_value_size;

        if (_block_num == _ba->_blocks.size() - 1)
        {
            if (p >= _ba->_unused)
            {
                _p = 0;
                return;
            }
        }

        if (p >= _ba->_blocks[_block_num] + size)
        {
            _block_num++;
            init();
        }
        else
        {
            _p = reinterpret_cast<object_pointer>(p);
        }
    }

} // namespace stl_ext
