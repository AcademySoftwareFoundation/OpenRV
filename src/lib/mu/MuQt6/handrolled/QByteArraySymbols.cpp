//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

c->arrayType(c->byteType(), 1, 0);

addSymbols(new Function(c, "constData", constData, None, Compiled, QByteArray_constData_QByteArray_byteECB_BSB__, Return, "byte[]",
                        Parameters, new Param(c, "this", "qt.QByteArray"), End),

           new Function(c, "append", _n_append4, None, Compiled, qt_QByteArray_append_QByteArray_QByteArray_string, Return, "qt.QByteArray",
                        Parameters, new Param(c, "this", "qt.QByteArray"), new Param(c, "str", "string"), End),
           EndArguments);
