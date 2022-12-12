//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

class: Foo { float a; float b; };

\: foo (float;)
{
    let list = [1,2,3,4];
    bool failed = false;

    try
    {
        let [q,r,s,t,y] = list;
    }
    catch (exception exc)
    {
        print("GOOD: it threw: %s\n" % string(exc));
        failed = true;
    }

    assert(failed == true);

    let (a, ({b, c}, d))    = (1, (Foo(float(math.pi), float(math.e)), 2)),
        {x,z}               = Foo(3.321,4.321),
        [l0, l1, l2, l3]    = list,
        [la, lb, lc]        = list,
        q:s:r:t             = list;

    print("q=%s, s=%s, r=%s, t=%s\n" % (q,s,r,t));
    let tt:qq = t;

    assert(tt == 4);

    b;
}

\: bar (void;)
{
    let ({a,b},c):[_,({e,f},_)] = [(Foo(1.0,2.0),(3.1,3.2)),
                                   (Foo(4.0,5.0),(6.1,6.2)),
                                   (Foo(4.9,5.9),(6.12,6.22))];

    assert(a == 1.0 &&
           b == 2.0 &&
           c._0 == 3.1 &&
           c._1 == 3.2);

    assert(e == 4.9);
    assert(f == 5.9);
}

print("foo = %s\n"  % foo());
assert(foo() == float(math.pi));
bar();
