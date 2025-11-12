//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// MISSING: startDetached (bool; string program, string[] arguments, string
// workingDirectory, "qint64 *" pid)

addSymbol(new Function(c, "startDetached", _n_startDetached0, None, Compiled,
                       // qt_QProcess_startDetached_bool_string_stringBSB_ESB_string_int64,
                       qt_QProcess_startDetached_bool_string_stringBSB_ESB_, Return, "bool", Parameters, new Param(c, "command", "string"),
                       new Param(c, "arguments", "string[]"), End));
