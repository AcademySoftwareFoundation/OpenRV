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

#include <iostream>

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
                Rv::ImGuiPythonBridge::clearCallbacks();
                ImGui_ImplOpenGL2_Shutdown();
                ImGui_ImplQt_Shutdown();
                ImPlot::DestroyContext();
                ImGui::DestroyContext();
            }
        }
        else
        {
            std::cout << "DiagnosticsModule::shutdown - Warning: shutdown called but init count is already 0" << std::endl;
        }
    }

    int DiagnosticsModule::_initCount = 0;

    DiagnosticsView::DiagnosticsView(QWidget* parent, const QSurfaceFormat& fmt)
        : QOpenGLWidget(parent)
        , m_timer(this)
        , m_initialized(false)
    {
        setFormat(fmt);
        setMinimumSize(100, 100);

        // Set the timer interval (40 ms = 25 fps -- more than enough for idle updates)
        // but don't connect it yet - connection happens when first shown.
        m_timer.setInterval(40);
        
        // NOTE: No ImGui initialization here - it will be done lazily when first shown.
        // NOTE: Timer connection is also deferred until first show to minimize overhead.
    }

    void DiagnosticsView::initializeImGui()
    {
        if (!m_initialized)
        {
            DiagnosticsModule::init();
            applyStyle();
            ImGui_ImplQt_RegisterWidget(this);
            m_initialized = true;
        }
    }

    DiagnosticsView::~DiagnosticsView()
    {
        m_timer.stop();
        if (m_initialized)
        {
            DiagnosticsModule::shutdown();
        }
    }

    void DiagnosticsView::initializeGL()
    {
        QOpenGLWidget::initializeGL();
        initializeOpenGLFunctions();
        // NOTE: ImGui initialization moved to showEvent for lazy loading.
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

    void DiagnosticsView::showHelpWindow()
    {
        ImGui::Begin("ImGui/ImPlot Help");
        ImGui::TextWrapped("The diagnostics window uses the easy-to-learn Dear "
                           "ImGui and ImPlot APIs and their python bindings.");
        ImGui::TextWrapped("");
        ImGui::TextWrapped(
            "Adding your own diagnostics info is quick and easy, much easier "
            "than building your own Qt user interface.");
        ImGui::TextWrapped(
            "From your Python plugin, you simply need to register a "
            "diagnostics callback, and from the callback, call ImGui/ImPlot "
            "functions to display your data.");
        ImGui::TextWrapped(
            "Note that there is no need to implement backends for handling "
            "keyboard/mouse and graphics; this part is already done for you.");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("Hello World example:");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("---------------------------------------------");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("import pyimgui as imgui");
        ImGui::TextWrapped("import pyimplot as implot");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("def __init__( self ):");
        ImGui::TextWrapped(
            "   commands.register_diagnostics_callback( self.my_callback )");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("def __del__( self ):");
        ImGui::TextWrapped(
            "   commands.unregister_diagnostics_callback( self.my_callback )");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("def my_callback( self ):");
        ImGui::TextWrapped("   imgui.begin( \"Example Window\" )");
        ImGui::TextWrapped("   imgui.text( \"Hello World!\" )");
        ImGui::TextWrapped("   imgui.end()");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("---------------------------------------------");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("Full reference for Dear ImGui and Implot and their "
                           "bindings can be found here:");
        ImGui::TextLinkOpenURL("ImGui", "https://github.com/ocornut/imgui");
        ImGui::SameLine();
        ImGui::TextWrapped("and");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("ImGui Python bindings",
                               "https://github.com/pthom/imgui_bundle/blob/"
                               "main/external/imgui/bindings/pybind_imgui.cpp");
        ImGui::TextLinkOpenURL("ImPlot", "https://github.com/pthom/implot");
        ImGui::SameLine();
        ImGui::TextWrapped("and");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL(
            "ImPlot Python bindings",
            "https://github.com/pthom/imgui_bundle/blob/main/external/implot/"
            "bindings/pybind_implot.cpp");
        ImGui::TextWrapped("");
        ImGui::TextWrapped("Comprehensive Python examples of how to create "
                           "interactive interfaces can be found");
        ImGui::TextLinkOpenURL(
            "here (ImGui)",
            "https://github.com/pthom/imgui_bundle/blob/main/bindings/"
            "imgui_bundle/demos_python/demos_immapp/imgui_demo.py");
        ImGui::SameLine();
        ImGui::TextWrapped("and");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL(
            "here (ImPlot)",
            "https://github.com/pthom/imgui_bundle/blob/main/bindings/"
            "pyodide_web_demo/examples/demo_implot_stock.py");
        ImGui::TextWrapped("");
        ImGui::TextWrapped(
            "Finally, several good additional resources for ImGui and its "
            "add-ons are easily found online, including video tutorials.");
        ImGui::TextWrapped("A good interactive reference manual can be found");
        ImGui::SameLine();
        ImGui::TextLinkOpenURL("here",
                               "https://pthom.github.io/imgui_manual_online/"
                               "manual/imgui_manual.html");
        ImGui::End();
    }

    void DiagnosticsView::paintGL()
    {
        // Only paint if we're initialized (i.e., if the widget has been shown).
        if (!m_initialized)
        {
            return;
        }

        QOpenGLWidget::paintGL();

        // Start ImGui Frame
        ImGui_ImplQt_NewFrame(this);
        ImGui_ImplOpenGL2_NewFrame();
        ImGui::NewFrame();

        // Create a dockspace inside the Diagnostics View window
        ImGuiID dockspace_id = ImGui::GetID("DiagnosticsDockSpace");
        ImVec2 dockspace_size = ImGui::GetContentRegionAvail();
        ImGui::DockSpaceOverViewport(dockspace_id);

        // Check if we have any Python callbacks registered
        size_t numCallbacks = Rv::ImGuiPythonBridge::nbCallbacks();
        bool forceShowHelp = false;
        
        if (numCallbacks == 0 || forceShowHelp)
        {
            showHelpWindow();
        }

        if (numCallbacks > 0)
        {
            Rv::ImGuiPythonBridge::callCallbacks();
        }

        // Render ImGui Frame
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    }

    void DiagnosticsView::showEvent(QShowEvent* event)
    {
        QOpenGLWidget::showEvent(event);
        
        // Perform lazy initialization when first shown
        if (!m_initialized)
        {
            initializeImGui();
            
            // Connect the timer now that we actually need it
            connect(&m_timer, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));
        }
        
        // Start the timer when the widget becomes visible.
        m_timer.start();
    }

    void DiagnosticsView::hideEvent(QHideEvent* event)
    {
        QOpenGLWidget::hideEvent(event);
        // Stop the timer when the widget is hidden to save CPU/GPU resources.
        m_timer.stop();
    }

} // namespace Rv
