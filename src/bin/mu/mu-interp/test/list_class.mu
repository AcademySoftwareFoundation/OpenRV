//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
Value := int;

documentation: 
"An STL list<> like class.
functions allow self-updating (not a functional list).";

class: List
{
    documentation:
    "Cell class holds individual list items.";

    class: Cell 
    { 
        Value value; 
        Cell next; 
    }

    Cell start, end;

    method: List (List; Value v) 
    { 
        start = List.Cell(v, nil); 
        end = start;
    }

    function: size (int; List this)
    {
        int n = 0;
        for (Cell c = start; !(c eq nil); c = c.next) n++;
        n;
    }

    function: size_recursive (int; List this)
    {
        function: f (int; Cell c, int n) { if c eq nil then n else f(c.next, n+1); }
        f(this.start, 0);
    }

    function: push_back (void; List this, Value v)
    {
        end.next = Cell(v, nil);
        end = end.next;
    }

    function: push_back (void; List this, List other)
    {
        end.next = other.start;
        end = other.end;
    }

    function: push_front (void; List this, Value v)
    {
        Cell c = Cell(v, start);
        start = c;
    }

    function: push_front (void; List this, List other)
    {
        other.end.next = start;
        start = other.start;
    }

    function: front (Value; List this) { start.value; }
    function: back (Value; List this) { end.value; }

    function: pop_front (Value; List this)
    {
        Cell c = start;
        if (!(c eq nil)) start = start.next;
        c.value;
    }

    operator: [] (Value; List this, int index)
    {
        function: f (Value; Cell c, int i, int index) 
        { 
            if (c eq nil)
            {
                throw "bad";
            }
            else
            {
                return if i == index then c.value else f(c.next, i+1, index);
            }
        }

        f(start, 0, index);
    }
}


List l1 = List(1);
for (int i=2; i < 10; i++) l1.push_back(i);

List l2 = List(100);
for (int i=101; i < 200; i++) l2.push_back(i);

l1.push_back(l2);
assert(l1.size() == l1.size_recursive());
assert(l1[8] == 9);

try
{
    l1[500];
    assert(false);
}
catch (...)
{
    print("caught it\n");
}

