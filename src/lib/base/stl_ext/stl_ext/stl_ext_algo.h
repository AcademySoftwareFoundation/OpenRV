//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __stl_ext_algo__h__
#define __stl_ext_algo__h__
#include <algorithm>
#include <iterator>

namespace stl_ext
{

    //
    //  Function remove
    //
    //  This removes an element from a Container by erasing it. It does a
    //  linear search to find the element to remove.
    //

    template <typename C>
    void remove(C& container, typename C::value_type value)
    {
        typename C::iterator i =
            std::find(container.begin(), container.end(), value);

        if (i != container.end())
        {
            container.erase(i);
        }
    }

    //
    //  Function remove_unsorted
    //
    //  This removes an element from a Container by copying the last
    //  element in the container on top of the element to be removed. It
    //  then erases the last element.
    //

    template <class Container>
    void remove_unsorted(Container& container,
                         typename Container::value_type value)
    {
        typename Container::iterator i =
            std::find(container.begin(), container.end(), value);

        if (i != container.end())
        {
            *i = container.back();
            container.erase(container.begin() + container.size() - 1);
        }
    }

    //
    //  Function delete_contents
    //
    //  Deletes the contents of a container. Useful when the container
    //  holds pointers.
    //

    template <class Container> inline void delete_contents(Container& container)
    {
        for (typename Container::iterator i = container.begin();
             i != container.end(); ++i)
            delete *i;
    }

    //
    //  Function exists
    //
    //  Returns true if the element exists inside the container false otherwise.
    //

    template <class Container>
    inline bool exists(const Container& container,
                       typename Container::value_type value)
    {
        return std::find(container.begin(), container.end(), value)
               != container.end();
    }

    //
    //  Cast Adapters for std::transform. So transform can be used to copy
    //  from a container of one type to another.
    //

    template <class From, class To> struct StaticPointerCast
    {
        To* operator()(From* x) const { return static_cast<To*>(x); }
    };

    template <class From, class To> struct DynamicPointerCast
    {
        To* operator()(From* x) const { return dynamic_cast<To*>(x); }
    };

    template <class From, class To> struct ReinterpertCast
    {
        To operator()(From x) const { return reinterpret_cast<To>(x); }
    };

    template <class From, class To> struct StaticCast
    {
        To operator()(From x) const { return static_cast<To>(x); }
    };

    template <class From, class To> struct DynamicCast
    {
        To operator()(From x) const { return dynamic_cast<To>(x); }
    };

    //
    //  Predicates. (All end in _p)
    //

    template <class From, class To> struct IsA_p
    {
        bool operator()(From* s) const { return dynamic_cast<To*>(s) != 0; }
    };

    template <class From, class To> struct IsNotA_p
    {
        bool operator()(From* s) const { return dynamic_cast<To*>(s) == 0; }
    };

    template <class T> struct IsNull_p
    {
        bool operator()(T* p) const { return p == 0; }
    };

    template <class T> struct IsNonNull_p
    {
        bool operator()(T* p) const { return p != 0; }
    };

    //
    //  Full container versions of std algorithms
    //

    template <class Container, class UnaryFunction>
    inline typename Container::iterator for_each(Container& c, UnaryFunction f)
    {
        return std::for_each(c.begin(), c.end(), f);
    }

    template <class Container>
    inline typename Container::iterator
    find(Container& c, const typename Container::value_type& v)
    {
        return std::find(c.begin(), c.end(), v);
    }

    template <class Container> inline void sort(Container& c)
    {
        std::sort(c.begin(), c.end());
    }

    template <class Container, class OutputIterator>
    inline OutputIterator copy(const Container& c, OutputIterator i)
    {
        return std::copy(c.begin(), c.end(), i);
    }

} // namespace stl_ext

#endif // __stl_ext_algo__h__
