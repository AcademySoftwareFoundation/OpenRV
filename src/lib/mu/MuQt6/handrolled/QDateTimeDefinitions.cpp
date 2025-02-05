//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

Pointer qt_QDateTime_fromString_QDateTime_string_string(Mu::Thread& NODE_THREAD,
                                                        Pointer param_string,
                                                        Pointer param_format)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QString arg0 = qstring(param_string);
    const QString arg1 = qstring(param_format);

    QStringList parts = arg1.split(" ");
    QDate date = QDate::fromString(parts[0], "yyyy-MM-dd");
    QTime time = QTime::fromString(parts[1], "hh:mm:ss");

    QDateTime dt(date, time);
    return makeqtype<QDateTimeType>(c, dt, "qt.QDateTime");
}

static NODE_IMPLEMENTATION(_n_fromString1, Pointer)
{
    NODE_RETURN(qt_QDateTime_fromString_QDateTime_string_string(
        NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}
