//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
require runtime;

function: foo ([string]; int depth)
{
    if (depth == 4) 
    {
        return runtime.backtrace();
    }
    else 
    {
        return foo(depth+1);
    }
}

[string] stuff = foo(0);

if (stuff eq nil)
{
    print("NO BACKTRACE\n");
}
else
{
    int i = 0;
    
    for_each (frame; stuff)
    {
        i++;
        print("%s\n" % frame);
    }
}


