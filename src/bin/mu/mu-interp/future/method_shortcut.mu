//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Method short cut. set() is called as Foo.set(this, n).  
//
//  NOTE: currently there's a problem with reversing the order of
//  these functions:
//
//  ERROR: No match found for function "set" with 1 argument: Foo
//      Option #1: Foo.set (void; Foo this, Foo n) (member)
//  test/method_shortcut.mu, line 7, char 35: Cannot resolve call to "set".
//  ERROR: attempted call to unresolved function
//

class: Foo 
{
  method: Foo (Foo; Foo n) { set(n); }
  method: set (void; Foo n) { next = n; }

  Foo next;
};

Foo(Foo(nil));

