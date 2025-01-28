//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __stl_ext__slice_h__
#define __stl_ext__slice_h__

namespace stl_ext
{

    //
    //  slice provides a way to index into an existing vector or
    //  deque. The access methods do not yet allow iterators.
    //
    //  push_back() will add elements to the master container until the element
    //  would appear in the slice.
    //
    //  There are two versions of slice: the normal template class version
    //  with constructors and a slice_struct version which does not have
    //  constructors. If you need to use the slice in a union (like the
    //  yacc union for instance) you should use the struct version. Note
    //  that if you make the slice start at 0 with a stride of 1 you
    //  basically have a proxy to the underlying container.
    //

    template <class Container> struct stride_struct;

    template <class Container> class slice
    {
    public:
        typedef typename Container::value_type value_type;
        typedef value_type* pointer;
        typedef const pointer const_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef size_t size_type;
        typedef int difference_type;
        typedef slice<Container> this_type;

        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;
        typedef typename Container::reverse_iterator reverse_iterator;
        typedef
            typename Container::const_reverse_iterator const_reverse_iterator;

        slice(Container& c, iterator i, int stride = 1)
            : _c(&c)
            , _start(i - c.begin())
            , _stride(stride)
        {
        }

        slice(Container& c, size_t i, int stride = 1)
            : _c(&c)
            , _start(i)
            , _stride(stride)
        {
        }

        slice(Container& c, int stride = 1)
            : _c(&c)
            , _start(0)
            , _stride(stride)
        {
        }

        slice()
            : _c(0)
            , _start(0)
            , _stride(0)
        {
        }

        size_t start() const { return _start; }

        size_t stride() const { return _stride; }

        Container& container() { return *_c; }

        const Container& container() const { return *_c; }

        const_reference operator[](size_type i) const
        {
            return const_reference(container()[i * _stride + _start]);
        }

        reference operator[](size_type i)
        {
            return reference(container()[i * _stride + _start]);
        }

        iterator begin() { return _c->begin(); }

        const_iterator begin() const { return ((const Container*)_c)->begin(); }

        iterator end() { return _c->end(); }

        const_iterator end() const { return ((const Container*)_c)->end(); }

        void push_back(const value_type& t)
        {
            do
            {
                container().push_back(t);
            } while ((container().size() - _start) % _stride);
        }

        reference front() { return (*_c)[start()]; }

        const_reference front() const { return (*_c)[start()]; }

        bool empty() const { return _c ? container().empty() : true; }

        size_t size() const
        {
            return empty() ? 0 : (container().size() - _start) / _stride;
        }

        const_reference back() const { return (*this)[size() - 1]; }

        reference back() { return (*this)[size() - 1]; }

    private:
        Container* _c;
        size_t _start;
        int _stride;
    };

    //
    //  This is a no-constructor version that you can use in a union.
    //

    template <class Container> struct slice_struct
    {
        typedef typename Container::value_type value_type;
        typedef value_type* pointer;
        typedef const pointer const_pointer;
        typedef value_type& reference;
        typedef reference const_reference;
        typedef size_t size_type;
        typedef int difference_type;
        typedef slice_struct<Container> this_type;

        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;
        typedef typename Container::reverse_iterator reverse_iterator;
        typedef
            typename Container::const_reverse_iterator const_reverse_iterator;

        Container* _c;
        size_t _start;
        int _stride;

        size_t start() const { return _start; }

        size_t stride() const { return _stride; }

        Container& container() { return *_c; }

        const Container& container() const { return *_c; }

        const_reference operator[](size_type i) const
        {
            return const_reference(container()[i * _stride + _start]);
        }

        reference operator[](size_type i)
        {
            return reference(container()[i * _stride + _start]);
        }

        void push_back(const value_type& t)
        {
            do
            {
                container().push_back(t);
            } while ((container().size() - _start) % _stride);
        }

        iterator begin() { return _c->begin(); }

        const_iterator begin() const { return ((const Container*)_c)->begin(); }

        iterator end() { return _c->end(); }

        const_iterator end() const { return ((const Container*)_c)->end(); }

        reference front() { return (*_c)[start()]; }

        const_reference front() const { return (*_c)[start()]; }

        bool empty() const { return _c ? container().empty() : true; }

        size_t size() const
        {
            return empty() ? 0 : (container().size() - _start) / _stride;
        }

        const_reference back() const { return (*this)[size() - 1]; }

        reference back() { return (*this)[size() - 1]; }
    };

} // namespace stl_ext

#endif // __stl_ext__slice_h__
