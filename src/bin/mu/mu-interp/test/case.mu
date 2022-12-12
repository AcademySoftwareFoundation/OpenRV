//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

union: Foo 
{ 
      A (int,Foo) 
    | B float 
    | None 
}

use Foo;

\: testFoo (string; Foo x)
{
    case (x)
    {
        None                -> { return("NONE"); }
        A (321, None)       -> { return("321 with a None"); }
        A (321, _)          -> { return("321!"); }
        A (_, None)         -> { return("A with a None"); }
        A (_, B x)          -> { return("A with a B x = %f" % x); }
        A (_, A (_, B x))   -> { return("A with an A with a B x = %f" % x); }
        A x                 -> { return("A x"); }
        B 1.0               -> { return("B 1.0"); }
        B 1.123             -> { return("B 1.123"); }
        B x                 -> { return("B x = %s" % x); }
    }
}

let testValues = [ (B(1.123), "B 1.123"),
                   (B(1.12311), "B x = 1.12311"),
                   (A((32,B(3.333))), "A with a B x = 3.333000"),
                   (A((32,A((0, B(3.333))))), "A with an A with a B x = 3.333000"),
                   (A((321,None)), "321 with a None"),
                   (A(1,A(1,A(1,None))), "A x"),
                   (None, "NONE") ];

for_each (v; testValues)
{
    let (x, s) = v;
    assert(testFoo(x) == s);
}


let q = "three";

\: stringTest (string; string q)
{
    case (q)
    {
        "one" -> { return("1"); }
        "two" -> { return("2"); }
        t -> { return("value = %s" % t); }
    }
}

\: intTest (string; int x)
{
    case (x)
    {
        -1 -> { return("minus one"); }
        0 -> { return("zero"); }
        1 -> { return("one"); }
        2 -> { return("two"); }
        _ -> { return("whatever"); }
    }

    return "bad";
}

assert(intTest(-1) == "minus one");
assert(intTest(0) == "zero");
assert(intTest(1) == "one");
assert(intTest(2) == "two");
assert(intTest(3) == "whatever");
assert(intTest(-3) == "whatever");
assert(stringTest("one") == "1");
assert(stringTest("two") == "2");
assert(stringTest("three") == "value = three");
