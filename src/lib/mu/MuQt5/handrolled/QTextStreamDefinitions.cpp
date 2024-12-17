//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

// Pointer
// qt_QTextStream_QTextStream_QTextStream_QTextStream_string_int(Mu::Thread&
// NODE_THREAD, Pointer param_this, Pointer param_string, int param_openMode)
// {
//     MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
//     QString arg1 = qstring(param_string);
//     QIODevice::OpenMode arg2 = (QIODevice::OpenMode)(param_openMode);
//     setqpointer<QTextStreamType>(param_this,new QTextStream(arg1, arg2));
//     return param_this;
// }

Pointer qt_QTextStream_QTextStream_QTextStream_QTextStream_QByteArray_int(
    Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_array,
    int param_openMode)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QByteArray* arg1 = &getqtype<QByteArrayType>(param_array);
    QIODevice::OpenMode arg2 = (QIODevice::OpenMode)(param_openMode);
    setqpointer<QTextStreamType>(param_this, new QTextStream(arg1, arg2));
    return param_this;
}

static void print_void_qt_QTextStream_string(Thread& NODE_THREAD, Pointer obj,
                                             Pointer p)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextStream* arg0 = getqpointer<QTextStreamType>(obj);
    QString arg1 = qstring(p);
    *arg0 << arg1;
}

// static NODE_IMPLEMENTATION(_n_QTextStream3, Pointer)
// {
//     NODE_RETURN(qt_QTextStream_QTextStream_QTextStream_QTextStream_string_int(NODE_THREAD,
//     NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, int)));
// }

static NODE_IMPLEMENTATION(_n_QTextStream4, Pointer)
{
    NODE_RETURN(
        qt_QTextStream_QTextStream_QTextStream_QTextStream_QByteArray_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, int)));
}

static NODE_IMPLEMENTATION(printTextStream, void)
{
    print_void_qt_QTextStream_string(NODE_THREAD, NODE_ARG(0, Pointer),
                                     NODE_ARG(1, Pointer));
}
