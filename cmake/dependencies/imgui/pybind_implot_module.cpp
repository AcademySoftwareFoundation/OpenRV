//******************************************************************************
//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

// This file is part of the ImPlot Python bindings module for RV (pyimgui).
// Instead of patching pybind_implot.cpp directly, we create a separate a
// wrapper module that add the necessary NB_MODULE macro.
#include "pybind_implot.cpp"

NB_MODULE(pyimplot, m) { py_init_module_implot(m); }
