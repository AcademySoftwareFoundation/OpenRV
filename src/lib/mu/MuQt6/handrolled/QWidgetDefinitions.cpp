//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

static NODE_IMPLEMENTATION(_n_QWidget_windowFlags_int, int)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidget* arg0 = object<QWidget>(NODE_ARG(0, Pointer));
    NODE_RETURN(arg0->windowFlags());
}

static NODE_IMPLEMENTATION(_n_QWidget_setWindowFlags_void_int, void)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidget* arg0 = object<QWidget>(NODE_ARG(0, Pointer));
    arg0->setWindowFlags(Qt::WindowFlags(NODE_ARG(0, int)));
}

Pointer qt_QWidget_action_QAction_QWidget_int(Mu::Thread& NODE_THREAD, Pointer param_this, int index)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidget* arg0 = object<QWidget>(param_this);
    return makeinstance<QActionType>(c, arg0->actions()[index], "qt.QAction");
}

static NODE_IMPLEMENTATION(_n_action0, Pointer)
{
    NODE_RETURN(qt_QWidget_action_QAction_QWidget_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}
