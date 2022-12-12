//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
\: add5 (int; int a, int b) { a + b; }

class: X
{
    method: f (int;) { this.g(); }
    method: g (int;) { add(911, 0); }
}

class: Y
{
    method: pr(void;) { for_each (i; ints()) print ("%s\n" % i); }
    method: ints(int[];) { int[] {1,2,3}; }
}

class: Z
{
    method: Z (Z; int _a) { a=_a; }
    int a;
}

class: Base
{
    method: f (int; string a) { return int(a); }
    method: f (int; int a, int b) { this.g(a) + b; }
    method: g (int; int a) { a; }
    method: p ((int;int);) { g; }
}

class: Derived : Base
{
    method: f (int; int a, int b) { g(a) - b; }
    method: g (int; int a) { a + 888; }
    method: p ((int;int);) { f(7,); }
}

\: bdtest (void;)
{
    Base b = Derived();
    let F = b.p();
    F(77);
}

bdtest();  


{
    class: foo
    {
        [int] list;

        method: F (void;) { list = G() : list; }
        method: G (int;) { 1; }
    }

    let f = foo();
    f.F();
    f.F();
    //print("%s\n" % f);
    // should be foo {[1,1]}
}


// not working yet
// let q = add(1,2);

// not yet
// \: F (int;) 
// { 
//     let (a,b,c) = tadd(1,2); 
//     c;
// }

\: tadd ((int,int,int); int a, int b) { (a,b,add(a,b)); }

\: add (int; int a, int b) { add1(a, b); }
\: add1 (int; int a, int b) { add2(a, b); }
\: add3 (int; int a, int b) { add4(a, b); }
\: add2 (int; int a, int b) { add3(a, b); }
\: add4 (int; int a, int b) { add5(a, b); }


// not yet (try this one first)
//assert(add(1,2) == F());
assert(add(1,2) == 3);
assert(X().f() == 911);

\: foo (int; int a)
{
    let x = bar(a, 10),
        y = float(bar(a, 11));
    x + y;
}

\: bar (int; int a, int b) { a + b; }

assert(foo(1) == 23);

\: F_each (void;)
{
    for_each (i; listtest());
}

\: F_index (void;)
{
    for_each (i; arraytest());
}

\: listtest ([int];) { [l1(), l2(), l3()]; }
\: tupletest ((int, int, int);) { (l1(), l2(), l3()); }
\: arraytest (int[];) { int[] {1,2,3}; }

// crashes:
// \: tupletest () { (l1(), l2(), l3()); }

\: l1 (int;) { 1; }
\: l2 (int;) { 2; }
\: l3 (int;) { 3; }

{
    let [a, b, c] = listtest(),
        (d, e, f) = tupletest();
    assert(a == 1 && a == d);
    assert(b == 2 && b == e);
    assert(c == 3 && c == f);
}

//
//  Method short cut. set() is called as Foo.set(this, n).  
//

class: Foo 
{
  method: Foo (Foo; Foo n) { set(n); }
  method: set (void; Foo n) { next = n; }

  Foo next;
};

Foo(Foo(nil));


class: Rec
{
    Rec _next;

    method: next (Rec;) 
    { 
        if (_next eq nil) _next = Rec(Rec(nil));
        return _next; 
    }
}

