//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

addSymbol(new Function(c, "toInt", toInt, None, Compiled, QVariant_toInt_int,
                       Return, "int", Parameters,
                       new Param(c, "this", "qt.QVariant"), End));

/*
addSymbol( new Function(c, "toObject", toQObject, None,
                        Compiled, QVariant_toQObject_int,
                        Return, "qt.QObject",
                        Parameters,
                        new Param(c, "this", "qt.QVariant"),
                        End) );
*/

// MISSING: toInt (int; QVariant this, "bool *" ok)
