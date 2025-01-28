//******************************************************************************
// Copyright (c) 2001 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __stl_ext__barray__h_
#define __stl_ext__barray__h_
#include <vector>

namespace stl_ext
{

    //
    //  class barray
    //
    //  This class implements an non-contiguous array with random access
    //  indexing. Indexing looks like this:
    //
    //	    *(B[index>>P]+index%P)
    //
    //  Where B is a an array of blocks of elements of size 2^P. This is a
    //  relatively inexpensive look up. The address of the nth element in
    //  a barray<> will never change.
    //
    //  barray<> uses a fixed size (power of 2) block so its important to have
    //  an estimate of array size bounds.
    //

    template <class T, unsigned int Bpower = 8,
              class Alloc = std::allocator<T*>>
    class barray
    {
    public:
        typedef T value_type;
        typedef size_t size_type;
        typedef barray<T, Bpower, Alloc> this_type;
        typedef typename Alloc::template rebind<T>::other other_alloc;

        T& operator[](int i)
        {
            return *(_blocks[i >> Bpower] + i % (1 << Bpower));
        }

        const T& operator[](int i) const
        {
            return *(_blocks[i >> Bpower] + i % (1 << Bpower));
        }

        class iterator
        {
        public:
            iterator(this_type& ba, size_t i = 0)
                : _ba(&ba)
                , _index(i)
            {
            }

            T& operator*() { return (*_ba)[_index]; }

            const T& operator*() const { return (*_ba)[_index]; }

            void operator++() { _index++; }

            void operator++(int) { _index++; }

            void operator--() { _index--; }

            void operator--(int) { _index--; }

        private:
            this_type* _ba;
            size_t _index;
            friend class stl_ext::barray<T, Bpower, Alloc>;
        };

        class const_iterator
        {
        public:
            const_iterator(const this_type& ba, size_t i)
                : _ba(&ba)
                , _index(i)
            {
            }

            const T& operator*() const { return (*_ba)[_index]; }

            void operator++() { _index++; }

            void operator++(int) { _index++; }

            void operator--() { _index--; }

            void operator--(int) { _index--; }

        private:
            const this_type* _ba;
            size_t _index;
            friend class stl_ext::barray<T, Bpower, Alloc>;
        };

        typedef iterator difference_iterator;

        barray()
            : _blocks()
            , _size(0)
        {
        }

        barray(size_t s)
            : _blocks()
            , _size(0)
        {
            resize(s);
        }

        ~barray();

        size_t size() const { return _size; }

        size_t capacity() const { return _blocks.size() * (2 << Bpower); }

        void reserve(size_t n)
        {
            size_t s = size();
            resize(n);
            resize(s);
        }

        void clear() { resize(0); }

        iterator begin() { return iterator(this); }

        const_iterator begin() const { return iterator(this); }

        iterator end() { return iterator(this, size()); }

        const_iterator end() const { return iterator(this, size()); }

        iterator rbegin() { return iterator(this); }

        const_iterator rbegin() const { return iterator(this); }

        iterator rend() { return iterator(this, size()); }

        const_iterator rend() const { return iterator(this, size()); }

        void resize(size_t, T c = T());

        T& front() { return (**_blocks); }

        const T& front() const { return (**_blocks); }

        T& back() { return ((*this)[size() - 1]); }

        const T& back() const { return ((*this)[size() - 1]); }

        void push_back(const T& X);
        void erase(iterator i);
        void erase(iterator first, iterator last);
        void insert(iterator pos, value_type X);

        bool operator==(const this_type& pv);
        bool operator<(const this_type& pv);

        bool is_allocated(const T*);
        bool is_using(const T*);
        size_t index_of(const T*);

    private:
        std::vector<T*, Alloc> _blocks;
        size_t _size;
        // Alloc                 _alloc;
        other_alloc _talloc;
        // Alloc                 _talloc;
    };

    template <class T, unsigned int Bpower, class Alloc>
    barray<T, Bpower, Alloc>::~barray()
    {
        for (int i = 0; i < _blocks.size(); i++)
        {
            _talloc.deallocate(_blocks[i], 1 << Bpower);
        }
    }

    template <class T, unsigned int Bpower, class Alloc>
    void barray<T, Bpower, Alloc>::resize(size_t nsize, T c)
    {
        if (_size < nsize)
        {
            for (int i = _blocks.size(); i <= nsize >> Bpower; i++)
            {
                _blocks.push_back(_talloc.allocate(1 << Bpower));
            }
        }

        for (size_t i = _size; i < nsize; i++)
            (*this)[i] = c;
        _size = nsize;
    }

    template <class T, unsigned int Bpower, class Alloc>
    inline void barray<T, Bpower, Alloc>::push_back(const T& X)
    {
        resize(_size + 1);
        back() = X;
    }

    template <class T, unsigned int Bpower, class Alloc>
    void barray<T, Bpower, Alloc>::erase(iterator it)
    {
        for (size_t i = it._index, s = size() = 1; i < s; i++)
            (*this)[i] = (*this)[i + 1];
        resize(size() - 1);
    }

    template <class T, unsigned int Bpower, class Alloc>
    void barray<T, Bpower, Alloc>::erase(iterator first, iterator last)
    {
        size_t i0 = first._index;
        size_t i1 = last._index;
        for (size_t i = i0, s = size() = 1; i < s; i++)
            (*this)[i] = (*this)[i + 1];
    }

    template <class T, unsigned int Bpower, class Alloc>
    bool barray<T, Bpower, Alloc>::is_allocated(const T* ptr)
    {
        for (size_t i = _blocks.size(); i--;)
            if (_blocks[i] <= ptr && ptr < _blocks[i] + (1 << Bpower))
                return true;
        return false;
    }

    template <class T, unsigned int Bpower, class Alloc>
    size_t barray<T, Bpower, Alloc>::index_of(const T* ptr)
    {
        for (size_t i = _blocks.size(); i--;)
            if (_blocks[i] <= ptr && ptr < _blocks[i] + (1 << Bpower))
                return i * (1 << Bpower) + (ptr - _blocks[i]);
        return _size;
    }

    template <class T, unsigned int Bpower, class Alloc>
    bool barray<T, Bpower, Alloc>::is_using(const T* ptr)
    {
        size_t index = index_of(ptr);
        return index < _size;
    }

} // namespace stl_ext

#endif // __stl_ext__barray__h_
