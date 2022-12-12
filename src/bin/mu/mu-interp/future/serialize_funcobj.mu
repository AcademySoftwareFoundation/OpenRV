//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use io;

//let M = \: (float;float a, float b, float c) { b + math.sin(c) + a; };
//let F = M(,,0)(,68);

function: doit (float; int a, int b)
{
    let MF = \: (float; float a, float b, float c)
    {
        let A = \: (float; float a, float b) { a + b; };
        math.sin(c) + A(1,)(2);
    };

    let A = MF(,,0);
    A(a, b);
}

doit(1, 2);

//let F = A(,68);

// print(string(F) + "\n");

// print("WRITING\n");
// let ofile = ofstream("/tmp/test.muc");
// let n = serialize(ofile, F);
// ofile.close();
// n;

// print("READING\n");
// let ifile = ifstream("/tmp/test.muc");
// (float;float) G = deserialize(ifile);
// ifile.close();
// G(1.0);

