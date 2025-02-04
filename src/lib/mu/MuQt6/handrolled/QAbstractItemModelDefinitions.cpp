//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

Pointer
qt_QAbstractItemModel_createIndex_QModelIndex_QAbstractItemModel_int_int_object(
    Mu::Thread& NODE_THREAD, Pointer param_this, int param_row,
    int param_column, Pointer param_any)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    MuQt_QAbstractItemModel* arg0 = object<MuQt_QAbstractItemModel>(param_this);
    int arg1 = (int)(param_row);
    int arg2 = (int)(param_column);
    Pointer arg3 = (Pointer)(param_any);
    return makeqtype<QModelIndexType>(
        c, arg0->createIndex0_pub(arg1, arg2, arg3), "qt.QModelIndex");
}

static NODE_IMPLEMENTATION(_n_createIndex0, Pointer)
{
    NODE_RETURN(
        qt_QAbstractItemModel_createIndex_QModelIndex_QAbstractItemModel_int_int_object(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int), NODE_ARG(3, Pointer)));
}
