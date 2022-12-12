//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  The Y combinator in Mu
//  NOTE: this isn't really possible without a purely dynamic type.
//  The simply typed lambda calculus cannot express Y()
//

//\: Y (le) 
//{    
//  (\: (f) {f(f);}) (\: (f) { le( \: (x) { f(f)(x); }); });
//}

