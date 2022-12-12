//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

Point := vector float[3];

union: Weekday { Monday | Tuesday | Wednesday | Thursday | Friday }

union: NumberList { ListOfFloat [float] |
                    ListOfInt [int] |
                    ListOfShort [short] }

union: Tree 
{ 
    class: MidPoint { float value; int dimension; }
    class: NodeValue { MidPoint p; Tree left; Tree right; }
    
    Leaf Point | Node NodeValue
}

union: IntList 
{   
    Empty | Cons (int, IntList) 
}


union: Foo
{
    class: Bar { Foo a; Bar b; int x; float y; vector float[3] p; }

      A int
    | B float
    | C vector float[3]
    | D vector float[2]
    | E vector float[4]
    | F (void;Foo,Foo)
    | G (int,float,int)
    | H Bar
    | I (Bar, Foo)
    | J int[]
} 

union: StringIntMap
{
    Empty | MapEntry(string, StringIntMap, StringIntMap)
}

//----------------------------------------------------------------------

\: testTree (void;)
{
    use Tree;

    Tree tree = Node(MidPoint(1.0, 3), Leaf(), Leaf(1,2,3));
    Tree leaf = Leaf(1,2,3);

    let n                   = Node(MidPoint(1.0, 2), Leaf(1,2,3), Leaf(4,5,6)),
        Node {{a, b}, c, d} = n,
        Leaf q              = d;

    \: doit (void; Tree tree, Tree leaf)
    {
        case (leaf)
        {
            Leaf p -> { assert(p == Point(1,2,3)); }
            Node {{v,d}, left, right} -> { throw "wrong!\n"; }
        }

        case (tree)
        {
            Leaf p -> { throw "wrong"; }

            Node {{v,d}, left, right} ->
            {
                //print("Node ((%f,%d), %s, %s)\n" % (v,d,left,right)); 
                assert(v == 1.0);
                assert(d == 3);
            }
        }
    }

    doit(tree, leaf);
}

\: testEnum (void;)
{
    //
    //  Constructors are in scope of union type. Otherwise "use" 
    //  the union to get its constructors in scope
    //

    \: abbreviation (string; Weekday day)
    {
        case (day)
        {
            //
            //  Inside case statement the expression type is in scope
            //  so no prefix (Weekday.Monday) necessary
            //

            Friday -> { return "fri"; }
            Monday -> { return "mon"; }
            Thursday -> { return "thurs"; }
            Tuesday -> { return "tues"; }
            Wednesday -> { return "wed"; }
        }

        return "bad";
    }

    assert(abbreviation(Weekday.Friday) == "fri");
    assert(abbreviation(Weekday.Thursday) == "thurs");
    assert(abbreviation(Weekday.Wednesday) == "wed");
    assert(abbreviation(Weekday.Tuesday) == "tues");
    assert(abbreviation(Weekday.Monday) == "mon");
}

\: testList (void;)
{
    use IntList;

    IntList list = Empty;

    print("building...");
    for (int i=99999; i >= 0; i--) list = Cons(i, list);
    print("done\n");

    \: length (int; IntList l)
    {
        bool done = false;
        int count = 0;

        while (true)
        {
            case (l)
            {
                Empty       -> { return count; }
                Cons (_,y)  -> { l = y; count++; }
            }
        }

        return 0;
    }

    print("counting...");
    print("length of list is %d\n" % length(list));
}

 
\: testFoo (void;)
{
    use Foo;

    let q = I(Bar(A(1), Bar(), 1, math.pi, (vector float[3])(1,2,3)), A(99));

    case (q)
    {
        C x -> { print("%g, %g, %g\n" % (x.x, x.y, x.z)); }
        
        I ({a, b, x, y, p}, foo) ->
        {
            print("a = %s, b = %s, x = %d, y = %g, z = (%g, %g, %g) *and* foo = %s\n"
                  % (a, b, x, y, p.x, p.y, p.z, foo));
        }
    }
}

\: variantWithList (void;)
{
    use NumberList;

    let ilist = ListOfInt([1,2,3,4]),
        flist = ListOfFloat([1.0, 2.0, 4.0]),
        slist = ListOfShort([short(123)]);

    let ListOfInt one:two:other         = ilist,
        ListOfFloat fone:ftwo:fother    = flist,
        ListOfShort l                   = slist;

    assert(one == 1);
    assert(two == 2);
    assert(head(other) == 3);
    assert(head(tail(other)) == 4);
    assert(fone == 1.0);
    assert(ftwo == 2.0);
    assert(head(fother) == 4.0);
    assert(head(l) == 123);
}

testEnum();
testTree();
testList();
testFoo();
variantWithList();

//require autodoc;
//print(autodoc.document_symbol("IntList"));
