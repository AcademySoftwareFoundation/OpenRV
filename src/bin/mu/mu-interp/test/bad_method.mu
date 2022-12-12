//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

try
{
    string o = nil;
    o.size();
}
catch (exception e)
{
    print("caught: ");
    print(e);
    print("\n");
}
