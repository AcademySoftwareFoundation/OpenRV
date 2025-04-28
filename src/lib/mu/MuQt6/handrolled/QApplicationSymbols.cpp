//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

c->arrayType(c->stringType(), 1, 0);

addSymbols(

    new Function(c, "QApplication", _n_QApplication0, None, Return,
                 "qt.QApplication", Parameters,
                 new Param(c, "this", "qt.QApplication"),
                 new Param(c, "args", "string[]"), End),

    EndArguments);
