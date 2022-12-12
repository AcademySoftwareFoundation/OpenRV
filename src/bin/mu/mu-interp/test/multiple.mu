//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Multiple inheritance
//

class: Bprime { int bp; }

class: A { int a; }
class: B : Bprime { int b; int bb; }
class: C { int c; int cc;}
class: D : A, B, C { int d; }

class: E { int e; }
class: F { int f; }
class: G { int g; }
class: H : E, F, G { int h; }

class: J : H, D { int j; }

union: JU { JUnion J };
use JU;

\: foo ()
{
    //
    // Make this hard by putting the class instance inside a union. The
    // casting will need to sort this out
    //

    JU ju = JUnion(11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22);

    let JUnion j = ju;  // unpack the class instance

    
    H h = j;        print("h  => %s\n" % h);     // "upcast"
    D d = j;        print("d  => %s\n" % d);     // "upcast" to a D
    B b = j;        print("b  => %s\n" % b);     // "upcast" to a B
    D nd = b;       print("nd => %s\n" % nd);    // "downcast" back to D
    J nj = b;       print("nj => %s\n" % nj);    // "downcast" back to J
    // NOT ALLOWED: G g  = b;       print("g = %s\n"  % h);     // "overcast" to G
    G g = nj;       print("g  => %s\n"  % g);     // "upcast" back to G

    assert(h.h == 12);
    assert(d.d == 17);
    assert(b.b == 19);
    assert(b.bp == 18);
    assert(g.g == 14);
}

class: Ae : A { ; }
class: Be : B { ; }
class: Ce : C { ; }
class: De : D { ; }
class: Ee : E { ; }
class: Fe : F { ; }
class: Ge : Ae, Be, Ce, De, Ee, Fe { ; }

\: bar ()
{
    Ae ae = Ge();
    Ge ge = ae;
    Be be = ge;
    Fe fe = ge;
    be;
}

foo();
bar();
