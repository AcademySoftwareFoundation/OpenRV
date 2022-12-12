//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use test;

base_class base = a_class("BASE");
a_class a = base;

try
{
    b_class b = base;
}
catch (exception exc)
{
    print("CAUGHT: " + string(exc) + "\n");
}
