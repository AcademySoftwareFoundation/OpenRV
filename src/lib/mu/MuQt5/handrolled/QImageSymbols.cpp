//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// MISSING: QImage (QImage; QImage this, string fileName, "const char *" format)

addSymbol(new Function(c, "QImage", _n_QImage8, None, Compiled,
                       QImage_QImage_QImage_QImage_string_string, Return,
                       "qt.QImage", Parameters,
                       new Param(c, "this", "qt.QImage"),
                       new Param(c, "fileName", "string"),
                       new Param(c, "format", "string"), End));
