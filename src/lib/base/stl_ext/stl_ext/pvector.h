//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __stl_ext__pvector_h__
#define __stl_ext__pvector_h__
#include <vector>

namespace stl_ext
{

    //
    //  class pvector<>
    //
    //  pvector<> is an optimization for compilers that do not support
    //  partial template specialization; it implements a vector of
    //  pointers for any type based off a vector<void*> instance. The
    //  semantics are a bit different than the regular vector<> class: the
    //  pvector<> parameter is _assumed_ to be a pointer. So a pvector of
    //  Foo pointers will be declared like this:
    //
    //	pvector<Foo>
    //
    //  The return value of pvector<Foo>::operator[](size_t) is a
    //  Foo*. This class should function like any other STL class since it
    //  mimics the API of vector including the internal typedefs.
    //

    template <class Alloc> class void_vector
    {
    public:
        void_vector(size_t s = 0)
            : _v(s)
        {
        }

        void_vector(size_t s, void* i)
            : _v(s, i)
        {
        }

        void_vector(const void_vector& v)
            : _v(v)
        {
        }

        typedef std::vector<void*, Alloc> stl_vector;

        void clear() { _v.clear(); }

        void reserve(size_t t) { _v.reserve(t); }

        void resize(size_t t) { _v.resize(t); }

        size_t size() const { return _v.size(); }

        size_t max_size() const { return _v.max_size(); }

        size_t capacity() const { return _v.capacity(); }

        bool empty() const { return _v.empty(); }

        void pop_back() { _v.pop_back(); }

    protected:
        stl_vector _v;
    };

    //
    // class: pvector<class T>
    //

    template <class T, class Alloc = std::allocator<T>>
    class pvector : public void_vector<Alloc>
    {
    public:
        typedef T* value_type;
        typedef value_type* pointer;
        typedef const pointer const_pointer;
        typedef value_type& reference;
        // #ifndef _MSC_VER
        //     typedef const reference const_reference;
        // #endif
        typedef size_t size_type;
        typedef int difference_type;
        typedef pvector<T, Alloc> this_type;
        typedef typename void_vector<Alloc>::stl_vector vector_type;

        class iterator
        {
        public:
            typedef typename vector_type::iterator it;
            typedef iterator itype;

            iterator(const itype& i) { _i = i._i; }

            explicit iterator(const it& i) { _i = i; }

            void operator=(const itype& i) { _i = i._i; }

            void operator=(const it& i) { _i = i; }

            reference operator*() { return static_cast<value_type>(*_i); }
            // #ifdef _MSC_VER
            reference operator*() const
            // #else
            // const_reference	operator*() const
            // #endif
            {
                return static_cast<value_type>(*_i);
            }

            void operator++() { _i++; }

            void operator++(int) { ++_i; }

            void operator--() { _i--; }

            void operator--(int) { --_i; }

            void operator+=(int i) { _i += i; }

            void operator-=(int i) { _i -= i; }

            bool operator==(const itype& i) const { return _i == i._i; }

            bool operator!=(const itype& i) const { return _i != i._i; }

            bool operator<(const itype& i) const { return _i < i._i; }

            bool operator>=(const itype& i) const { return !(_i < i._i); }

            bool operator<=(const itype& i) const
            {
                return _i < i._i || (*this) == i;
            }

            bool operator>(const itype& i) const
            {
                return !((*this) < i) && ((*this) != i);
            }

            itype operator+(int i) const { return itype(_i + i); }

            itype operator-(int i) const { return itype(_i - i); }

        private:
            it _i;
            friend class stl_ext::pvector<T>;
        };

        class const_iterator
        {
        public:
            typedef typename vector_type::const_iterator it;
            typedef const_iterator itype;

            const_iterator(const itype& i) { _i = i._i; }

            explicit const_iterator(const it& i) { _i = i; }

            void operator=(const itype& i) { _i = i._i; }

            void operator=(const it& i) { _i = i; }

            reference operator*() { return static_cast<value_type>(*_i); }

            // #ifndef _MSC_VER
            //	const_reference	operator*() const
            //			{ return static_cast<value_type>(*_i); }
            // #endif
            void operator++() { _i++; }

            void operator++(int) { ++_i; }

            void operator--() { _i--; }

            void operator--(int) { --_i; }

            void operator+=(int i) { _i += i; }

            void operator-=(int i) { _i -= i; }

            bool operator==(const itype& i) const { return _i == i._i; }

            bool operator!=(const itype& i) const { return _i != i._i; }

            bool operator<(const itype& i) const { return _i < i._i; }

            bool operator>=(const itype& i) const { return !(_i < i._i); }

            bool operator<=(const itype& i) const
            {
                return _i < i._i || (*this) == i;
            }

            bool operator>(const itype& i) const
            {
                return !((*this) < i) && ((*this) != i);
            }

            itype operator+(int i) const { return itype(_i + i); }

            itype operator-(int i) const { return itype(_i - i); }

        private:
            it _i;
            friend class stl_ext::pvector<T>;
        };

        typedef iterator reverse_iterator;
        typedef const_iterator const_reverse_iterator;

        pvector()
            : void_vector<Alloc>()
        {
        }

        explicit pvector(size_type initial_size)
            : void_vector<Alloc>(initial_size)
        {
        }

        pvector(size_type initial_size, value_type X)
            : void_vector<Alloc>(initial_size, X)
        {
        }

        pvector(const this_type& v)
            : void_vector<Alloc>(v)
        {
        }

        this_type& operator=(const this_type& v)
        {
            _v(v._v);
            return *this;
        }

        // #ifndef _MSC_VER
        //     const_reference	    operator [] (size_type i) const
        //			    { return const_reference(this->_v[i]); }
        // #endif
        reference operator[](size_type i) { return reference(this->_v[i]); }

        iterator begin() { return iterator(this->_v.begin()); }

        const_iterator begin() const
        {
            return const_iterator(this->_v.begin());
        }

        iterator end() { return iterator(this->_v.end()); }

        const_iterator end() const { return const_iterator(this->_v.end()); }

        reverse_iterator rbegin()
        {
            return reverse_iterator(this->_v.rbegin());
        }

        const_reverse_iterator rbegin() const
        {
            return const_reverse_iterator(this->_v.rbegin);
        }

        reverse_iterator rend() { return reverse_iterator(this->_v.rend()); }

        const_reverse_iterator rend() const
        {
            return const_reverse_iterator(this->_v.rend());
        }

        reference front() { return reference(this->_v.begin()); }

        // #ifndef _MSC_VER
        //     const_reference	    front() const
        //			    { return const_reference(this->_v.begin());
        //} #endif
        reference back() { return reference(this->_v.back()); }

        // #ifndef _MSC_VER
        //     const_reference	    back() const { return
        //     const_reference(this->_v.back()); }
        // #endif

        //	note: push_back, insert, et al. use value_type as a parameter
        //	instead of const_reference (i.e. as opposed to
        //  std::vector<>::push_back(std::vector<>::const_reference) )
        //

        void push_back(value_type X) { this->_v.push_back(X); }

        void erase(iterator i) { this->_v.erase(i._i); }

        void erase(iterator first, iterator last)
        {
            this->_v.erase(first._i, last._i);
        }

        void insert(iterator pos, value_type X) { this->_v.insert(pos._i, X); }

        bool operator==(const this_type& pv) { return this->_v == pv._v; }

        bool operator<(const this_type& pv) { return this->_v < pv._v; }
    };

} // namespace stl_ext

#endif // __stl_ext__pvector_h__
