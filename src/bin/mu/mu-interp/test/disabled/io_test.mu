//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Test input/output streams. 
//
//  Just for the fun, try and duplicate C++'s "stream operator"
//  syntax on top of Mu's io module.
//

use io;

string testfile = "test.file";

//
//  ManipFunc is a function that takes an ostream and returns an ostream
//

ManipFunc := (ostream; ostream);

//
//  These are the output and input operators. Since ostream uses
//  print() these operators convert the stream-like syntax into
//  regular function calls.
//

operator: << (ostream; ostream o, string s) { print(o, s); }
operator: << (ostream; ostream o, int i) { print(o, i); }
operator: << (ostream; ostream o, bool i) { print(o, i); }
operator: << (ostream; ostream o, float i) { print(o, i); }
operator: << (ostream; ostream o, byte b) { print(o, b); }

//
//  This output operator mimics C++ stream manipulators. The operator
//  takes a stream and a function that takes a stream. It calls it
//  with the passed in stream returning a stream. This is convenient
//  since the io module includes functions that already have this
//  signature.
//

operator: << (ostream; ostream o, ManipFunc x) { x(o); }

//
//  Now try and use all of the above.
//  "endl" and "flush" are actually stream functions which have the
//  same signature as ManipFunc, so you can just use them as stream
//  operator arguments without modification.
//

function: write_it (ostream; ostream o)
{
    o << "hello world" 
      << ", int = " << 123 
      << ", float = " << 456.789 // currently mu defaults to float if no suffix given
      << ", bool = " << true
      << ", byte = " << byte(64)
      << ", string = " << "foo"
      << endl
      << flush;
}

function: read_it (istream; istream i)
{
    string in_string;
    int in_int;
    float in_float;
    bool in_bool;
    byte in_byte;

    read_string(i, 4); in_int = read_int(i);
    read_string(i, 3); in_float = read_float(i);
    read_string(i, 3); in_bool = read_bool(i);
    read_string(i, 3); in_byte = read_byte(i);
    read_string(i, 3); in_string = read_string(i);
    read_string(i);

    //print("in_int = %s\n" % in_int);
    //print("in_float = %s\n" % in_float);
    //print("in_bool = %s\n" % in_bool);
    //print("in_byte = %s\n" % in_byte);
    //print("in_string = %s\n" % in_string);

    assert(in_int == 123);
    assert(in_float == 456.789);
    assert(in_bool == true);
    assert(in_byte == 64);
    assert(in_string == "foo");
    
    assert(i.eof());
    i;
}

//
//  Write the file
//

ofstream ofile = ofstream(testfile);
write_it(ofile);
ofile.close();

//
//  Read it back
//

ifstream ifile = ifstream(testfile);
string stuff;

if (ifile.good() && !ifile.bad() && !ifile.fail() && !ifile.eof())
{
    stuff = ifile.gets("\n");
}

let o = "hello world, int = 123, float = 456.789, bool = 1, byte = 64, string = foo";
if (stuff != o)
{
    print("STUFF = %s\n" % stuff);
    print("O     = %s\n" % o);
}
assert(stuff == o);
ifile.close();

//
//  Now use an osstream on write_it()
//

osstream ss = osstream();
write_it(ss);
stuff = string(ss);
assert(stuff == (o + "\n"));

//
//  Finally, let's try and scan it.
//

isstream is = isstream(stuff);
read_it(is);

ifile = ifstream(testfile);
read_it(ifile);
ifile.close();

write_it(out());
