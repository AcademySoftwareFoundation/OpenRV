//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//


documentation: bar 
"Here's some global variable
Does it work:?";

global float bar = 1.0;

//----------------------------------------------------------------------
documentation: 
"Function documentation could live right here. It could be
long enough that you get a feeling for how the comment would
appear in real source code. Or maybe not:

    May be there will be intendation?

Maybe there will be none. I don't know";

documentation: a
"A is the name of some parameter taken by foo";

\: foo (float; float a) 
{ 
    return math.pow(a * a + a / a, 1.0 / a); 
}

documentation:
"Baz is a function.
That's documented here.";

\: baz (float; float b)
{
    b + b;
}

documentation: add
"Add is (+)";

documentation: a
"A is the first number to add.";

documentation: b
"B is the second number to Add";

\: add (int; int a, int b ) { a + b; }
require autodoc;
print("BAR: \n" + autodoc.document_symbol("bar"));
print("FOO: \n" + autodoc.document_symbol("foo"));
print("FOO.a: \n" + autodoc.document_symbol("foo.a"));
print("BAZ: \n" + autodoc.document_symbol("baz"));
print("ADD: \n" + autodoc.document_symbol("add"));
print("ADD.a: \n" + autodoc.document_symbol("add.a"));
print("ADD.b: \n" + autodoc.document_symbol("add.b"));


\: f1 (int; int a) { return a; }
\: f1 (int; int a, int b) { return a + b; }
\: f2 (int; int a) { return a; }

class: A
{
    class: B
    {
        int b;
    }

    int a;
}

// a documentation module
documentation: {

A.B "some A.B docs"
f1.a "blah blah blah"
f1 (int;int) "f1 docs"
f1 (int;int,int) "f1#2 docs"
f2 "f2 docs"


}


print("-------f1: \n" + autodoc.document_symbol("f1"));
print("-------f1.a: \n" + autodoc.document_symbol("f1.a"));
print("-------f2: \n" + autodoc.document_symbol("f2"));
print("-------B: \n" + autodoc.document_symbol("A.B"));
