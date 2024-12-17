#ifndef __Mu__HashTable__h__
#define __Mu__HashTable__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
// #include <unistd.h>
#include <iostream>

namespace Mu
{

    //
    //	class HashTable<T,Traits>
    //
    //	Implements a generic hash table. The Traits object type must
    //	have the following functions defined:
    //
    //	    bool Traits::equals(const T&, const T&)
    //	    HashTable<T>::HashValue Traits::hash(const T&);
    //
    //	The value type "T" must have the following defined:
    //
    //	    Publicly destructable
    //	    Default copy constructor
    //	    Assignable
    //	    Comparable with operator==
    //

    template <class T, class Traits> class HashTable
    {
    public:
        MU_GC_NEW_DELETE

        typedef HashTable<T, Traits> ThisType;
        typedef unsigned long HashValue;

        HashTable();
        ~HashTable();

        //
        //	An item is part of a singly linked list.
        //

        class Item
        {
        public:
            MU_GC_NEW_DELETE
            friend class HashTable<T, Traits>;
            Item(const T& data);

            const T& data() const { return _data; }

            const Item* next() const { return _next; }

            Item* next() { return _next; }

        private:
            ~Item() {}

            T _data;
            Item* _next;
        };

        const Item* add(const T&);
        const Item* find(const T&) const;
        const Item* firstItemOf(HashValue) const;

        //
        //	Iterator over the items in a HashTable
        //
        //	for (Iterator i(table); i; ++i)
        //	    doSomething(*i);
        //

        class Iterator
        {
        public:
            MU_GC_NEW_DELETE
            Iterator(const HashTable<T, Traits>*);
            Iterator(const HashTable<T, Traits>&);

            operator bool() const { return _item ? true : false; }

            void operator++();

            const T& operator*() const { return _item->data(); }

            int index() const { return _index; }

        private:
            const ThisType* _table;
            Item* _item;
            size_t _index;
        };

    private:
        void resize();
        const Item* addItem(Item*);
        friend class Iterator;

    private:
        unsigned int _size;
        unsigned int _tableSize;
        Item** _table;
    };

    //
    //  Used by HashTable<> for resize()
    //

    extern unsigned long nextPrime(unsigned long last);

    //
    //  Implementation of HashTable::Iterator
    //

    template <class T, class Traits>
    inline HashTable<T, Traits>::Iterator::Iterator(
        const HashTable<T, Traits>* table)
        : _table(table)
        , _index(0)
        , _item(0)
    {
        if (_table->_tableSize)
            this->operator++();
    }

    template <class T, class Traits>
    inline HashTable<T, Traits>::Iterator::Iterator(
        const HashTable<T, Traits>& table)
        : _table(&table)
        , _index(0)
        , _item(0)
    {
        if (_table->_tableSize)
            this->operator++();
    }

    template <class T, class Traits>
    void HashTable<T, Traits>::Iterator::operator++()
    {
        if (_item)
            if (!(_item = _item->next()))
                _index++;

        if (!_item)
        {
            for (; _index < _table->_tableSize && !_table->_table[_index];
                 _index++)
                ;
            if (_index < _table->_tableSize)
                _item = _table->_table[_index];
        }
    }

    //
    //  Implementation of HashTable
    //

    template <class T, class Traits>
    HashTable<T, Traits>::Item::Item(const T& thing)
        : _data(thing)
        , _next(0)
    {
    }

    template <class T, class Traits>
    HashTable<T, Traits>::HashTable()
        : _table(0)
        , _tableSize(0)
        , _size(0)
    {
        resize();
    }

    template <class T, class Traits> HashTable<T, Traits>::~HashTable()
    {
        for (int i = 0; i < _tableSize; i++)
        {
            while (Item* item = _table[i])
            {
                _table[i] = item->_next;
                delete item;
            }
        }

#ifdef MU_USE_BOEHM_COLLECTOR
        _table = 0;
        // nothing
#endif

#if defined(MU_USE_BASE_COLLECTOR) || defined(MU_USE_NO_COLLECTOR)
        delete[] _table;
#endif
    }

    template <class T, class Traits>
    const typename HashTable<T, Traits>::Item*
    HashTable<T, Traits>::add(const T& thing)
    {
        if (const Item* item = find(thing))
            return item;
        if (_size++ >= _tableSize)
            resize();
        return addItem(new Item(thing));
    }

    template <class T, class Traits>
    const typename HashTable<T, Traits>::Item*
    HashTable<T, Traits>::addItem(typename HashTable<T, Traits>::Item* item)
    {
        HashValue hashIndex = Traits::hash(item->data()) % _tableSize;
        item->_next = _table[hashIndex];
        _table[hashIndex] = item;
        return item;
    }

    template <class T, class Traits>
    const typename HashTable<T, Traits>::Item*
    HashTable<T, Traits>::find(const T& thing) const
    {
        HashValue hashIndex = Traits::hash(thing) % _tableSize;

        for (Item* item = _table[hashIndex]; item; item = item->_next)
            if (Traits::equals(item->data(), thing))
                return item;
        return 0;
    }

    template <class T, class Traits>
    const typename HashTable<T, Traits>::Item*
    HashTable<T, Traits>::firstItemOf(
        typename HashTable<T, Traits>::HashValue h) const
    {
        return _table[h % _tableSize];
    }

    template <class T, class Traits> void HashTable<T, Traits>::resize()
    {
        HashValue oldPrime = _tableSize;
        Item** oldTable = _table;

        _tableSize = nextPrime(oldPrime);
#ifdef MU_USE_BOEHM_COLLECTOR
        _table = (Item**)GC_MALLOC(_tableSize * sizeof(Item*));
#endif

#if defined(MU_USE_BASE_COLLECTOR) || defined(MU_USE_NO_COLLECTOR)
        _table = new Item*[_tableSize];
#endif

        for (int i = 0; i < _tableSize; i++)
            _table[i] = 0;

        for (int i = 0; i < oldPrime; i++)
        {
            for (Item *item = oldTable[i], *nextItem; item;)
            {
                nextItem = item->_next;
                addItem(item);
                item = nextItem;
            }
        }

#if defined(MU_USE_BASE_COLLECTOR) || defined(MU_USE_NO_COLLECTOR)
        if (oldTable)
            delete[] oldTable;
#endif
    }

} // namespace Mu

#endif // __Mu__HashTable__h__
