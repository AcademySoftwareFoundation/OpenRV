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

Pointer qt_QVariant_QVariant_QVariant_QVariant_QIcon(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this,
                                                     Pointer param_val)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QIcon arg1 = getqtype<QIconType>(param_val);
    setqtype<QVariantType>(param_this, QVariant(arg1));
    return param_this;
}

static NODE_IMPLEMENTATION(_n_QVariant80, Pointer)
{
    NODE_RETURN(qt_QVariant_QVariant_QVariant_QVariant_QIcon(
        NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
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
