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

#include "DiagnosticsView.Roboto_Regular"

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

        setMinimumSize(100, 100);

        DiagnosticsModule::init();

        applyStyle();

        ImGui_ImplQt_RegisterWidget(this);

        // Send an invalidate request every 33 milliseconds (eg: 30 fps tops)
        connect(&m_timer, &QTimer::timeout, this,
                QOverload<>::of(&QWidget::update));

        // every 40 ms = 25 fps -- more than enough for idle updates
        m_timer.start(40);
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

    void DiagnosticsView::applyStyle()
    {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        // Base colors for a pleasant and modern dark theme with dark accents
        colors[ImGuiCol_Text] = ImVec4(
            0.92f, 0.93f, 0.94f, 1.00f); // Light grey text for readability
        colors[ImGuiCol_TextDisabled] =
            ImVec4(0.50f, 0.52f, 0.54f, 1.00f); // Subtle grey for disabled text
        colors[ImGuiCol_WindowBg] = ImVec4(
            0.14f, 0.14f, 0.16f, 1.00f); // Dark background with a hint of blue
        colors[ImGuiCol_ChildBg] = ImVec4(
            0.16f, 0.16f, 0.18f, 1.00f); // Slightly lighter for child elements
        colors[ImGuiCol_PopupBg] =
            ImVec4(0.18f, 0.18f, 0.20f, 1.00f); // Popup background
        colors[ImGuiCol_Border] =
            ImVec4(0.28f, 0.29f, 0.30f, 0.60f); // Soft border color
        colors[ImGuiCol_BorderShadow] =
            ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // No border shadow
        colors[ImGuiCol_FrameBg] =
            ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Frame background
        colors[ImGuiCol_FrameBgHovered] =
            ImVec4(0.22f, 0.24f, 0.26f, 1.00f); // Frame hover effect
        colors[ImGuiCol_FrameBgActive] =
            ImVec4(0.24f, 0.26f, 0.28f, 1.00f); // Active frame background
        colors[ImGuiCol_TitleBg] =
            ImVec4(0.14f, 0.14f, 0.16f, 1.00f); // Title background
        colors[ImGuiCol_TitleBgActive] =
            ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // Active title background
        colors[ImGuiCol_TitleBgCollapsed] =
            ImVec4(0.14f, 0.14f, 0.16f, 1.00f); // Collapsed title background
        colors[ImGuiCol_MenuBarBg] =
            ImVec4(0.20f, 0.20f, 0.22f, 1.00f); // Menu bar background
        colors[ImGuiCol_ScrollbarBg] =
            ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // Scrollbar background
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(
            0.24f, 0.26f, 0.28f, 1.00f); // Dark accent for scrollbar grab
        colors[ImGuiCol_ScrollbarGrabHovered] =
            ImVec4(0.28f, 0.30f, 0.32f, 1.00f); // Scrollbar grab hover
        colors[ImGuiCol_ScrollbarGrabActive] =
            ImVec4(0.32f, 0.34f, 0.36f, 1.00f); // Scrollbar grab active
        colors[ImGuiCol_CheckMark] =
            ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Dark blue checkmark
        colors[ImGuiCol_SliderGrab] =
            ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Dark blue slider grab
        colors[ImGuiCol_SliderGrabActive] =
            ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Active slider grab
        colors[ImGuiCol_Button] =
            ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Dark blue button
        colors[ImGuiCol_ButtonHovered] =
            ImVec4(0.28f, 0.38f, 0.48f, 1.00f); // Button hover effect
        colors[ImGuiCol_ButtonActive] =
            ImVec4(0.32f, 0.42f, 0.52f, 1.00f); // Active button
        colors[ImGuiCol_Header] = ImVec4(
            0.24f, 0.34f, 0.44f, 1.00f); // Header color similar to button
        colors[ImGuiCol_HeaderHovered] =
            ImVec4(0.28f, 0.38f, 0.48f, 1.00f); // Header hover effect
        colors[ImGuiCol_HeaderActive] =
            ImVec4(0.32f, 0.42f, 0.52f, 1.00f); // Active header
        colors[ImGuiCol_Separator] =
            ImVec4(0.28f, 0.29f, 0.30f, 1.00f); // Separator color
        colors[ImGuiCol_SeparatorHovered] =
            ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Hover effect for separator
        colors[ImGuiCol_SeparatorActive] =
            ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Active separator
        colors[ImGuiCol_ResizeGrip] =
            ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Resize grip
        colors[ImGuiCol_ResizeGripHovered] =
            ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Hover effect for resize grip
        colors[ImGuiCol_ResizeGripActive] =
            ImVec4(0.44f, 0.54f, 0.64f, 1.00f); // Active resize grip
        colors[ImGuiCol_Tab] =
            ImVec4(0.25f, 0.25f, 0.25f, 1.00f); // Inactive tab
        colors[ImGuiCol_TabHovered] =
            ImVec4(0.7f, 0.7f, 0.7f, 1.00f); // Hover effect for tab
        colors[ImGuiCol_TabActive] =
            ImVec4(0.5f, 0.5f, 0.5f, 1.00f); // Active tab color
        colors[ImGuiCol_TabUnfocused] =
            ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Unfocused tab
        colors[ImGuiCol_TabUnfocusedActive] =
            ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Active but unfocused tab
        colors[ImGuiCol_PlotLines] =
            ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Plot lines
        colors[ImGuiCol_PlotLinesHovered] =
            ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Hover effect for plot lines
        colors[ImGuiCol_PlotHistogram] =
            ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Histogram color
        colors[ImGuiCol_PlotHistogramHovered] =
            ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Hover effect for histogram
        colors[ImGuiCol_TableHeaderBg] =
            ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Table header background
        colors[ImGuiCol_TableBorderStrong] =
            ImVec4(0.28f, 0.29f, 0.30f, 1.00f); // Strong border for tables
        colors[ImGuiCol_TableBorderLight] =
            ImVec4(0.24f, 0.25f, 0.26f, 1.00f); // Light border for tables
        colors[ImGuiCol_TableRowBg] =
            ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Table row background
        colors[ImGuiCol_TableRowBgAlt] =
            ImVec4(0.22f, 0.24f, 0.26f, 1.00f); // Alternate row background
        colors[ImGuiCol_TextSelectedBg] =
            ImVec4(0.24f, 0.34f, 0.44f, 0.35f); // Selected text background
        colors[ImGuiCol_DragDropTarget] =
            ImVec4(0.46f, 0.56f, 0.66f, 0.90f); // Drag and drop target
        colors[ImGuiCol_NavHighlight] =
            ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Navigation highlight
        colors[ImGuiCol_NavWindowingHighlight] =
            ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // Windowing highlight
        colors[ImGuiCol_NavWindowingDimBg] =
            ImVec4(0.80f, 0.80f, 0.80f, 0.20f); // Dim background for windowing
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(
            0.80f, 0.80f, 0.80f, 0.35f); // Dim background for modal windows

        // Style adjustments
        style->WindowPadding = ImVec2(8.00f, 8.00f);
        style->FramePadding = ImVec2(5.00f, 4.00f);
        style->CellPadding = ImVec2(6.00f, 6.00f);
        style->ItemSpacing = ImVec2(6.00f, 6.00f);
        style->ItemInnerSpacing = ImVec2(6.00f, 6.00f);
        style->TouchExtraPadding = ImVec2(0.00f, 0.00f);
        style->IndentSpacing = 25;
        style->ScrollbarSize = 11;
        style->GrabMinSize = 10;
        style->WindowBorderSize = 1;
        style->ChildBorderSize = 1;
        style->PopupBorderSize = 1;
        style->FrameBorderSize = 1;
        style->TabBorderSize = 1;
        style->WindowRounding = 7;
        style->ChildRounding = 4;
        style->FrameRounding = 3;
        style->PopupRounding = 4;
        style->ScrollbarRounding = 9;
        style->GrabRounding = 3;
        style->LogSliderDeadzone = 4;
        style->TabRounding = 4;

        ImFontConfig fontCfg;
        fontCfg.FontDataOwnedByAtlas = false;

        ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
            Roboto_Regular_compressed_data, Roboto_Regular_compressed_size, 13,
            &fontCfg);
    }

    void DiagnosticsView::paintGL()
    {
        QOpenGLWidget::paintGL();

        // Start ImGui Frame
        ImGui_ImplQt_NewFrame(this);
        ImGui_ImplOpenGL2_NewFrame();
        ImGui::NewFrame();

        // Create a dockspace inside the Diagnostics View window
        ImGuiID dockspace_id = ImGui::GetID("DiagnosticsDockSpace");
        ImVec2 dockspace_size = ImGui::GetContentRegionAvail();
        ImGui::DockSpaceOverViewport(dockspace_id);
        Rv::ImGuiPythonBridge::callCallbacks();

        // Render ImGui Frame
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    }

} // namespace Rv
