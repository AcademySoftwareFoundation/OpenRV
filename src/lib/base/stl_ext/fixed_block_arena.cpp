//******************************************************************************
// Copyright (c) 2001 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <assert.h>
#include <string.h>
#include <stl_ext/fixed_block_arena.h>
#include <stl_ext/pvector.h>
#include <iostream>
#include <stl_ext/stl_ext_algo.h>

namespace stl_ext
{
    using namespace std;

    fixed_block_arena::fixed_block_arena(size_t value_size)
    {
        pthread_mutex_init(&_lock, 0);
        _allocators.push_back(new fixed_block_allocator(value_size));
    }

    fixed_block_arena::~fixed_block_arena()
    {
        pthread_mutex_destroy(&_lock);
        delete_contents(_allocators);
    }

    fixed_block_arena::pointer fixed_block_arena::allocate()
    {
        try
        {
            return _allocators.back()->allocate();
        }
        catch (std::bad_alloc& e)
        {
            pthread_mutex_lock(&_lock);
            size_t size = _allocators.back()->_value_size;
            _allocators.push_back(new fixed_block_allocator(size));

            //
            //  This could fail, but if it does, there's nothing to be
            //  done
            //

            pthread_mutex_unlock(&_lock);
            return _allocators.back()->allocate();
        }
    }

    void fixed_block_arena::deallocate(fixed_block_arena::pointer p)
    {
        if (p)
        {
            if (_allocators.size() == 1)
            {
                _allocators.back()->deallocate(p);
            }
            else
            {
                //
                //  Go back to front -- its more likely the object was
                //  recently allocated. NOTE: this could be improved
                //  greatly by testing only the largest block for each
                //  allocator instead of using the is_allocated()
                //  function. That would make it possible to quickly check
                //  the largest amount of memory first and then later
                //  check the small parts.
                //

                for (int i = _allocators.size() - 1; i >= 0; i--)
                {
                    allocator_pointer ap = _allocators[i];

                    if (ap->is_allocated(p))
                    {
                        ap->deallocate(p);
                        return;
                    }
                }

                //
                //  Should not get here
                //

                abort();
            }
        }
    }

    size_t fixed_block_arena::capacity()
    {
        size_t bytes = 0;

        for (int i = 0; i < _allocators.size(); i++)
        {
            bytes += _allocators[i]->capacity();
        }

        return bytes;
    }

    bool
    fixed_block_arena::is_allocated(fixed_block_arena::const_pointer p) const
    {
        for (int i = _allocators.size() - 1; i >= 0; i--)
        {
            if (_allocators[i]->is_allocated(p))
                return true;
        }

        return false;
    }

    bool fixed_block_arena::is_live(fixed_block_arena::const_pointer p)
    {
        for (int i = _allocators.size() - 1; i >= 0; i--)
        {
            if (_allocators[i]->is_live(p))
                return true;
        }

        return false;
    }

    void fixed_block_arena::mark_free(bool b)
    {
        for (int i = _allocators.size() - 1; i >= 0; i--)
        {
            _allocators[i]->mark_free(b);
        }
    }

    //----------------------------------------------------------------------

    void fixed_block_arena::iterator::init()
    {
        _it = _arena->_allocators.front()->begin();
    }

    void fixed_block_arena::iterator::next()
    {
        if (_arena && _index < _arena->_allocators.size())
        {
            _it++;

            if (_it == _arena->_allocators[_index]->end())
            {
                _index++;

                if (_index >= _arena->_allocators.size())
                {
                    _arena = 0;
                    return;
                }
            }
        }
    }

} // namespace stl_ext
