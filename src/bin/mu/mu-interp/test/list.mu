//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
Vec := vector float[3];

\: test (void;)
{
    let a = [int] {1, 2, 3, 4, 5, 6},
        b = cons(-3, cons(-2, cons(-1, a))),
        c = [1,2,3,4],
        d = [a, c, b],
        f = [[1,2], [3,4,5]],
        g = [[[1],[2]], [[3],[4],[5]]];
            

    let [q,r,s,t] = c;
    let [[a0, a1], [b0, b1, b2]] = f;

    assert(q == 1);
    assert(r == 2);
    assert(s == 3);
    assert(t == 4);

    assert(a0 == 1 &&
           a1 == 2 &&
           b0 == 3 &&
           b1 == 4 &&
           b2 == 5);

    for_each (i; a) print(i);

    assert(string(a) == "[1, 2, 3, 4, 5, 6]");
    assert(string(b) == "[-3, -2, -1, 1, 2, 3, 4, 5, 6]");
    assert(string(c) == "[1, 2, 3, 4]");
    assert(string(d) == "[[1, 2, 3, 4, 5, 6], [1, 2, 3, 4], [-3, -2, -1, 1, 2, 3, 4, 5, 6]]");
    assert(string(f) == "[[1, 2], [3, 4, 5]]");
    assert(string(g) == "[[[1], [2]], [[3], [4], [5]]]");

    print("a = %s\n" % a);
    print("b = %s\n" % b);
    print("c = %s\n" % c);
    print("d = %s\n" % d);
    print("f = %s\n" % f);
    print("g = %s\n" % g);
    print("tail(a) = %s\n" % tail(a));
    print("head(a) = %d\n" % head(a));
    print("%s\n" % cons(1,nil));

    let v = [Vec(1), Vec(2), Vec(3)];

    [int] l;
    for (int i=0; i < 1000000; i++) l = i : l;

    //
    //  This is to make sure the grammar allows this type of
    //  expression. The list literal has to be a primary_expression
    //  for this type of thing to work.
    //

    operator: + ([int]; [int] list, (int,int,int) tuple) 
    { 
        let [a,b,c] = list,
            (x,y,z) = tuple;

        return [a+x, b+y, c+z];
    }

    {
        let [a,b,c] = [1,2,3] + (1,2,3);
        assert(a == 2);
        assert(b == 4);
        assert(c == 6);
    }
}

test();
