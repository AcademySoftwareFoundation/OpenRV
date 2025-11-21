//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

addSymbols(new Function(c, "findChild", findChild, None, Return, ftn, Parameters, new Param(c, "this", ftn), new Param(c, "name", "string"),
                        End),

           new Function(c, "setProperty", setProperty, None, Return, "bool", Parameters, new Param(c, "this", ftn),
                        new Param(c, "name", "string"), new Param(c, "value", "qt.QVariant"), End),

           new Function(c, "tr", _n_tr0, None, Compiled, qt_QObject_tr_string_QObject_string_string_int, Return, "string", Parameters,
                        new Param(c, "sourceText", "string"), new Param(c, "disambiguation", "string", Value(nullptr)),
                        new Param(c, "n", "int", Value((int)-1)), End),

           EndArguments);
