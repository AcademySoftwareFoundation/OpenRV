//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
int i = 69;

for (int i=0; i<10; i+=1)
{
    assert(i >= 0 && i < 10);
    print(i);
}

assert(i == 69);

while (i>60)
{
    print(i);
    assert(i > 60);
    i = i - 1;
}

assert(i == 60);

do
{
    i += 1;
    print(i);
    assert(i <= 69);
} while (i < 69);

assert(i == 69);
