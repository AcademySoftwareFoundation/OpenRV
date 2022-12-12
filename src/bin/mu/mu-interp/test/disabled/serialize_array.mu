//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

use io;
BigMess := int[2,2][][];
BigMess foo;

for (int j=0; j < 10; j++)
{
    foo.push_back(int[2,2][]());

    for (int i=0; i < j * 10; i++) 
    {
        foo.back().push_back(int[2,2] {i, j, j, i});
    }
}

string dump = string(foo);

{
    ofstream f = ofstream("/tmp/test.muc");
    serialize(f, foo);
    f.close();
}

{
    ifstream f = ifstream("/tmp/test.muc");
    BigMess bar = deserialize(f);
    assert(dump == string(bar));
    f.close();
}
