//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

\: doit (void;)
{
    string s1 = "Hello World!\n";
    string s2 = "Bye.\n";
    string s3 = "Bye.\n";
    string s4 = s2;

    print(s1);
    print(s2);

    assert(s1.size() == 13);

    print(s1.substr(0,5));
    assert(s1.substr(0,5) == "Hello");

    print("\n");
    print("the int five: \"" + 5 + "\"\n");
    print("the float five point one two three: \"" + 5.123 + "\"\n");
    print("the bool \"" + false + "\" and \"" + true + "\"\n");

    try
    {
        s1[1000];
    }
    catch (exception exc)
    {
        // should be out of range
        print("caught: " + string(exc) + "\n");
    }

    string[] words = { "one", "two", "three", "four" };
    let joined = string.join(words, " む ");
    print("join = \"%s\"\n" % joined);
    string[] words2 = joined.split(" む ");
    print("words2 = %s\n" % words2);

    for_index(i; words) assert(words[i] == words2[i]);

    // Test contains method
    string testStr = "Hello World";
    assert(string.contains(testStr,"World") == 6);
    assert(string.contains(testStr,"Hello") == 0);
    assert(string.contains(testStr,"xyz") == -1);
    print("contains tests passed\n");

    // Test replace method
    string original = "Hello Bob";
    string replaced = string.replace(original, "Bob", "123");
    assert(replaced == "Hello 123");
    assert(original == "Hello Bob"); // original should be unchanged
    
    string notFound = string.replace(original, "xyz", "abc");
    assert(notFound == "Hello Bob"); // should return original if not found
    print("replace tests passed\n");
}

doit();
