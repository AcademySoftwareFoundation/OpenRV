//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Supply the constructors that normally are automatically
//  generated. Make sure that the NodeAssembler deals with this
//  situation in a sane way.
//

class: Foo
{
    method: Foo (Foo; int _a, int _b) { a=_a; b=_b; }
    method: Foo (Foo;) { a = 0; b = 0; }
    int a;
    int b;
}

class: Bar
{
    int a;
    int b;
}

class: FooDerived : Foo
{
    method: FooDerived (FooDerived; int _a, int _b)
    {
        // call base class constructor explicitly
        Foo.Foo(this, _a, _b);
    }
}

let a = Foo(1,2),
    b = Foo(),
    c = Bar(1,2),
    d = Bar();

assert(FooDerived(9,8).b == 8);
