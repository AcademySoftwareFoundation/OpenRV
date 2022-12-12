//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
string foo = nil;
assert(foo eq nil);
print(foo); print("\n");
print(foo + "\n");

foo = "foo bar";

try
{
    foo.split(nil);
}
catch (exception e)
{
    //
    // should be nil argument exception
    //

    print(string(e) + "\n");
}

foo = nil;

try
{
    foo.split(" ");
}
catch (exception e)
{
    //
    // should be nil argument exception also
    //

    print(string(e) + "\n");
}
