//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Sized is a interface to any type that has a "size" function of
//  type (int;Thing) where Thing is the actual type. In addition,
//  there is another function "bar" which may optionally be part of
//  the type. Its optional because the interface implements a default
//  version of the function -- so in this case, int[] can be cast to a
//  Sized.
//

// interface: Sized
// {
//   \: bar (int; Sized x) { 123; }
//   \: size (int; Sized);
// }

// Sized foo = int[] {1, 2, 3};
// assert(foo.size() == 3);
// assert(foo.bar() == 123);
// foo.size();
