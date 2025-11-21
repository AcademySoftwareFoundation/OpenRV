//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

Pointer QByteArray_constData_QByteArray_byteECB_BSB__(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QByteArray arg0 = getqtype<QByteArrayType>(param_this);
    Class* atype = (Class*)c->arrayType(c->byteType(), 1, 0);
    DynamicArray* array = new DynamicArray(atype, 1);
    array->resize(arg0.size());
    memcpy(array->data<char>(), arg0.constData(), arg0.size());
    return array;
}

static NODE_IMPLEMENTATION(constData, Pointer)
{
    NODE_RETURN(QByteArray_constData_QByteArray_byteECB_BSB__(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

Pointer qt_QByteArray_append_QByteArray_QByteArray_string(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_str)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QByteArray& arg0 = getqtype<QByteArrayType>(param_this);
    const QString arg1 = qstring(param_str);
    return makeqtype<QByteArrayType>(c, arg0.append(arg1.toStdString().c_str()), "qt.QByteArray");
}

static NODE_IMPLEMENTATION(_n_append4, Pointer)
{
    NODE_RETURN(qt_QByteArray_append_QByteArray_QByteArray_string(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}
