//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <assert.h>
#include <stl_ext/block_alloc_arena.h>
#include <stl_ext/pvector.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>

namespace stl_ext
{
    using namespace std;

    block_alloc_arena block_alloc_arena::_static_arena;

    block_alloc_arena::block_alloc_arena(size_t gl, size_t lbs)
        : _running_allocation(0)
        , _garbage_limit(gl)
        , _large_block_size(lbs)
        , _garbage_function(0)
        , _disable(false)
    {
        pthread_mutex_init(&_lock, 0);
    }

    block_alloc_arena::~block_alloc_arena()
    {
        pthread_mutex_destroy(&_lock);
        for (auto p : _small_blocks)
            delete p;
        for (auto& p : _large_blocks)
            delete p.second;
    }

    inline size_t adjust_size(size_t n)
    {
        //
        //	Should user of the class be able to tune these?
        //

        if (n < 8)
            n = 8;
        else
            while (n & 0x7)
                n++;
        return n >> 3;
        // return n;
    }

    void block_alloc_arena::set_garbage_function(garbage_function func)
    {
        pthread_mutex_lock(&_lock);
        _garbage_function = func;
        pthread_mutex_unlock(&_lock);
    }

    void block_alloc_arena::reset_running_allocation()
    {
        pthread_mutex_lock(&_lock);
        _running_allocation = 0;
        pthread_mutex_unlock(&_lock);
    }

    void block_alloc_arena::set_garbage_limit(size_t limit)
    {
        pthread_mutex_lock(&_lock);
        _garbage_limit = limit;
        pthread_mutex_unlock(&_lock);
    }

    void block_alloc_arena::set_large_block_size(size_t t)
    {
        pthread_mutex_lock(&_lock);
        _large_block_size = t;
        pthread_mutex_unlock(&_lock);
    }

    void* block_alloc_arena::allocate(size_t n)
    {
        if (_disable)
        {
            return (void*)(new char[n]);
        }
        else
        {
            _running_allocation += n;
            if (_running_allocation >= _garbage_limit)
            {
                if (_garbage_function)
                    (*_garbage_function)(this);
                _running_allocation = 0;
            }

            pthread_mutex_lock(&_lock);

            size_t on = n;
            n = adjust_size(n);

            const bool large = on >= _large_block_size;
            allocator* a = 0;

            //
            //  Make room
            //

            if (large)
            {
                a = _large_blocks[n];

                if (!a)
                {
                    a = new allocator(n << 3);
                    _large_blocks[n] = a;
                }
            }
            else
            {
                while (_small_blocks.size() <= n)
                    _small_blocks.push_back(0);

                if (!_small_blocks[n])
                {
                    _small_blocks[n] = new allocator(n << 3);
                }

                a = _small_blocks[n];
            }

            pthread_mutex_unlock(&_lock);

            void* obj;

            try
            {
                obj = a->allocate();
            }
            catch (std::bad_alloc& e)
            {
                cerr << "ERROR: block_alloc_arena: failed to allocate " << on
                     << " bytes from arena size " << (n << 3) << endl;

                throw;
            }
            catch (std::exception& e)
            {
                cerr << "ERROR: caught std::exception during allocation"
                     << endl;

                throw;
            }

            return obj;
        }
    }

    void block_alloc_arena::deallocate(void* p, size_t n)
    {
        if (_disable)
        {
            delete[] ((char*)p);
        }
        else
        {
            if (p)
            {
                pthread_mutex_lock(&_lock);
                if (n <= _running_allocation)
                    _running_allocation -= n;

                if (n >= _large_block_size)
                {
                    n = adjust_size(n);
                    pthread_mutex_unlock(&_lock);
                    if (allocator* a = _large_blocks[n])
                        a->deallocate(p);
                }
                else
                {
                    n = adjust_size(n);
                    pthread_mutex_unlock(&_lock);
                    if (_small_blocks.size() > n && _small_blocks[n])
                        _small_blocks[n]->deallocate(p);
                }
            }
        }
    }

    bool block_alloc_arena::is_allocated(const void* p, size_t n) const
    {
        //
        //	If any of the last three bits are non-zero the alignment would
        // be 	wacky, so it can't possibly be pointing to a struct in here.
        //

        if ((size_t(p) & 0x7) != 0)
            return false;

        n = adjust_size(n);
        return _small_blocks.size() > n && _small_blocks[n]
                   ? _small_blocks[n]->is_allocated(p)
                   : false;
    }

    bool block_alloc_arena::is_allocated(const void* p) const
    {
        for (auto b : _small_blocks)
            if (b && b->is_allocated(p))
                return true;
        for (auto& bp : _large_blocks)
            if (bp.second->is_allocated(p))
                return true;
        return false;
    }

    bool block_alloc_arena::is_live(const void* p, size_t n)
    {
        n = adjust_size(n);
        if (_small_blocks.size() > n && _small_blocks[n])
            return _small_blocks[n]->is_live(p);
        auto i = _large_blocks.find(n);
        if (i != _large_blocks.end())
            return i->second->is_live(p);
        return false;
    }

    bool block_alloc_arena::is_live(const void* p)
    {
        for (auto b : _small_blocks)
            if (b && b->is_live(p))
                return true;
        for (auto& bp : _large_blocks)
            if (bp.second->is_live(p))
                return true;
        return false;
    }

    void block_alloc_arena::mark_free(bool b)
    {
        pthread_mutex_lock(&_lock);
        for (auto block : _small_blocks)
            if (block)
                block->mark_free(b);
        pthread_mutex_unlock(&_lock);
    }

    void block_alloc_arena::iterator::init()
    {
        if (_arena)
        {
            if (_arena->_small_blocks.size() && _arena->_small_blocks[_index])
            {
                _i = _arena->_small_blocks[_index]->begin();
            }
            else
            {
                _i = allocator::iterator();
                next();
            }
        }
    }

    void block_alloc_arena::iterator::next()
    {
        if (!_arena)
            return;
        allocator::iterator end;

        _i++;

        if (_i == end)
        {
            _index++;
            for (; _index < _arena->_small_blocks.size()
                   && !_arena->_small_blocks[_index];
                 _index++)
                ;
            if (_index >= _arena->_small_blocks.size())
            {
                _index--;
                return;
            }
            _i = _arena->_small_blocks[_index]->begin();
        }
    }

    size_t block_alloc_arena::capacity()
    {
        size_t bytes = 0;
        for (auto block : _small_blocks)
            if (block)
                bytes += block->capacity();
        return bytes;
    }

} // namespace stl_ext
