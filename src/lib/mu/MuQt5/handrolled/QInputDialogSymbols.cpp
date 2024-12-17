//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// declare the tuple type (string,bool) just in case
Context::TypeVector tt(2);
tt[0] = c->stringType();
tt[1] = c->boolType();
c->tupleType(tt);

addSymbols(new Function(c, "getText", getText, None, Return, "(string,bool)",
                        Parameters, new Param(c, "parent", "qt.QWidget"),
                        new Param(c, "title", "string"),
                        new Param(c, "label", "string"),
                        new Param(c, "echoMode", "int"),
                        new Param(c, "text", "string"),
                        new Param(c, "windowFlags", "int"), End),

           EndArguments);
