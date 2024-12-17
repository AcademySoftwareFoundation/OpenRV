//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

Pointer QByteArray_constData_QByteArray_byteECB_BSB__(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this)
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
    NODE_RETURN(QByteArray_constData_QByteArray_byteECB_BSB__(
        NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}
