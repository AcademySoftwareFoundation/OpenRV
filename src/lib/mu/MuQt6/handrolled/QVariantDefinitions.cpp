//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

static int QVariant_toInt_int(Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QVariant arg0 = getqtype<QVariantType>(param_this);
    return arg0.toInt();
}

static NODE_IMPLEMENTATION(toInt, int)
{
    NODE_RETURN(QVariant_toInt_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

/*
static Pointer
QVariant_toQObject_QObject(Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QVariant arg0 = getqtype<QVariantType>(param_this);
    QObject* obj = arg0.value<QObject*>();
    return makeqpointer<QObjectType>(c, obj, "qt.QObject");
}

static NODE_IMPLEMENTATION(toQObject, Pointer)
{
    NODE_RETURN(QVariant_toQObject_QObject(NODE_THREAD, NONNIL_NODE_ARG(0,
Pointer)));
}
*/
