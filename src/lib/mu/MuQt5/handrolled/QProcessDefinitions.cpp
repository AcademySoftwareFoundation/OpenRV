//
// Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

bool qt_QProcess_startDetached_bool_string_stringBSB_ESB_(
    Mu::Thread& NODE_THREAD, Pointer param_command, Pointer param_arguments)
{
    // Leave the default value for the 3rd and 4th arguments
    //    const QString &workingDirectory = QString()
    //    qint64 *pid = nullptr

    const QString arg0 = qstring(param_command);
    const QStringList arg1 = qstringlist(param_arguments);
    return QProcess::startDetached(arg0, arg1);
}

static NODE_IMPLEMENTATION(_n_startDetached0, bool)
{
    NODE_RETURN(qt_QProcess_startDetached_bool_string_stringBSB_ESB_(
        NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}
