//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
require math_linear;
use io;

M44 := float[4,4];
Vec4 := vector float[4];
Vec3 := vector float[3];

\: format (string; M44 M)
{
    ostream out = osstream();

    for_index (i,j; M)
    {
        print(out, " %g" % M[i,j]);
        if (j == 3) print(out, "\n");
    }

    string(out);
}

\: format (string; float[5,5] M)
{
    ostream out = osstream();

    for_index (i,j; M)
    {
        print(out, " %g" % M[i,j]);
        if (j == 4) print(out, "\n");
    }

    string(out);
}

\: test ()
{
    let A = M44(2, 0, 0, 0,
                0, 2, 0 ,0,
                0, 0, 2, 0,
                0, 0, 0, 1);

    let B = M44(2, 0, 0, 0,
                0, 2, 0 ,0,
                0, 0, 2, 0,
                0, 0, 0, 1);

    let M = M44(2, 0, 0, 0,
                0, 2, 0 ,0,
                0, 0, 2, 0,
                0, 0, 0, 1);

    let Q = float[5,5] {2, 0, 0, 0, 0,
                        0, 2, 0 ,0, 0,
                        0, 0, 2, 0, 0,
                        0, 0, 0, 2, 0,
                        0, 0, 0, 0, 1};

    float[100,100] BIG;
    for_index (i, j; BIG) if (i == j) BIG[i,j] = 2.0;

    let v4 = Vec4(1,2,3,1);
    let v3 = Vec3(1,2,3);

    print("M = \n%s" % format(M));
    print("inverse(M) = \n%s" % format(inverse(M)));
    print("Q = \n%s" % format(Q));
    print("inverse(Q) = \n%s" % format(inverse(Q)));
    inverse(BIG);
    print("M * v4 = %s\n" % (M * v4));
    print("M * v3 = %s\n" % (M * v3));
    print("A * B = \n%s" % format(A * B));

}


test();
