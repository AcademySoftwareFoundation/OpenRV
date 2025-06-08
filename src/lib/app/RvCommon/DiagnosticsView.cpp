//
// Copyright (C) 2024 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Python.h>
#include <RvCommon/DiagnosticsView.h>

#include <imgui.h>
#include <imgui_impl_qt.hpp>
#include <imgui_impl_opengl2.h>
#include <implot/implot.h>

#include <ImGuiPythonBridge.h>
#include <imgui_node_editor.h>
namespace ed = ax::NodeEditor;

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
            ImPlot::CreateContext();
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
                ImPlot::DestroyContext();
                ImGui::DestroyContext();
            }
        }
    }

    int DiagnosticsModule::_initCount = 0;

    std::vector<PyObject*> DiagnosticsView::s_imguiCallbacks;

    void DiagnosticsView::registerImGuiCallback(PyObject* callable)
    {
        if (PyCallable_Check(callable))
        {
            Py_INCREF(callable);
            s_imguiCallbacks.push_back(callable);
        }
    }

    void DiagnosticsView::callImGuiCallbacks()
    {
        for (auto* cb : s_imguiCallbacks)
        {
            PyGILState_STATE gstate = PyGILState_Ensure();
            PyObject* result = PyObject_CallObject(cb, nullptr);
            if (!result && PyErr_Occurred())
                PyErr_Print();
            Py_XDECREF(result);
            PyGILState_Release(gstate);
        }
    }

    DiagnosticsView::DiagnosticsView(QWidget* parent, const QSurfaceFormat& fmt)
        : QOpenGLWidget(parent)
        , m_timer(this)
    {
        setFormat(fmt);

        DiagnosticsModule::init();

        ImGui_ImplQt_RegisterWidget(this);

        // Send an invalidate request every 33 milliseconds (eg: 30 fps tops)
        connect(&m_timer, &QTimer::timeout, this,
                QOverload<>::of(&QWidget::update));
        m_timer.start(
            40); // every 40 ms = 25 fps -- more than enough for idle updates
    }

    DiagnosticsView::~DiagnosticsView()
    {
        m_timer.stop();
        DiagnosticsModule::shutdown();
    }

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

        // Set ImGui window to cover the full size of the Qt widget
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width(), height()));

        ImGui::Begin("Diagnostics View", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // Create a dockspace inside the Diagnostics View window
        ImGuiID dockspace_id = ImGui::GetID("DiagnosticsDockSpace");
        ImVec2 dockspace_size = ImGui::GetContentRegionAvail();
        ImGui::DockSpace(dockspace_id, dockspace_size, ImGuiDockNodeFlags_None);

        Rv::ImGuiPythonBridge::callCallbacks();

        ImGui::End();

        // Render ImGui Frame
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    }

} // namespace Rv
