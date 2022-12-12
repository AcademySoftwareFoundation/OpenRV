//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

let s = "这是一个测试的Unicode字符编码",
    a = "هذا اختبار لليونيكود ترميز الأحر";

print("s = \"%s\", length = %d\n" % (s, s.size()));
print("c = %c\n" % '是');

assert(s.substr(7, 7) == "Unicode");
assert(s[7] == "U");
assert(s[14] == '字');
assert(s.size() == 18);
string x = 'む';
assert(x == 'む');

print("無 => %c\n" % 'む');
