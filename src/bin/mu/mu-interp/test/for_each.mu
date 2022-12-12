//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

int[] iarray = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

string[] sarray = {"one", "two", "three",
                   "four", "five", "six", 
                   "seven", "eight", "nine", 
                   "ten"};

float[4,4] M = {1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1};


float[][] farray = { {1,2,3}, {4,5,6}, {7,8,9}, {10,10,10} };

for_each (i; iarray) print(i + string(" "));
print("\n");

int count = 1;
for_each (i; iarray) assert(i == count++);
assert(count == iarray.size() + 1);

for_each (i; sarray) print(i + string(" "));
print("\n");

for_each (i; M) print(i + string(" "));
print("\n");


for_each (i; farray) 
{
    print(i.back() + string(" "));
}

for_each (i; iarray)
{
    if (i == 5) break;
    assert(i < 5);
}

bool foundSix = false;

print("\n");
print("iarray = %s\n" % iarray);
for_each (i; iarray)
{
    if (i == 5) { print(" i=5 "); continue; }
    print(" %d" % i);
    //assert(i > 5 || i < 5);
    if (i == 6) foundSix = true;
}

assert(foundSix);

print("\n");


