//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use encoding;


\: test (void;)
{
    let s = "hello",
        u = string_to_utf8(s),
        b = to_base64(u),
        e = utf8_to_string(b),
        r = from_base64(b);

    assert(e == "aGVsbG8=");
    print("s = %s\n" % s);
    print("u = %s\n" % u);
    print("b = %s\n" % b);
    print("e = %s\n" % e);
    print("r = %s\n" % r);
    assert(utf8_to_string(r) == s);
}

test();

