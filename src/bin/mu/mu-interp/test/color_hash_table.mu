//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: color_hash_table
{
    //
    //  NOTE: we're defining color the same way rvui and gl modules
    //  do: an r g b triple + alpha. 
    //

    Color := vector float[4];

    global int[] primes = 
        { 7, 13, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
          49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469, 
          12582917, 25165843 };

    \: nextPrime (int; int p)
    {
        for_index (i; primes) if (primes[i] > p) return primes[i];
        return primes.back();
    }

    \: hash (Color c) { string(c).hash(); }

    documentation: """
    A basic hash map which uses a color (R, G, B) as the index. The hash
    is created by converting to a string and using the string.hash() function.
    """;

    class: HashTable
    {
        class: Item
        {
            Color  color;
            string value;
            Item   next;
        }

        Item[] _table;
        int _numItems;

        method: _addInternal (void; Color color, string value)
        {
            let i = hash(color) % _table.size();
            _table[i] = Item(color, value, _table[i]);
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
                    _addInternal(i.color, i.value);
                }
            }
        }

        method: HashTable (HashTable; int initialSize)
        {
            _table = Item[]();
            _table.resize(nextPrime(initialSize));
            resize();
        }

        method: add (void; Color color, string value)
        {
            _addInternal(color, value);
            if (_numItems > _table.size() * 2) resize();
        }

        method: find (string; Color color)
        {
            let i = hash(color) % _table.size();

            for (Item x = _table[i]; x neq nil; x = x.next)
            {
                if (x.color == color) return x.value;
            }

            return nil;
        }
    }
}

