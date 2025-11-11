//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

globalScope()->addSymbols(new Function(c, "print", printTextStream, None, Return, "void", Compiled, print_void_qt_QTextStream_string,
                                       Parameters, new Param(c, "textstream", "qt.QTextStream"), new Param(c, "outputString", "string"),
                                       End),
                          EndArguments);

addSymbols(new Function(c, "QTextStream", _n_QTextStream0, None, Compiled, qt_QTextStream_QTextStream_QTextStream_QTextStream, Return,
                        "qt.QTextStream", Parameters, new Param(c, "this", "qt.QTextStream"), End),
           new Function(c, "QTextStream", _n_QTextStream1, None, Compiled, qt_QTextStream_QTextStream_QTextStream_QTextStream_QIODevice,
                        Return, "qt.QTextStream", Parameters, new Param(c, "this", "qt.QTextStream"),
                        new Param(c, "device", "qt.QIODevice"), End),
           // Can't use this one yet
           // new Function(c, "QTextStream", _n_QTextStream3, None, Compiled,
           // qt_QTextStream_QTextStream_QTextStream_QTextStream_string_int, Return,
           // "qt.QTextStream", Parameters, new Param(c, "this", "qt.QTextStream"), new
           // Param(c, "string", "string"), new Param(c, "openMode", "int"), End),
           new Function(c, "QTextStream", _n_QTextStream5, None, Compiled,
                        qt_QTextStream_QTextStream_QTextStream_QTextStream_QByteArray_int, Return, "qt.QTextStream", Parameters,
                        new Param(c, "this", "qt.QTextStream"), new Param(c, "array", "qt.QByteArray"), new Param(c, "openMode", "int"),
                        End),

           EndArguments);
