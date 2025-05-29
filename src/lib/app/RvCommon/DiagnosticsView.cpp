//
// Copyright (C) 2024 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <RvCommon/DiagnosticsView.h>

#include <imgui.h>
#include <imgui_impl_qt.hpp>
#include <imgui_impl_opengl2.h>

namespace Rv
{
    class DiagnosticsModule
    {
    public:
        static void init();
        static void shutdown();

    private:
        static int _initCount;
    };

    void DiagnosticsModule::init()
    {
        _initCount++;

        if (_initCount == 1)
        {
            _initCount++;
            ImGui::CreateContext();
            ImGui_ImplQt_Init();
            ImGui_ImplOpenGL2_Init();
        }
    }

    void DiagnosticsModule::shutdown()
    {
        if (_initCount > 0)
        {
            _initCount--;

            if (_initCount == 0)
            {
                ImGui_ImplOpenGL2_Shutdown();
                ImGui_ImplQt_Shutdown();
                ImGui::DestroyContext();
            }
        }
    }

    int DiagnosticsModule::_initCount = 0;

    DiagnosticsView::DiagnosticsView(QWidget* parent, const QSurfaceFormat& fmt)
        : QOpenGLWidget(parent)
    {
        setFormat(fmt);

        DiagnosticsModule::init();

        ImGui_ImplQt_RegisterWidget(this);
    }

    DiagnosticsView::~DiagnosticsView() { DiagnosticsModule::shutdown(); }

    void DiagnosticsView::initializeGL()
    {
        QOpenGLWidget::initializeGL();
        initializeOpenGLFunctions();
    }

    void DiagnosticsView::paintGL()
    {
        QOpenGLWidget::paintGL();

        // Start ImGui Frame
        ImGui_ImplQt_NewFrame(this);
        ImGui_ImplOpenGL2_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        // Render ImGui Frame
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    }

} // namespace Rv
