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

namespace Rv
{
    class DiagnosticsView
        : public QOpenGLWidget
        , private QOpenGLExtraFunctions
    {
    public:
        DiagnosticsView(QWidget* parent, const QSurfaceFormat& fmt);
        ~DiagnosticsView();

        void initializeGL() override;
        void paintGL() override;

    private:
        QTimer m_timer;
    };
} // namespace Rv

#endif // __RvCommon__ImguiView__h__
