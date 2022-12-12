//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use commands;

module: property_hash_table
{
    global int[] primes = 
        { 7, 13, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
          49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469, 
          12582917, 25165843 };

    \: nextPrime (int; int p)
    {
        for_index (i; primes) if (primes[i] > p) return primes[i];
        return primes.back();
    }

    KeyType := string;
    ValueType := string[];

    documentation: """
    A basic hash map from string -> string[]
    """;

    class: HashTable
    {
        class: Item
        {
            KeyType     name;
            ValueType   value;
            Item        next;
        }

        Item[] _table;
        int _numItems;

        method: hash (KeyType name) { name.hash(); }

        method: _addInternal (void; KeyType name, ValueType value)
        {
            let i = hash(name) % _table.size();
            _table[i] = Item(name, value, _table[i]);
            _numItems++;
        }

        method: resize (void;)
        {
            let newSize = nextPrime(_table.size()),
                oldTable = _table;

            _table = Item[]();
            _table.resize(newSize);

            for_each (item; oldTable)
            {
                for (Item i = item; i neq nil; i = i.next)
                {
                    _addInternal(i.name, i.value);
                }
            }
        }

        method: HashTable (HashTable; int initialSize)
        {
            _table = Item[]();
            _table.resize(nextPrime(initialSize));
            resize();
        }

        method: add (void; KeyType name, ValueType value)
        {
            _addInternal(name, value);
            if (_numItems > _table.size() * 2) resize();
        }

        method: allItems (Item[];)
        {
            Item[] items;

            for_each (item; _table)
            {
                while (item neq nil)
                {
                    items.push_back(item);
                    item = item.next;
                }
            }

            return items;
        }

        method: keys (string[];)
        {
            string[] array; 
            for_each (i; allItems()) array.push_back(i.name);
            array;
        }

        method: find (ValueType; KeyType name)
        {
            let i = hash(name) % _table.size();

            for (Item x = _table[i]; x neq nil; x = x.next)
            {
                if (x.name == name) return x.value;
            }

            return nil;
        }
    }

    \: propertyHashTableFromNode (HashTable; string nodeNameOrType)
    {
        let h = HashTable(8);

        for_each (prop; properties(nodeNameOrType))
        {
            let parts    = prop.split("."),
                nodeName = parts[0],
                compName = parts[1],
                propName = parts[2];

            ValueType array = h.find(compName);

            if (array eq nil) 
            { 
                array = ValueType();
                h.add(compName, array);
            }

            array.push_back(propName);
        }

        return h;
    }
}

