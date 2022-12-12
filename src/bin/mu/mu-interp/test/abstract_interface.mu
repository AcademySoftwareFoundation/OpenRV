//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

interface: Sized
{
  function: size (int; Sized);
  function: bar (void; Sized);
}

try
{
    //
    //  This dynamic cast should throw. int[] does not properly
    //  implement the Sized interface because function bar() is
    //  abstract.
    //

    //
    //  NOTE: if you make the int[] a constant (replace i with 1
    //  for example). It will throw at parse time not at runtime.
    //  hence the use of "i" to force it at runtime.
    //

    int i = 1; 
    Sized foo = int[] {i, 2, 3};
    assert(false);
}
catch (exception exc)
{
    print("ok\n");
}
