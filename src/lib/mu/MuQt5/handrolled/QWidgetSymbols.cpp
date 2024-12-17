//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

//
//  For some reason the window flags is not showing up in the
//  metaObject system.
//

c->arrayType(c->stringType(), 1, 0);
Context::TypeVector ttypes(4);
ttypes[0] = c->intType();
ttypes[1] = ttypes[0];
ttypes[2] = ttypes[0];
ttypes[3] = ttypes[0];
c->tupleType(ttypes);

addSymbols(

    new Function(c, "windowFlags", _n_QWidget_windowFlags_int, None, Return,
                 "int", Parameters, new Param(c, "this", "qt.QWidget"), End),

    new Function(c, "setWindowFlags", _n_QWidget_setWindowFlags_void_int, None,
                 Return, "void", Parameters, new Param(c, "this", "qt.QWidget"),
                 new Param(c, "windowFlags", "int"), End),

    new Function(c, "getContentsMargins", _n_QWidget_getContentsMargins, None,
                 Return, "(int,int,int,int)", Parameters,
                 new Param(c, "this", "qt.QWidget"), End),

    EndArguments);
