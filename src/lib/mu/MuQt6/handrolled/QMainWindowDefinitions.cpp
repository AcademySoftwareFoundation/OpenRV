//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

void qt_QMainWindow_resizeDock_void_QMainWindow_QDockWidget_int_int(
    Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_dockwidget,
    int param_size, int param_orientation)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QMainWindow* arg0 = object<QMainWindow>(param_this);
    QDockWidget* arg1 = object<QDockWidget>(param_dockwidget);
    int arg2 = (int)(param_size);
    Qt::Orientation arg3 = (Qt::Orientation)(param_orientation);

    QList<QDockWidget*> dockWidgets;
    dockWidgets.append(arg1);
    QList<int> sizes;
    sizes.append(arg2);

    arg0->resizeDocks(dockWidgets, sizes, arg3);
}

static NODE_IMPLEMENTATION(_n_resizeDock0, void)
{
    qt_QMainWindow_resizeDock_void_QMainWindow_QDockWidget_int_int(
        NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
        NODE_ARG(2, int), NODE_ARG(3, int));
}
