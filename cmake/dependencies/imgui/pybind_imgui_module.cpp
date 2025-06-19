//******************************************************************************
//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

// This file is part of the ImGui Python bindings module for RV (pyimgui).
// Instead of patching pybind_imgui.cpp directly, we create a separate a
// wrapper module that add the necessary NB_MODULE macro.
#include "pybind_imgui.cpp"

NB_MODULE(pyimgui, m) { py_init_module_imgui_main(m); }
