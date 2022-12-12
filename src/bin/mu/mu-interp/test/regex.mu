//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

\: show (void; string[] pieces) { print("%s\n" % pieces); }

regex r = "[ab]0";
print("Regular expression is: " + string(r) + "\n");

assert(regex.match(r, "a0") && regex.match(r, "b0"));
assert(regex.match("bar", "foobar"));
assert(!regex.match("yellowberry", "blue"));

r = regex("A[1-9]", regex.Extended | regex.IgnoreCase);
assert(regex.match(r, "a8") && regex.match(r, "A8"));
assert(r.match("a8") && r.match("A8")); // alternate calling style

// US telephone number
r = regex("(([0-9]{3})[-.])?([0-9]{3})[-.]([0-9]{4})");

assert( int(regex.smatch(r, "555-1234").back()) == 1234 );

show( r.smatch("555-1234") );
show( r.smatch("415-555-1234") );
show( r.smatch("415.555.1234") );

assert(regex.smatch(r, "4015") eq nil); // no match

{
    let text = "123 bar foo 213 blah 11113",
        re   = "([12]+)3",
        repl = "[\1]",
        val  = regex.replace(re, text, repl);

    print("text = %s\nre = %s\nrepl = %s\nval = %s\n" % (text, re, repl, val));

    assert(val == "[12] bar foo [21] blah [1111]");
}

print("\n\n\n");

let re = regex("(<[^>]+>)([^<]*)(</[^>]+>)");
string te = "<thing>123 and more</thing>";

print("%s\n" % re.smatch(te));

