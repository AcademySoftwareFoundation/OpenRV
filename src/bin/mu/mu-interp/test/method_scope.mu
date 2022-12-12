//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

class: X
{
    method: f (int;) { this.g(); }
    method: g (int;) { 911; }
}

class: Base
{
    int q;

    method: Base (Base;) 
    { 
        q = 9;
        x = 99.1; // will require a cast

        // BELOW fails, "x" and "q" are a free variables which should
        // resolve (eventually) to the fields. In the first case it
        // fails to even see "x". In the second "q" is seen, but can't
        // be passed in for some reason.

        //Q = \: (int;int a) { a + x; };
        //Q = \: (int;int a) { a + q; };

        // should fail
        //Q = \: (float; int a) { a + 1; };

        Q = \: (int; int a) { a + 1; };
    }

    \: ff (void; Base B, int a) { ; }
    \: ff (void; Base B, int a, int b) { ; }

    method: f (int; int a, int b) { this.g(a) + b; }
    method: g (int; int a) { a; }
    method: p ((int;int);) { g; }

    (int;int) Q;
    int x;
}

class: Derived : Base
{
    method: Derived (Derived;) 
    { 
        x = 999; 
        y = 888; 
        Q = \: (int; int a) { a * 2; }; 
    }

    \: ff (void; Derived d, string a) { ; }

    method: f (int; int a, int b) { this.g(a) - b; }
    method: g (int; int a) { a + y; }
    method: p ((int;int);) { f(7,); }

    int y;
}

\: foo (void;)
{
    Base b = Derived();
    
    let G = b.g,
        F = b.f(1,),
        BaseG = Base.g,
        DervG = Derived.g;
    
    assert(G(123) == 888 + 123);
    assert(F(2) == 1 + 888 - 2);
    assert(Base.g(b, 2) == 2);
    assert(b.Q(321) == 321 * 2);
    assert(b.f(2,3) == (2+888) - 3);
    assert(BaseG(b, 1) == 1);
    assert(DervG(b, 1) == 1 + 888);
    assert(b.p()(77) == 7 + 888 - 77);
    assert(Base.p(b)(77) == b.g(77));
    assert((X().f)() == 911);

    Derived().ff("a");
    Derived().ff(1,2);
}

foo();  

