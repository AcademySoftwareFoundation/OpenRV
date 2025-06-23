//
// Copyright (C) 2024 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __RvCommon__ImguiView__h__
#define __RvCommon__ImguiView__h__

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QSurfaceFormat>
#include <QTimer>

#include <vector>

struct _object;
typedef _object PyObject;

namespace Rv
{
    class DiagnosticsView
        : public QOpenGLWidget
        , private QOpenGLExtraFunctions
    {
    public:
        DiagnosticsView(QWidget* parent, const QSurfaceFormat& fmt);
        ~DiagnosticsView();

        static void registerImGuiCallback(PyObject* callable);
        static void callImGuiCallbacks();

        void initializeGL() override;
        void paintGL() override;

        void applyStyle();
        void handleMenuBar();
        void resetDockSpace();

    private:
        QTimer m_timer;
        static std::vector<PyObject*> s_imguiCallbacks;
    };
} // namespace Rv

#endif // __RvCommon__ImguiView__h__
