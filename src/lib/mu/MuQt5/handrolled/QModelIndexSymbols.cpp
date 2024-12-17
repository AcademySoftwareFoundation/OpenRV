//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

c->arrayType(this, 1, 0);

addSymbols(

    new Function(c, "internalPointer", _n_internalPointer, None, Return,
                 "object", Parameters, new Param(c, "this", "qt.QModelIndex"),
                 End),

    EndArguments);
