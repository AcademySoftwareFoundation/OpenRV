//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

c->arrayType(c->byteType(), 1, 0);

addSymbol(new Function(c, "constData", constData, None, Compiled,
                       QByteArray_constData_QByteArray_byteECB_BSB__, Return,
                       "byte[]", Parameters,
                       new Param(c, "this", "qt.QByteArray"), End));
