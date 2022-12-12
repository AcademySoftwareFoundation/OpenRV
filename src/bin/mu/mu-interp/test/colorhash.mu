//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use color_hash_table;

HashTable table = HashTable(10); // initial expected size (can be approx)

//
//  Look up some specific colors and make sure the values match
//

\: lookupValid (void; (Color,string)[] colors, HashTable table)
{
    for_each (c; colors) 
    {
        let (color, value) = c,
            v = table.find(color);

        assert(v == value);
        //print("%s => %s\n" % (color, v));
    }
}

//
//  Some basic colors and names. 
//

let colors = (Color,string)[] 
{ 
    (Color(1,0,0), "red"),
    (Color(0,1,0), "green"),
    (Color(0,0,1), "blue"),
    (Color(0,1,1), "cyan"),
    (Color(1,0,1), "magenta"),
    (Color(1,1,0), "yellow") 
};

//
//  Add our colors and test to see if they are found properly
//

for_each (c; colors) table.add(c._0, c._1);
lookupValid(colors, table);

//
//  Add 20000 random entries. Adding can be a bit slow since the table
//  has to be rebuilt each time the load becomes too large. In this
//  case the table will be reconstructed 10 times before the loop is
//  complete.
//

repeat (20000)
{
    use math_util;
    let c = Color(random(1.0), random(1.0), random(1.0)),
        v = string(c);

    table.add(c, v);
}

//
//  Test our colors again. The lookup should be fast.
//

//print("-\n");
lookupValid(colors, table);
