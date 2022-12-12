//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use test;
use io;

a_class a = a_class("A");
b_class b = b_class("B");

b.value = 123.321;
b.ch = 'q';
a.next = b;
a.value = 69;

string a_value = string(a);
string b_value = string(b);

let ofile = ofstream("/tmp/test.muc");
let n = serialize(ofile, a);
ofile.close();
n;

let ifile = ifstream("/tmp/test.muc");
a_class xa = deserialize(ifile);
b_class xb = xa.next;
ifile.close();

assert(string(xa) == a_value);
assert(string(xb) == b_value);


a_class root = a_class("ROOT");
b_class head = b_class("HEAD");
root.next = head;

for (int i=0; i < 1000; i++)
{
    b_class b = b_class("NODE " + i);
    head.next = b;
    b.value = float(100) / 100.0;
    b.ch = 'm';
    head = b;
}

string rootdump = string(root);

osstream osbuf = osstream();
serialize(osbuf, root);

isstream isbuf = isstream(string(osbuf));
a_class newroot = deserialize(isbuf);
assert(rootdump == string(newroot));
