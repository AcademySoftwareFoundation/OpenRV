//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

int i;

for (i=0; i<100; i++)
{
    if (i == 10) break;
    assert(i != 10);
}

assert(i == 10);

for (i=0; i<100; i++)
{
    continue;
    assert(false);
}

assert(i == 100);

while (i != 0)
{
    i--;
    if (i == 10) break;
}

assert(i == 10);

while (i != 100)
{
    i++;
    continue;
    assert(false);
}

assert(i == 100);

do
{
    if (i == 10) break;
    i--;
} while(i > 0);

assert(i == 10);

do
{
    i++;
    continue;
    assert(false);
} while(i < 100);

