//
// Copyright (c) 2010, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __stl_ext__hash_table__h__
#define __stl_ext__hash_table__h__
#ifndef _MSC_VER
#include <unistd.h>
#endif

namespace stl_ext
{

    //
    //  class hash_table<T,Traits>
    //
    //  Implements a generic hash table. The Traits object type must have
    //  the following functions defined:
    //
    //	bool Traits::equals(const T&, const T&)
    //	hash_table<T>::HashValue Traits::hash(const T&);
    //
    //  The value type "T" must have the following defined:
    //
    //	Publicly destructable
    //	Default copy constructor
    //	Assignable
    //	Comparable with operator==
    //
    //  Note: This code was transplanted and does not fully follow STL
    //  conventions. The behavior is similar to the SGI hash_table class,
    //  but the implementation is completely different.
    //

    template <class T, class Traits> class hash_table
    {
    public:
        hash_table();
        ~hash_table();

        typedef hash_table<T, Traits> this_type;
        typedef unsigned long hash_value;
        typedef T value_type;

        //
        //	An item is part of a singly linked list.
        //

        class item
        {
        public:
            friend class hash_table<T, Traits>;
            item(const T& data);

            const T& data() const { return _data; }

            const item* next() const { return _next; }

            item* next() { return _next; }

        private:
            ~item() {}

            T _data;
            item* _next;
        };

        const item* insert(const T&);
        const item* find(const T&) const;
        const item* first_item_of(hash_value) const;

        //
        //	iterator over the items in a hash_table
        //
        //	for (iterator i(table); i; ++i)
        //	    doSomething(*i);
        //

        class iterator
        {
        public:
            iterator(hash_table<T, Traits>*);
            iterator(hash_table<T, Traits>&);

            operator bool() const { return _item ? true : false; }

            void operator++();

            void operator++(int) { this->operator++(); }

            const T& operator*() const { return _item->data(); }

            bool operator==(const iterator& i) const;
            bool operator!=(const iterator& i) const;

            int index() const { return _index; }

        private:
            this_type* _table;
            item* _item;
            size_t _index;
        };

        iterator begin() { return iterator(this); }

        iterator end() { return iterator(this, -1); }

    private:
        void resize();
        const item* add_item(item*);
        friend class iterator;

    private:
        unsigned int _size;
        unsigned int _tableSize;
        item** _table;
    };

    //
    //  Used by hash_table<> for resize()
    //

    extern unsigned long next_prime(unsigned long last);

    //
    //  Implementation of hash_table::iterator
    //

    template <class T, class Traits>
    inline hash_table<T, Traits>::iterator::iterator(
        hash_table<T, Traits>* table)
        : _table(table)
        , _index(0)
        , _item(0)
    {
        if (_table && _table->_tableSize)
            this->operator++();
    }

    template <class T, class Traits>
    inline hash_table<T, Traits>::iterator::iterator(
        hash_table<T, Traits>& table)
        : _table(&table)
        , _index(0)
        , _item(0)
    {
        if (_table->_tableSize)
            this->operator++();
    }

    template <class T, class Traits>
    void hash_table<T, Traits>::iterator::operator++()
    {
        if (_item)
            if (!(_item = _item->next()))
                _index++;

        if (!_item && _table)
        {
            for (; _index < _table->_tableSize && !_table->_table[_index];
                 _index++)
                ;
            if (_index < _table->_tableSize)
                _item = _table->_table[_index];
        }
    }

    template <class T, class Traits>
    inline bool
    hash_table<T, Traits>::iterator::operator==(const iterator& i) const
    {
        return bool(*this) == bool(i);
    }

    template <class T, class Traits>
    inline bool
    hash_table<T, Traits>::iterator::operator!=(const iterator& i) const
    {
        return bool(*this) != bool(i);
    }

    //
    //  Implementation of hash_table
    //

    template <class T, class Traits>
    hash_table<T, Traits>::item::item(const T& thing)
        : _data(thing)
        , _next(0)
    {
    }

    template <class T, class Traits>
    hash_table<T, Traits>::hash_table()
        : _table(0)
        , _tableSize(0)
        , _size(0)
    {
        resize();
    }

    template <class T, class Traits> hash_table<T, Traits>::~hash_table()
    {
        for (int i = 0; i < _tableSize; i++)
        {
            while (item* item = _table[i])
            {
                _table[i] = item->_next;
                delete item;
            }
        }

        delete[] _table;
    }

    template <class T, class Traits>
    const typename hash_table<T, Traits>::item*
    hash_table<T, Traits>::insert(const T& thing)
    {
        if (const item* item = find(thing))
            return item;
        if (_size++ >= _tableSize)
            resize();
        return add_item(new item(thing));
    }

    template <class T, class Traits>
    const typename hash_table<T, Traits>::item*
    hash_table<T, Traits>::add_item(typename hash_table<T, Traits>::item* item)
    {
        hash_value hashIndex = Traits::hash(item->data()) % _tableSize;
        item->_next = _table[hashIndex];
        _table[hashIndex] = item;
        return item;
    }

    template <class T, class Traits>
    const typename hash_table<T, Traits>::item*
    hash_table<T, Traits>::find(const T& thing) const
    {
        hash_value hashIndex = Traits::hash(thing) % _tableSize;

        for (item* item = _table[hashIndex]; item; item = item->_next)
            if (Traits::equals(item->data(), thing))
                return item;
        return 0;
    }

    template <class T, class Traits>
    const typename hash_table<T, Traits>::item*
    hash_table<T, Traits>::first_item_of(
        typename hash_table<T, Traits>::hash_value h) const
    {
        return _table[h % _tableSize];
    }

    template <class T, class Traits> void hash_table<T, Traits>::resize()
    {
        hash_value oldPrime = _tableSize;
        item** oldTable = _table;
        item* nextitem;

        _tableSize = next_prime(oldPrime);
        _table = new item*[_tableSize];

        for (int i = 0; i < _tableSize; i++)
            _table[i] = 0;

        for (int i = 0; i < oldPrime; i++)
        {
            item* item = oldTable[i];

            for (item* item = oldTable[i]; item;)
            {
                nextitem = item->_next;
                add_item(item);
                item = nextitem;
            }
        }

        if (oldTable)
            delete[] oldTable;
    }

} // namespace stl_ext

#endif // __stl_ext__hash_table__h__
