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
#include <QShowEvent>
#include <QHideEvent>

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

        void showHelpWindow();
        void applyStyle();
        void handleMenuBar();
        void resetDockSpace();

    protected:
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;

    private:
        void initializeImGui();

        QTimer m_timer;
        bool m_initialized = false;
        
    };
} // namespace Rv

#endif // __RvCommon__ImguiView__h__
